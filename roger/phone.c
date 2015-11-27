/**
 * Roger Router
 * Copyright (c) 2012-2014 Jan-Michael Brummer
 *
 * This file is part of Roger Router.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libroutermanager/appobject.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/contact.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/address-book.h>
#include <libroutermanager/call.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/router.h>
#include <libroutermanager/fax_phone.h>
#include <libroutermanager/libfaxophone/phone.h>
#include <libroutermanager/libfaxophone/isdn-convert.h>

#include <roger/journal.h>
#include <roger/pref.h>
#include <roger/phone.h>
#include <roger/fax.h>
#include <roger/main.h>
#include <roger/uitools.h>
#include <roger/icons.h>
#include <roger/application.h>
#include <roger/journal.h>

#if GTK_CHECK_VERSION(3,12,0)
#define UHB 1
#endif

static GtkWidget *phone_window = NULL;
gchar *phone_voice_get_title(void);
gboolean phone_voice_init(struct contact *contact, struct connection *connection);
void phone_voice_terminated(struct phone_state *state, struct capi_connection *connection);
static GtkWidget *phone_window_create_menu(struct profile *profile, struct phone_state *state);
GtkWidget *phone_voice_create_child(struct phone_state *state, GtkWidget *grid);
void phone_voice_delete(struct phone_state *state);

void phone_voice_status(struct phone_state *state, struct capi_connection *connection)
{
}

struct phone_device phone_device_voice = {
	/* Title */
	phone_voice_get_title,
	/* Init */
	phone_voice_init,
	/* Terminated */
	phone_voice_terminated,
	/* Create menu */
	phone_window_create_menu,
	/* Create child */
	phone_voice_create_child,
	/* Delete */
	phone_voice_delete,
	/* Status */
	phone_voice_status,
};

extern struct phone_device phone_device_fax;

struct phone_device *phone_devices[2] = { &phone_device_voice, &phone_device_fax };

/**
 * \brief Create elapsed time string from two times
 * \param state phone state pointer
 * \return time string
 */
static gchar *timediff_str(struct phone_state *state)
{
	gint duration;
	static gchar buf[9];
	gint hours;
	gint minutes;
	gint seconds;
	gdouble time;

	if (!state->phone_session_timer) {
		strncpy(buf, "00:00:00", sizeof(buf));
		return buf;
	}

	time = g_timer_elapsed(state->phone_session_timer, NULL);

	duration = (gint) time;
	seconds = duration % 60;
	minutes = (duration / 60) % 60;
	hours = duration / (60 * 60);

	if (snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hours, minutes, seconds) >= 9) {
		buf[8] = '\0';
	}

	return buf;
}

/**
 * \brief Phone status timer
 * \param state pointer to phone state structure
 * \return TRUE
 */
static gboolean phone_status_timer_cb(gpointer data)
{
	struct phone_state *state = data;
	gchar *time_diff = timediff_str(state);
	gchar buf[32];

	snprintf(buf, sizeof(buf), _("Time: %s"), time_diff);

	if (roger_uses_headerbar()) {
		gtk_header_bar_set_subtitle(GTK_HEADER_BAR(state->header_bar), buf);
	} else {
		gchar *title = g_strdup_printf("%s - %s", phone_devices[state->type]->get_title(), buf);
		gtk_window_set_title(GTK_WINDOW(state->window), title);
		g_free(title);
	}

	if (state->connection) {
		/* Flush recording buffer (if needed) */
		phone_flush(state->connection);
	}

	return TRUE;
}

/**
 * \brief Setup phone status timer
 * \param state pointer to phone state structure
 */
void phone_setup_timer(struct phone_state *state)
{
	if (state->phone_session_timer && state->phone_session_timer_id) {
		g_debug("%s(): Skip timer setup", __FUNCTION__);
		return;
	}

	state->phone_session_timer = g_timer_new();
	g_timer_start(state->phone_session_timer);
	state->phone_session_timer_id = g_timeout_add(250, phone_status_timer_cb, state);
}

/**
 * \brief Remove phone status timer
 * \param state pointer to phone state structure
 */
void phone_remove_timer(struct phone_state *state)
{
	/* Make sure that the status information is updated */
	phone_status_timer_cb(state);

	/* Remove timer source */
	if (state->phone_session_timer_id) {
		g_source_remove(state->phone_session_timer_id);
		state->phone_session_timer_id = 0;
	}

	/* Remove gtimer */
	if (state->phone_session_timer) {
		g_timer_destroy(state->phone_session_timer);
		state->phone_session_timer = NULL;
	}
}

/**
 * \brief Set sensitive value of control buttons (type SOFTPHONE = true, else false)
 * \param state phone state pointer
 * \param status sensitive value
 */
static void phone_control_buttons_set_sensitive(struct phone_state *state, gboolean status)
{
	/* Set control button sensitive value */
	if (state->mute_button) {
		gtk_widget_set_sensitive(state->mute_button, status);
	}
	if (state->hold_button) {
		gtk_widget_set_sensitive(state->hold_button, status);
	}
	if (state->record_button) {
		gtk_widget_set_sensitive(state->record_button, status);
	}
}

/**
 * \brief Set sensitive value of hangup/pickup button depending of allow state
 * \param state pointer to phone state structure
 * \param allow_dial flag indicating if pick up is allowed
 */
void phone_dial_buttons_set_dial(struct phone_state *state, gboolean allow_dial)
{
	struct profile *profile = profile_get_active();

	gtk_widget_set_sensitive(state->pickup_button, allow_dial);
	gtk_widget_set_sensitive(state->hangup_button, !allow_dial);

	/* Toggle control buttons sensitive value */
	if (router_get_phone_port(profile) == PORT_SOFTPHONE) {
		phone_control_buttons_set_sensitive(state, !allow_dial);
	} else {
		phone_control_buttons_set_sensitive(state, FALSE);
	}
}

/**
 * \brief CAPI connection established callback - Starts timer if needed
 * \param object appobject
 * \param connection capi connection pointer
 * \param user_data phone state pointer
 */
static void capi_connection_established_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;

	g_debug("%s(): called", __FUNCTION__);

	if (state->connection == connection) {
		phone_setup_timer(state);
	}
}

/**
 * \brief CAPI connection terminated callback - Stops timer
 * \param object appobject
 * \param connection capi connection pointer
 * \param user_data phone state pointer
 */
static void capi_connection_terminated_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;

	g_debug("%s(): called", __FUNCTION__);

	if (state->connection != connection) {
		return;
	}

	phone_devices[state->type]->terminated(state, connection);

	/* Remove timer */
	phone_remove_timer(state);

	phone_dial_buttons_set_dial(state, TRUE);
	state->connection = NULL;
}

void capi_connection_status_cb(AppObject *object, gint status, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;

	phone_devices[state->type]->status(state, connection);
}

/**
 * \brief Phone connection failed - Show error dialog including user support text
 */
static void phone_connection_failed(struct phone_state *state)
{
	GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(state->window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Connection problem"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(error_dialog), _("This is most likely due to a closed CAPI port or an invalid phone number.\n\nSolution: Dial #96*3* with your phone and check your numbers within the settings"));

	gtk_dialog_run(GTK_DIALOG(error_dialog));
	gtk_widget_destroy(error_dialog);
}

/**
 * \brief Dialog showing that an active call is running
 * \param window parent window
 */
static void phone_active_call_dialog(GtkWidget *window)
{
	GtkWidget *dialog;

	/* Create dialog and run it */
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, _("A call is in progress, hangup first"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/**
 * \brief Reset phone control buttons (active)
 * \param state pointer to phone state structure
 */
static void phone_control_buttons_reset(struct phone_state *state)
{
	if (state->mute_button && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(state->mute_button))) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(state->mute_button), FALSE);
	}
	if (state->hold_button && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(state->hold_button))) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(state->hold_button), FALSE);
	}
	if (state->record_button && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(state->record_button))) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(state->record_button), FALSE);
	}
}

/**
 * \brief Pickup button clicked callback
 * \param button pickup button
 * \param user_data pointer to phone state structure
 */
static void pickup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct profile *profile = profile_get_active();
	struct phone_state *state = user_data;
	gchar *scramble;

	/* Get selected number (either number format or based on the selected name) */
	state->number = gtk_entry_get_text(GTK_ENTRY(state->search_entry));
	if (!EMPTY_STRING(state->number) && !(isdigit(state->number[0]) || state->number[0] == '*' || state->number[0] == '#' || state->number[0] == '+')) {
		state->number = g_object_get_data(G_OBJECT(state->search_entry), "number");
	}

	if (EMPTY_STRING(state->number)) {
		return;
	}

	scramble = call_scramble_number(state->number);
	g_debug("%s(): Want to dial '%s'", __FUNCTION__, scramble);
	g_free(scramble);

	switch (state->type) {
	case PHONE_TYPE_VOICE:
		if (router_get_phone_port(profile) == PORT_SOFTPHONE) {
			if (state->connection) {
				phone_active_call_dialog(state->window);
				return;
			}
			state->connection = phone_dial(state->number, router_get_suppress_state(profile));

			if (!state->connection) {
				phone_connection_failed(state);
			} else {
				phone_dial_buttons_set_dial(state, FALSE);
			}
		} else {
			gchar *number;

			number = g_strdup_printf("%s%s", router_get_suppress_state(profile) ? "*31#" : "", state->number);
			if (router_dial_number(profile, router_get_phone_port(profile), number)) {
				phone_dial_buttons_set_dial(state, FALSE);
			}
			g_free(number);
		}
		break;
	case PHONE_TYPE_FAX: {
		struct fax_ui *fax_ui = state->priv;

		state->connection = fax_dial(fax_ui->file, state->number, router_get_suppress_state(profile));
		if (state->connection) {
			fax_window_clear(fax_ui);
			phone_dial_buttons_set_dial(state, FALSE);
		} else {
			phone_connection_failed(state);
		}
		break;
	}
	default:
		g_warning("%s(): Unknown type selected", __FUNCTION__);
		break;
	}
}

/**
 * \brief Hangup button clicked callback
 * \param button hangup button
 * \param user_data pointer to phone state structure
 */
static void hangup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct profile *profile = profile_get_active();
	struct phone_state *state = user_data;
	const gchar *number = gtk_entry_get_text(GTK_ENTRY(state->search_entry));

	if (state->type == PHONE_TYPE_FAX || router_get_phone_port(profile) == PORT_SOFTPHONE) {
		if (!state->connection) {
			return;
		}

		g_debug("%s(): Hangup requested for %p", __FUNCTION__, state->connection);
		phone_control_buttons_reset(state);
		phone_hangup(state->connection);
		phone_dial_buttons_set_dial(state, TRUE);
	} else {
		if (router_hangup(profile, router_get_phone_port(profile), number)) {
			phone_dial_buttons_set_dial(state, TRUE);
		}
	}
}

/**
 * \brief Returns a string describing the phone number type
 * \param type phone number type
 * \return phone number type string
 */
static gchar *phone_number_type_to_string(enum phone_number_type type)
{
	gchar *tmp;

	switch (type) {
	case PHONE_NUMBER_HOME:
		tmp = g_strdup(_("Home"));
		break;
	case PHONE_NUMBER_WORK:
		tmp = g_strdup(_("Work"));
		break;
	case PHONE_NUMBER_MOBILE:
		tmp = g_strdup(_("Cell"));
		break;
	case PHONE_NUMBER_FAX_HOME:
		tmp = g_strdup(_("Fax Home"));
		break;
	case PHONE_NUMBER_FAX_WORK:
		tmp = g_strdup(_("Fax Work"));
		break;
	default:
		tmp = g_strdup(_("Unknown"));
		break;
	}

	return tmp;
}

/**
 * \brief Set contact name in phone dialog
 * \param state phone state structure
 * \param contact contact structure
 * \param identify flag indicating if we want to identify contact
 */
static void phone_search_entry_set_contact(struct phone_state *state, struct contact *contact, gboolean identify)
{
	struct contact *search_contact;

	if (!state || !contact) {
		return;
	}

	if (identify) {
		/* Copy contact and try to identify it */
		search_contact = contact_dup(contact);
		emit_contact_process(search_contact);
	} else {
		search_contact = contact;
	}

	g_object_set_data(G_OBJECT(state->search_entry), "number", search_contact->number);

	if (!EMPTY_STRING(search_contact->name)) {
		gtk_entry_set_text(GTK_ENTRY(state->search_entry), search_contact->name);
	} else {
		gtk_entry_set_text(GTK_ENTRY(state->search_entry), search_contact->number);
	}
}

/**
 * \brief Set contact name in phone dialog using list box row
 * \param state phone state structure
 * \param row list box row
 */
static void phone_search_entry_set_contact_by_row(struct phone_state *state, GtkListBoxRow *row)
{
	GtkWidget *grid = gtk_bin_get_child(GTK_BIN(row));
	struct contact *contact;
	gchar *number = g_object_get_data(G_OBJECT(grid), "number");

	contact = g_object_get_data(G_OBJECT(grid), "contact");
	contact->number = number;
	state->discard = TRUE;
	phone_search_entry_set_contact(state, contact, FALSE);

	gtk_widget_destroy(state->contact_menu);
}

/**
 * \brief Closed callback for contact menu - clean internal data
 * \param widget menu widget
 * \param user_data pointer to phone state structure
 */
static void phone_contact_menu_closed_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;

	/* Clear internal data */
	state->contact_menu = NULL;
	state->box = NULL;
}

/**
 * \brief Destroy contact in list box
 * \param widget list box child widget
 * \param user_data UNUSED
 */
static void phone_destroy_contacts(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
}

/**
 * \brief List box row has been activated - set phone number details
 * \param box list box widget
 * \param row list box row widget
 * \param user_data pointer to phone state structure
 */
static void phone_list_box_activated_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	struct phone_state *state = user_data;

	g_assert(state != NULL);
	g_assert(row != NULL);

	/* Set phone number details */
	phone_search_entry_set_contact_by_row(state, row);
}

/**
 * \brief Filter search entry
 * \param row list box row
 * \param user_data pointer to phone state structure
 * \return TRUE if there is a match, FALSE if no match has been found
 */
static gboolean phone_search_entry_filter_cb(GtkListBoxRow *row, gpointer user_data)
{
	GtkWidget *grid = gtk_bin_get_child(GTK_BIN(row));
	struct phone_state *state = user_data;
	struct contact *contact = g_object_get_data(G_OBJECT(grid), "contact");

	g_assert(state != NULL);
	g_assert(contact != NULL);

	if (EMPTY_STRING(state->filter)) {
		return TRUE;
	}

	return g_strcasestr(contact->name, state->filter) != NULL;
}

/**
 * \brief Search changed callback for search entry
 * \param entry search entry
 * \param user_Data pointer to phone state structure
 */
static void phone_search_entry_search_changed_cb(GtkSearchEntry *entry, gpointer user_data)
{
	GtkWidget *label;
	struct phone_state *state = user_data;
	GSList *contacts = NULL;
	GSList *list;

	/* Get current filter text */
	state->filter = gtk_entry_get_text(GTK_ENTRY(entry));

	/* If it is an invalid filter, abort and close menu if needed */
	if (EMPTY_STRING(state->filter) || isdigit(state->filter[0]) || state->discard || state->filter[0] == '*' || state->filter[0] == '#') {
		state->discard = FALSE;
		if (state->contact_menu) {
			gtk_widget_destroy(state->contact_menu);
		}
		return;
	}

	/* if needed, create contact menu */
	if (!state->contact_menu) {
		GtkWidget *placeholder;
		gint width, height;

		/* Create popover */
#if GTK_CHECK_VERSION(3,12,0)
		state->contact_menu = gtk_popover_new(NULL);
		gtk_popover_set_modal(GTK_POPOVER(state->contact_menu), FALSE);
		gtk_popover_set_position(GTK_POPOVER(state->contact_menu), GTK_POS_BOTTOM);
		gtk_popover_set_relative_to(GTK_POPOVER(state->contact_menu), GTK_WIDGET(entry));
#else
		state->contact_menu = gtk_menu_new();
#endif

		width = gtk_widget_get_allocated_width(state->search_entry) + gtk_widget_get_allocated_width(state->pickup_button) + gtk_widget_get_allocated_width(state->hangup_button);
		height = gtk_widget_get_allocated_height(state->child_frame) + gtk_widget_get_allocated_height(state->search_entry);

		gtk_widget_set_size_request(state->contact_menu, width, height);

		gtk_container_set_border_width(GTK_CONTAINER(state->contact_menu), 6);
		g_signal_connect(G_OBJECT(state->contact_menu), "closed", G_CALLBACK(phone_contact_menu_closed_cb), state);

		state->scrolled_win = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(state->scrolled_win), GTK_SHADOW_ETCHED_IN);
#if GTK_CHECK_VERSION(3,12,0)
		gtk_container_add(GTK_CONTAINER(state->contact_menu), state->scrolled_win);
#else
		GtkWidget *item = gtk_menu_item_new();
		gtk_container_add(GTK_CONTAINER(item), state->scrolled_win);
		gtk_widget_show_all(item);
		gtk_menu_attach(GTK_MENU(state->contact_menu), item, 0, 2, 0, 2);
#endif

		state->box = gtk_list_box_new();
		placeholder = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(placeholder), _("<b>No contacts found</b>"));
		gtk_widget_set_visible(placeholder, TRUE);
		gtk_list_box_set_placeholder(GTK_LIST_BOX(state->box), placeholder);
		gtk_list_box_set_filter_func(GTK_LIST_BOX(state->box), phone_search_entry_filter_cb, state, NULL);
		gtk_container_add(GTK_CONTAINER(state->scrolled_win), state->box);
		g_signal_connect(G_OBJECT(state->box), "row-activated", G_CALLBACK(phone_list_box_activated_cb), state);
		gtk_widget_show_all(state->contact_menu);
	}

	/* Clean previous contacts */
	gtk_container_foreach(GTK_CONTAINER(state->box), phone_destroy_contacts, NULL);

	state->filter = gtk_entry_get_text(GTK_ENTRY(entry));

	/* Add contacts to entry completion */
	contacts = address_book_get_contacts();

	for (list = contacts; list; list = list->next) {
		struct contact *contact = list->data;
		GSList *numbers;
		GtkWidget *num;
		GtkWidget *grid;
		gchar *name;

		if (!g_strcasestr(contact->name, state->filter) || !contact->numbers) {
			continue;
		}

		name = g_strdup_printf("<b>%s</b>", contact->name);

		for (numbers = contact->numbers; numbers != NULL; numbers = numbers->next) {
			struct phone_number *number = numbers->data;
			GtkWidget *image;
			gchar *num_str;
			gchar *type;

			grid = gtk_grid_new();

			/* Set standard spacing */
			gtk_grid_set_row_spacing(GTK_GRID(grid), 2);
			gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

			g_object_set_data(G_OBJECT(grid), "contact", contact);
			g_object_set_data(G_OBJECT(grid), "number", number->number);

			if (contact && contact->image) {
				GdkPixbuf *buf = image_get_scaled(contact->image, 32, 32);

				image = gtk_image_new_from_pixbuf(buf);
				g_object_unref(buf);
			} else {
				image = gtk_image_new_from_icon_name("avatar-default-symbolic", GTK_ICON_SIZE_DND);
			}

			gtk_grid_attach(GTK_GRID(grid), image, 0, 0, 1, 2);

			label = gtk_label_new(NULL);
			gtk_label_set_markup(GTK_LABEL(label), name);
			gtk_widget_set_halign(label, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(grid), label, 1, 0, 1, 1);

			type = phone_number_type_to_string(number->type);
			num_str = g_strdup_printf("%s: %s", type, number->number);
			num = gtk_label_new(num_str);
			g_free(num_str);
			g_free(type);
			gtk_widget_set_halign(num, GTK_ALIGN_START);
			gtk_widget_set_sensitive(num, FALSE);
			gtk_grid_attach(GTK_GRID(grid), num, 1, 1, 1, 1);
			gtk_widget_show_all(grid);

			gtk_list_box_insert(GTK_LIST_BOX(state->box), grid, -1);
		}

		g_free(name);
	}
}

#if UHB
/**
 * \brief Helper function which focus the active child within the list box
 * \param scrolled_win scrolled window
 * \param box list box widget
 * \param row active row within list box
 */
static void phone_list_box_set_focus(GtkWidget *scrolled_win, GtkWidget *box, GtkListBoxRow *row)
{
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_win));
	gint wleft, wtop, wbottom;
	gdouble top, bottom, value;

	/* Get coordinated */
	gtk_widget_translate_coordinates(GTK_WIDGET(row), box, 0, 0, &wleft, &wtop);
	wbottom = wtop + gtk_widget_get_allocated_height(GTK_WIDGET(row));

	/* Compute new vertial adjustment value */
	top = gtk_adjustment_get_value(vadj);
	bottom = top + gtk_adjustment_get_page_size(vadj);

	if (wtop < top) {
		value = wtop;
	} else if (wbottom > bottom) {
		value = wbottom - gtk_adjustment_get_page_size(vadj);
	} else {
		value = top;
	}

	/* Set value */
	gtk_adjustment_set_value(vadj, value);
}
#endif

/**
 * \brief icon of search entry pressed
 * \param entry search entry widget
 * \param icon_pos icon position
 * \param event active event
 * \param user_data pointer to phone_state structure
 */
static void phone_search_entry_icon_press_cb(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
	struct phone_state *state = user_data;

	if (icon_pos == GTK_ENTRY_ICON_PRIMARY) {
		/* If primary icon has been pressed, toggle menu */
		if (!state->contact_menu) {
			phone_search_entry_search_changed_cb(GTK_SEARCH_ENTRY(entry), user_data);
		} else {
			gtk_widget_destroy(state->contact_menu);
		}
	} else if (icon_pos == GTK_ENTRY_ICON_SECONDARY) {
		g_object_set_data(G_OBJECT(state->search_entry), "number", NULL);
	}
}

#if UHB
/**
 * \brief Handle cursor up/down in name entry field which controls the open popover menu (listbox) without losing focus
 * \param entry entry widget
 * \param event active event
 * \param user_data pointer to phone state structure
 * \return TRUE if should be propagated, otherwise FALSE
 */
static gboolean phone_search_entry_key_press_event_cb(GtkWidget *entry, GdkEvent *event, gpointer user_data)
{
	GtkListBoxRow *row = NULL;
	struct phone_state *state = user_data;
	GList *childs;
	guint keyval = ((GdkEventKey *)event)->keyval;
	gint length;

	if (!state->box) {
		return FALSE;
	}

	/* Escape pressed, unselect all and close menu */
	if (keyval == GDK_KEY_Escape) {
		gtk_list_box_unselect_all(GTK_LIST_BOX(state->box));
		gtk_widget_destroy(state->contact_menu);
		return TRUE;
	}
	/* Get selected row */
	row = gtk_list_box_get_selected_row(GTK_LIST_BOX(state->box));
	if (!row) {
		row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->box), 0);
	}

	if (!row) {
		return FALSE;
	}

	/* If we have a selected row and return/enter is pressed, set number and exit */
	if (gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row)) && (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter || keyval == GDK_KEY_ISO_Enter)) {
		phone_search_entry_set_contact_by_row(state, row);

		return TRUE;
	}

	/* Get length of listbox childs */
	childs = gtk_container_get_children(GTK_CONTAINER(state->box));
	g_assert(childs != NULL);

	length = g_list_length(childs);

	if (keyval == GDK_KEY_Up) {
		/* Handle key up */
		if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->box), length - 1);
			gtk_list_box_select_row(GTK_LIST_BOX(state->box), GTK_LIST_BOX_ROW(row));
			phone_list_box_set_focus(state->scrolled_win, state->box, row);
			return FALSE;
		}

		if (gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) > 0) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->box), gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) - 1);
			gtk_list_box_select_row(GTK_LIST_BOX(state->box), GTK_LIST_BOX_ROW(row));
		} else {
			gtk_list_box_unselect_all(GTK_LIST_BOX(state->box));
		}
	} else if (keyval == GDK_KEY_Down) {
		/* Handle key down */
		if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
			gtk_list_box_select_row(GTK_LIST_BOX(state->box), GTK_LIST_BOX_ROW(row));
			phone_list_box_set_focus(state->scrolled_win, state->box, row);
			return FALSE;
		}

		if (gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) < (length - 1)) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->box), gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) + 1);
			gtk_list_box_select_row(GTK_LIST_BOX(state->box), GTK_LIST_BOX_ROW(row));

		} else {
			gtk_list_box_unselect_all(GTK_LIST_BOX(state->box));
		}
	}

	phone_list_box_set_focus(state->scrolled_win, state->box, row);

	return FALSE;
}
#else
static gboolean on_match_selected_cb(GtkEntryCompletion *completion, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GValue value_contact = {0, };
	GValue value_number = {0, };
	struct phone_state *state = user_data;
	struct contact *contact;
	const gchar *number;

	gtk_tree_model_get_value(model, iter, 2, &value_number);
	gtk_tree_model_get_value(model, iter, 3, &value_contact);
	number = g_value_get_string(&value_number);
	contact = g_value_get_pointer(&value_contact);

	contact->number = g_strdup(number);
	state->discard = TRUE;
	phone_search_entry_set_contact(state, contact, FALSE);

	return FALSE;
}
#endif

/**
 * \brief Create name (number) frame
 * \param window phone window
 * \param contact contact structure (fill name entry if present)
 * \param state phone state pointer
 * \return name frame
 */
GtkWidget *phone_search_entry_new(GtkWidget *window, struct contact *contact, struct phone_state *state)
{
	/* Name entry */
	state->search_entry = gtk_search_entry_new();

	/* FIXME: Width is currently hard coded... */
	gtk_widget_set_size_request(state->search_entry, 270, -1);

	/* Make primary icon clickable */
	g_object_set(state->search_entry, "primary-icon-activatable", TRUE, "primary-icon-sensitive", TRUE, NULL);

	gtk_widget_set_tooltip_text(state->search_entry, _("Enter name or number"));
	gtk_entry_set_placeholder_text(GTK_ENTRY(state->search_entry), _("Enter name or number"));
	gtk_widget_set_hexpand(state->search_entry, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(state->search_entry), TRUE);
	g_signal_connect(state->search_entry, "icon-press", G_CALLBACK(phone_search_entry_icon_press_cb), state);

	/* Set contact name */
	state->discard = TRUE;
	phone_search_entry_set_contact(state, contact, TRUE);

#if UHB
	g_signal_connect(state->search_entry, "search-changed", G_CALLBACK(phone_search_entry_search_changed_cb), state);
	g_signal_connect(state->search_entry, "key-press-event", G_CALLBACK(phone_search_entry_key_press_event_cb), state);
#else
	GSList *l = address_book_get_contacts();
	GtkTreeIter iter;

	GtkEntryCompletion *completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 1);
	gtk_entry_set_completion(GTK_ENTRY(state->search_entry), completion);
	g_signal_connect(G_OBJECT(completion), "match-selected", G_CALLBACK(on_match_selected_cb), state);

	GtkListStore *model = gtk_list_store_new(4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

	while (l) {
		struct contact *c = l->data;
		GSList *numbers;
		gchar *name;

		for (numbers = c->numbers; numbers != NULL; numbers = numbers->next) {
			struct phone_number *number = numbers->data;

			gtk_list_store_append(model, &iter);

			name = g_strdup_printf("%s (%s: %s)", c->name, phone_number_type_to_string(number->type), number->number);
			gtk_list_store_set(model, &iter, 0, c->image, 1, name, 2, number->number, 3, c, -1);
		}
		l = l->next;
	}

	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(model));
#endif

	return state->search_entry;
}

/**
 * \brief Create dial button frame (pickup/hangup)
 * \param window window widget in order to set the dedault action
 * \param state phone state pointer
 * \return dial button frame
 */
GtkWidget *phone_dial_buttons_new(GtkWidget *window, struct phone_state *state)
{
	GtkWidget *grid;
	GtkWidget *image;

	/* Create grid */
	grid = gtk_grid_new();
	gtk_widget_set_can_focus(grid, FALSE);

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	/* Pickup button */
	state->pickup_button = gtk_button_new();
	gtk_widget_set_hexpand(state->pickup_button, TRUE);
	image = gtk_image_new_from_icon_name(APP_ICON_CALL, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(state->pickup_button), image);
	ui_set_suggested_style(state->pickup_button);

	gtk_widget_set_tooltip_text(state->pickup_button, _("Pick up"));
	g_signal_connect(state->pickup_button, "clicked", G_CALLBACK(pickup_button_clicked_cb), state);
	gtk_widget_set_can_default(state->pickup_button, TRUE);
	gtk_window_set_default(GTK_WINDOW(window), state->pickup_button);
	gtk_grid_attach(GTK_GRID(grid), state->pickup_button, 0, 0, 1, 1);

	/* Hangup button */
	state->hangup_button = gtk_button_new();
	gtk_widget_set_hexpand(state->hangup_button, TRUE);
	image = gtk_image_new_from_icon_name(APP_ICON_HANGUP, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(state->hangup_button), image);
#if UHN
	gtk_style_context_add_class(gtk_widget_get_style_context(state->hangup_button), GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);
#endif

	gtk_widget_set_tooltip_text(state->hangup_button, _("Hang up"));
	g_signal_connect(state->hangup_button, "clicked", G_CALLBACK(hangup_button_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(grid), state->hangup_button, 1, 0, 1, 1);

	return grid;
}

/**
 * \brief Create dial pad/control button
 * \param text_hi text high
 * \param text_lo text low
 * \param icon button icon
 * \return button widget
 */
static inline GtkWidget *phone_button_new(const gchar *text_hi, const gchar *text_lo, GtkWidget *icon)
{
	GtkWidget *button;
	GtkWidget *label_hi;
	GtkWidget *label_lo;
	GtkWidget *grid;

	/* Create grid */
	grid = gtk_grid_new();
	gtk_widget_set_can_focus(grid, FALSE);
	gtk_widget_set_hexpand(grid, TRUE);
	gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);

	if (!icon) {
		/* We have no icon, so create a button with text hi/lo */
		button = gtk_button_new();

		label_hi = gtk_label_new(text_hi);
		gtk_label_set_use_markup(GTK_LABEL(label_hi), TRUE);
		gtk_grid_attach(GTK_GRID(grid), label_hi, 0, 0, 1, 1);
	} else {
		/* We have an icon, so create a toggle button with text lo */
		button = gtk_toggle_button_new();

		gtk_grid_attach(GTK_GRID(grid), icon, 0, 0, 1, 1);
		gtk_widget_set_margin(icon, 4, 3, 4, 4);
	}

	/* Set label low */
	gtk_widget_set_can_focus(button, FALSE);
	label_lo = gtk_label_new(text_lo);
	gtk_label_set_use_markup(GTK_LABEL(label_lo), TRUE);
	gtk_grid_attach(GTK_GRID(grid), label_lo, 0, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(button), grid);

	return button;
}

/**
 * \brief Dialpad button pressed - send either dtmf code or add number to entry
 * \param widget button widget
 * \param user_data phone state pointer
 */
static void phone_dtmf_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;
	gint num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "num"));

	g_assert(state != NULL);

	if (state->connection) {
		/* Active connection is available, send dtmf code */
		phone_send_dtmf_code(state->connection, num);
	} else {
		/* Add number to entry */
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(state->search_entry));
		gchar *tmp = g_strdup_printf("%s%c", text, num);

		gtk_entry_set_text(GTK_ENTRY(state->search_entry), tmp);
		g_free(tmp);
	}
}

/** Dialpad button structure */
static struct phone_label {
	gchar *text;
	gchar *subtext;
	gchar chr;
} phone_labels[] = {
	{"<b>1</b>", "", '1'},
	{"<b>2</b>", "<small>ABC</small>", '2'},
	{"<b>3</b>", "<small>DEF</small>", '3'},
	{"<b>4</b>", "<small>GHI</small>", '4'},
	{"<b>5</b>", "<small>JKL</small>", '5'},
	{"<b>6</b>", "<small>MNO</small>", '6'},
	{"<b>7</b>", "<small>PQRS</small>", '7'},
	{"<b>8</b>", "<small>TUV</small>", '8'},
	{"<b>9</b>", "<small>WXYZ</small>", '9'},
	{"<b>*</b>", "", '*'},
	{"<b>0</b>", "<small>+</small>", '0'},
	{"<b>#</b>", "", '#'},
	{NULL, NULL}
};

/**
 * \brief Create dialpad frame
 * \param state phone state pointer
 * \return dialpad widget
 */
GtkWidget *phone_dialpad_new(struct phone_state *state)
{
	GtkWidget *frame;
	GtkWidget *grid;
	GtkWidget *button;
	gint index;

	/* Create frame */
	frame = gtk_frame_new(NULL);
	gtk_widget_set_can_focus(frame, FALSE);

	/* Create grid */
	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(frame), grid);

	/* Set standard spacing */
	gtk_widget_set_margin(grid, 6, 6, 6, 6);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);

	/* Add buttons */
	for (index = 0; phone_labels[index].text != NULL; index++) {
		button = phone_button_new(phone_labels[index].text, phone_labels[index].subtext, NULL);
		g_object_set_data(G_OBJECT(button), "num", GINT_TO_POINTER(phone_labels[index].chr));
		g_signal_connect(button, "clicked", G_CALLBACK(phone_dtmf_clicked_cb), state);
		gtk_grid_attach(GTK_GRID(grid), button, index % 3, index / 3, 1, 1);
	}

	return frame;
}

/**
 * \brief Hold button clicked callback - toggle hold call state
 * \param widget button widget
 * \param user_data phone state pointer
 */
static void phone_control_button_hold_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;

	if (!state || !state->connection) {
		return;
	}

	/* Toggle hold call */
	phone_hold(state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/**
 * \brief Mute button clicked callback - toggle mute call state
 * \param widget button widget
 * \param user_data phone state pointer
 */
static void phone_control_button_mute_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;

	if (!state || !state->connection) {
		return;
	}

	/* Toggle mute call */
	phone_mute(state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/**
 * \brief Record button clicked callback - toggle record call state
 * \param widget button widget
 * \param user_data phone state pointer
 */
static void phone_control_button_record_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;
	const gchar *user_plugins;
	gchar *path;

	if (!state || !state->connection) {
		return;
	}

	/* Create roger record directory */
	user_plugins = g_get_user_data_dir();
	path = g_build_filename(user_plugins, "roger", G_DIR_SEPARATOR_S, NULL);
	g_mkdir_with_parents(path, 0700);

	/* Toggle record state */
	phone_record(state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)), path);

	g_free(path);
}

/**
 * \brief Clear button clicked callback - removes last char of number/name
 * \param widget button widget
 * \param user_data phone state pointer
 */
static void phone_control_button_clear_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;
	GtkWidget *entry = state->search_entry;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	/* If there is text within the entry, remove last char */
	if (!EMPTY_STRING(text)) {
		gchar *new = g_strdup(text);

		new[strlen(text) - 1] = '\0';
		gtk_entry_set_text(GTK_ENTRY(entry), new);
		g_free(new);
	}
}

/**
 * \brief Create phone control buttons frame
 * \param state phone state pointer
 * \return phone control frame
 */
GtkWidget *phone_control_buttons_new(struct phone_state *state)
{
	GtkWidget *frame;
	GtkWidget *grid;
	GtkWidget *button;
	GtkWidget *image;

	/* Create frame */
	frame = gtk_frame_new(NULL);
	gtk_widget_set_can_focus(frame, FALSE);

	/* Create inner grid */
	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(frame), grid);

	/* Set standard spacing */
	gtk_widget_set_margin(grid, 6, 6, 6, 6);
	gtk_widget_set_hexpand(grid, FALSE);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);

	/* Add clear button */
	button = phone_button_new("<b>C</b>", _("<small>Clear</small>"), NULL);
	g_signal_connect(button, "clicked", G_CALLBACK(phone_control_button_clear_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

	/* Add mute button */
	image = gtk_image_new_from_icon_name("audio-input-microphone-symbolic", GTK_ICON_SIZE_BUTTON);
	state->mute_button = phone_button_new(NULL, _("<small>Mute</small>"), image);
	g_signal_connect(state->mute_button, "clicked", G_CALLBACK(phone_control_button_mute_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(grid), state->mute_button, 0, 1, 1, 1);

	/* Add hold button */
	image = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
	state->hold_button = phone_button_new(NULL, _("<small>Hold</small>"), image);
	g_signal_connect(state->hold_button, "clicked", G_CALLBACK(phone_control_button_hold_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(grid), state->hold_button, 0, 2, 1, 1);

	/* Add record button */
	image = gtk_image_new_from_icon_name("media-record-symbolic", GTK_ICON_SIZE_BUTTON);
	state->record_button = phone_button_new(NULL, _("<small>Record</small>"), image);
	g_signal_connect(state->record_button, "clicked", G_CALLBACK(phone_control_button_record_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(grid), state->record_button, 0, 3, 1, 1);

	return frame;
}

/**
 * \brief Phone window delete event callback
 * \param widget window widget
 * \param event event structure
 * \param data pointer to phone_state structure
 * \return FALSE if window should be deleted, otherwise TRUE
 */
static gboolean phone_window_delete_event_cb(GtkWidget *window, GdkEvent *event, gpointer data)
{
	struct phone_state *state = data;

	/* If there is an active connection, warn user and refuse to delete the window */
	if (state && state->connection) {
		phone_active_call_dialog(window);

		/* Keep window */
		return TRUE;
	}

	/* Disconnect all signal handlers */
	g_signal_handlers_disconnect_by_data(app_object, state);

	phone_devices[state->type]->delete(state);

	if (state->window == phone_window) {
		phone_window = NULL;
	}
	state->window = NULL;

	/* Free phone state structure and release phone window */
	g_slice_free(struct phone_state, state);

	return FALSE;
}

/**
 * \brief Phone toggled callback - sets selected port type to profile settings
 * \param item percieved check menu item
 * \param user_data containing pointer to corresponding phone type
 */
void phone_item_toggled_cb(GtkCheckMenuItem *item, gpointer user_data)
{
#if GTK_CHECK_VERSION(3,12,0)
	/* If item is active, adjust phone port accordingly and set sensitivity of phone buttons */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item))) {
		gint type = GPOINTER_TO_INT(user_data);

		/* Set active phone port */
		router_set_phone_port(profile_get_active(), type);
	}
#else
	/* If item is active, adjust phone port accordingly and set sensitivity of phone buttons */
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {
		gint type = GPOINTER_TO_INT(user_data);

		/* Set active phone port */
		router_set_phone_port(profile_get_active(), type);
	}
#endif
}

/**
 * \brief Create phone window menu which contains phone selection and suppress number toggle
 * \param profile pointer to current profile
 * \param state phone state pointer
 * \return newly create phone menu
 */
static GtkWidget *phone_window_create_menu(struct profile *profile, struct phone_state *state)
{
	GtkWidget *menu;
	GtkWidget *item;
	GSList *phone_list = NULL;
	struct phone *phone = NULL;
	gint type = 0;

#if GTK_CHECK_VERSION(3,12,0)
	GtkWidget *box;
	GSList *phone_radio_list = NULL;

	/* Create vertical box */
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin(box, 12, 12, 12, 12);

	/* Create popover */
	menu = gtk_popover_new(NULL);
	gtk_container_add(GTK_CONTAINER(menu), box);

	/* Fill menu */
	item = gtk_label_new(_("Phones"));
	gtk_widget_set_sensitive(item, FALSE);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

	/* Get phone list and active phone port */
	phone_list = router_get_phone_list(profile);
	type = router_get_phone_port(profile);

	/* Traverse phone list and add each phone */
	while (phone_list) {
		phone = phone_list->data;

		item = gtk_radio_button_new_with_label(phone_radio_list, phone->name);

		phone_radio_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(item));
		if (type == phone->type) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item), TRUE);
		}

		g_signal_connect(item, "toggled", G_CALLBACK(phone_item_toggled_cb), GINT_TO_POINTER(phone->type));
		g_object_set_data(G_OBJECT(item), "phone_state", state);

		gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);
		phone_list = phone_list->next;
	}

	/* Free phone list */
	router_free_phone_list(phone_list);

	/* Add separator */
	item = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

	/* Add suppress check item */
	item = gtk_check_button_new_with_label(_("Suppress number"));
	g_settings_bind(profile->settings, "suppress", item, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

	gtk_widget_show_all(box);
#else
	GSList *group = NULL;
	gint y = 0;

	menu = gtk_menu_new();

	item = gtk_menu_item_new_with_label(_("Phones"));
	gtk_widget_set_sensitive(item, FALSE);
	gtk_menu_attach(GTK_MENU(menu), item, 0, 1, y, y + 1);
	y++;

	/* Get phone list and active phone port */
	phone_list = router_get_phone_list(profile);
	type = router_get_phone_port(profile);

	/* Traverse phone list and add each phone */
	while (phone_list) {
		phone = phone_list->data;

		item = gtk_radio_menu_item_new_with_label(group, phone->name);
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));

		if (type == phone->type) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		}

		g_signal_connect(item, "toggled", G_CALLBACK(phone_item_toggled_cb), GINT_TO_POINTER(phone->type));
		g_object_set_data(G_OBJECT(item), "phone_state", state);

		gtk_menu_attach(GTK_MENU(menu), item, 0, 1, y, y + 1);
		y++;
		phone_list = phone_list->next;
	}

	/* Free phone list */
	router_free_phone_list(phone_list);

	/* Add separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_attach(GTK_MENU(menu), item, 0, 1, y, y + 1);
	y++;

	/* Add suppress check item */
	item = gtk_check_menu_item_new_with_label(_("Suppress number"));
	g_settings_bind(profile->settings, "suppress", item, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_menu_attach(GTK_MENU(menu), item, 0, 1, y, y + 1);
	y++;

	gtk_widget_show_all(menu);
#endif

	return menu;
}

/**
 * \brief Pick up incoming connection - either valid connection argument or active capi instead
 * \param state phone state pointer
 * \param connection incoming connection
 */
static void phone_pickup_incoming(struct phone_state *state, struct connection *connection)
{
	struct capi_connection *capi_connection;

	g_assert(state != NULL);
	g_assert(connection != NULL);

	capi_connection = connection->priv ? connection->priv : active_capi_connection;

	/* Pick up */
	if (capi_connection && !phone_pickup(capi_connection)) {
		/* Set internal connection */
		state->connection = capi_connection;

		/* Disable dial button */
		phone_dial_buttons_set_dial(state, FALSE);
	}
}

/**
 * \brief Create and show phone window
 * \param contact pointer to contact structure
 * \param connection incoming connection to pickup
 */
GtkWidget *phone_window_new(enum phone_type type, struct contact *contact, struct connection *connection, gpointer priv)
{
	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *menu_button;
	GtkWidget *grid;
	GtkWidget *frame;
	struct phone_state *state;
	struct profile *profile = profile_get_active();
	gint y = 0;

	if (!profile) {
		return NULL;
	}

	if (!phone_devices[type]->init(contact, connection)) {
		return NULL;
	}

	/* Allocate phone state structure */
	state = g_slice_alloc0(sizeof(struct phone_state));
	state->type = type;
	state->priv = priv;

	/* Create window */
	state->window = window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), phone_devices[state->type]->get_title());
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	g_object_set_data(G_OBJECT(window), "state", state);
	g_signal_connect(window, "delete_event", G_CALLBACK(phone_window_delete_event_cb), state);
	gtk_application_add_window(roger_app, GTK_WINDOW(window));

	/* Create grid and attach it to the window */
	grid = gtk_grid_new();

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 18);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	/* Set margin to 12px for all sides */
	gtk_widget_set_margin(grid, 12, 12, 12, 12);

	/* Create and add menu button to header bar */
	menu_button = gtk_menu_button_new();
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(menu_button), GTK_ARROW_NONE);

	if (roger_uses_headerbar()) {
		/* Create header bar and set it to window */
		header = gtk_header_bar_new();
		gtk_header_bar_set_title(GTK_HEADER_BAR(header), phone_devices[state->type]->get_title());
		gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header), _("Time: 00:00:00"));
		gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
		gtk_window_set_titlebar(GTK_WINDOW(window), header);
		gtk_header_bar_pack_end(GTK_HEADER_BAR(header), menu_button);
		gtk_container_add(GTK_CONTAINER(window), grid);
		state->header_bar = header;
	} else {
		gchar *title = g_strdup_printf("%s - %s", phone_devices[state->type]->get_title(), _("Time: 00:00:00"));
		gtk_window_set_title(GTK_WINDOW(window), title);
		GtkWidget *main_grid = gtk_grid_new();
		gtk_widget_set_hexpand(menu_button, FALSE);
		gtk_widget_set_halign(menu_button, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(main_grid), grid, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(main_grid), menu_button, 0, 1, 1, 1);
		gtk_widget_set_margin(menu_button, 12, 0, 0, 12);
		gtk_container_add(GTK_CONTAINER(window), main_grid);
		state->header_bar = window;
	}

	/* Create menu and add it to menu button */
#if GTK_CHECK_VERSION(3,12,0)
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), phone_devices[state->type]->create_menu(profile, state));
#else
	gtk_container_add(GTK_CONTAINER(menu_button), gtk_image_new_from_icon_name("view-list-symbolic", GTK_ICON_SIZE_MENU));
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(menu_button), phone_devices[state->type]->create_menu(profile, state));
#endif

	/* Add name frame */
	frame = phone_search_entry_new(window, contact, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, y, 1, 1);

	/* Add dial button frame */
	frame = phone_dial_buttons_new(window, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 1, y, 1, 1);

	/* Add dial pad */
	phone_devices[state->type]->create_child(state, grid);

	/* Connect connection signals */
	g_signal_connect(app_object, "connection-established", G_CALLBACK(capi_connection_established_cb), state);
	g_signal_connect(app_object, "connection-terminated", G_CALLBACK(capi_connection_terminated_cb), state);
	g_signal_connect(app_object, "connection-status", G_CALLBACK(capi_connection_status_cb), state);

	/* In case there is an incoming connection, pick it up */
	if (connection) {
		phone_pickup_incoming(state, connection);
	} else {
		/* No incoming call, allow dialing */
		phone_dial_buttons_set_dial(state, TRUE);
	}

	/* Show window */
	gtk_widget_show_all(window);

	return window;
}

gchar *phone_voice_get_title(void)
{
	return _("Phone");
}

gboolean phone_voice_init(struct contact *contact, struct connection *connection)
{
	/* If there is already an open phone window, present it to the user and return */
	if (phone_window) {
		struct phone_state *state;

		state = g_object_get_data(G_OBJECT(phone_window), "state");

		/* Incoming call? */
		if (state && !state->connection && connection) {
			phone_pickup_incoming(state, connection);

			/* Set contact name */
			phone_search_entry_set_contact(state, contact, TRUE);
		}

		gtk_window_present(GTK_WINDOW(phone_window));
		return FALSE;
	}

	return TRUE;
}

GtkWidget *phone_voice_create_child(struct phone_state *state, GtkWidget *grid)
{
	state->child_frame = phone_dialpad_new(state);
	gtk_grid_attach(GTK_GRID(grid), state->child_frame, 0, 1, 1, 1);

	/* Add control frame */
	state->control_frame = phone_control_buttons_new(state);
	gtk_grid_attach(GTK_GRID(grid), state->control_frame, 1, 1, 1, 1);

	return state->child_frame;
}

void phone_voice_terminated(struct phone_state *state, struct capi_connection *connection)
{
}

void phone_voice_delete(struct phone_state *state)
{
}

void app_show_phone_window(struct contact *contact, struct connection *connection)
{
	GtkWidget *window;

	window = phone_window_new(PHONE_TYPE_VOICE, contact, connection, NULL);

	if (!phone_window) {
		phone_window = window;
	}
}
