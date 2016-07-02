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

#include <config.h>

#include <string.h>

#include <gtk/gtk.h>

#include <libroutermanager/appobject.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/router.h>
#include <libroutermanager/device_phone.h>
#include <libroutermanager/address-book.h>
#include <libroutermanager/gstring.h>

#include <roger/main.h>
#include <roger/journal.h>
#include <roger/phone.h>
#include <roger/uitools.h>
#include <roger/icons.h>

#include <roger/contacts.h>

static struct phone_state *phone_state = NULL;

static gchar *phone_get_active_name(void)
{
	GSList *phone_list;
	struct profile *profile = profile_get_active();
	gchar *name = NULL;
	gint type;

	/* Get phone list and active phone port */
	phone_list = router_get_phone_list(profile);
	type = router_get_phone_port(profile);

	/* Traverse phone list and add each phone */
	for (; phone_list != NULL; phone_list = phone_list->next) {
		struct phone *phone = phone_list->data;

		if (type == phone->type) {
			name = g_strdup(phone->name);
			break;
		}
	}

	return name ? name : g_strdup(_("Phone"));
}

/**
 * \brief Phone status timer
 * \param data UNUSED
 * \return TRUE
 */
static gboolean phone_status_timer_cb(gpointer data)
{
	gchar *time_diff = connection_get_time(phone_state->connection);
	gchar buf[32];

	snprintf(buf, sizeof(buf), _("Time: %s"), time_diff);

	if (roger_uses_headerbar()) {
		gtk_header_bar_set_subtitle(GTK_HEADER_BAR(phone_state->header_bar), buf);
	} else {
		gchar *title = g_strdup_printf("%s - %s", phone_get_active_name(), buf);
		gtk_window_set_title(GTK_WINDOW(phone_state->window), title);
		g_free(title);
	}

	return TRUE;
}

/**
 * \brief Setup phone status timer
 */
void phone_setup_status_timer(void)
{
	phone_state->status_timer_id = g_timeout_add(250, phone_status_timer_cb, NULL);
}

/**
 * \brief Remove phone status timer
 */
void phone_remove_timer(void)
{
	/* Make sure that the status information is updated */
	phone_status_timer_cb(NULL);

	/* Remove timer source */
	if (phone_state->status_timer_id) {
		g_source_remove(phone_state->status_timer_id);
		phone_state->status_timer_id = 0;
	}
}

/**
 * \brief Set sensitive value of control buttons (type SOFTPHONE = true, else false)
 * \param state phone state pointer
 * \param status sensitive value
 */
static void phone_control_buttons_set_sensitive(gboolean sensitive)
{
	/* Set control button sensitive value */
	gtk_widget_set_sensitive(phone_state->mute_button, sensitive);
	gtk_widget_set_sensitive(phone_state->hold_button, sensitive);
	gtk_widget_set_sensitive(phone_state->record_button, sensitive);
}

/**
 * \brief Set sensitive value of hangup/pickup button depending of allow state
 * \param allow_dial flag indicating if pick up is allowed
 */
void phone_dial_buttons_set_dial(gboolean allow_dial)
{
	struct profile *profile = profile_get_active();

	gtk_widget_set_sensitive(phone_state->pickup_button, allow_dial);
	gtk_widget_set_sensitive(phone_state->hangup_button, !allow_dial);

	/* Toggle control buttons sensitive value */
	if (router_get_phone_port(profile) == PORT_SOFTPHONE) {
		phone_control_buttons_set_sensitive(!allow_dial);
	} else {
		phone_control_buttons_set_sensitive(FALSE);
	}
}

/**
 * \brief Connection established callback - Starts timer if needed
 * \param object appobject
 * \param connection connection pointer
 * \param user_data phone state pointer
 */
static void connection_established_cb(AppObject *object, struct connection *connection, gpointer user_data)
{
	if (!phone_state->status_timer_id) {
		phone_setup_status_timer();
	}
}

/**
 * \brief Connection terminated callback - Stops timer
 * \param object appobject
 * \param connection connection pointer
 * \param user_data phone state pointer
 */
static void connection_terminated_cb(AppObject *object, struct connection *connection, gpointer user_data)
{
	g_debug("%s(): connection = %p", __FUNCTION__, connection);

	if (!connection || !phone_state || !phone_state->connection || phone_state->connection->id != connection->id) {
		return;
	}

	/* Remove timer */
	phone_remove_timer();

	phone_dial_buttons_set_dial(TRUE);
	phone_state->connection = NULL;

	//TODO: Free connection
}

void connection_status_cb(AppObject *object, gint status, struct connection *connection, gpointer user_data)
{
	//phone_devices[state->type]->status(phone_state, connection);
}

/**
 * \brief Phone connection failed - Show error dialog including user support text
 */
static void phone_connection_failed(void)
{
	GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(phone_state->window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Connection problem"));
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
 */
static void phone_control_buttons_reset(void)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(phone_state->mute_button), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(phone_state->hold_button), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(phone_state->record_button), FALSE);
}

/**
 * \brief Pickup button clicked callback
 * \param button pickup button
 * \param user_data pointer to phone state structure
 */
void pickup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct profile *profile = profile_get_active();
	gchar *scramble;

	/* Get selected number (either number format or based on the selected name) */
	phone_state->number = gtk_entry_get_text(GTK_ENTRY(phone_state->search_entry));
	if (!EMPTY_STRING(phone_state->number) && !(isdigit(phone_state->number[0]) || phone_state->number[0] == '*' || phone_state->number[0] == '#' || phone_state->number[0] == '+')) {
		phone_state->number = g_object_get_data(G_OBJECT(phone_state->search_entry), "number");
	}

	if (EMPTY_STRING(phone_state->number)) {
		return;
	}

	scramble = call_scramble_number(phone_state->number);
	g_debug("%s(): Want to dial '%s'", __FUNCTION__, scramble);
	g_free(scramble);

	if (router_get_phone_port(profile) == PORT_SOFTPHONE) {
		if (phone_state->connection) {
			phone_active_call_dialog(phone_state->window);
			return;
		}
		phone_state->connection = phone_dial(phone_state->number, router_get_suppress_state(profile));

		if (!phone_state->connection) {
			phone_connection_failed();
		} else {
			phone_dial_buttons_set_dial(FALSE);
		}
	} else {
		gchar *number;

		number = g_strdup_printf("%s%s", router_get_suppress_state(profile) ? "*31#" : "", phone_state->number);
		if (router_dial_number(profile, router_get_phone_port(profile), number)) {
			phone_dial_buttons_set_dial(FALSE);
		}
		g_free(number);
	}
}

/**
 * \brief Hangup button clicked callback
 * \param button hangup button
 * \param user_data pointer to phone phone_state structure
 */
void hangup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct profile *profile = profile_get_active();
	const gchar *number = gtk_entry_get_text(GTK_ENTRY(phone_state->search_entry));

	if (phone_state->type == PHONE_TYPE_FAX || router_get_phone_port(profile) == PORT_SOFTPHONE) {
		if (!phone_state->connection) {
			return;
		}

		g_debug("%s(): Hangup requested for %p", __FUNCTION__, phone_state->connection);
		phone_control_buttons_reset();
		phone_hangup(phone_state->connection);
		phone_dial_buttons_set_dial(TRUE);
	} else {
		if (router_hangup(profile, router_get_phone_port(profile), number)) {
			phone_dial_buttons_set_dial(TRUE);
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
		tmp = g_strdup(_("Mobile"));
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
void phone_search_entry_search_changed_cb(GtkSearchEntry *entry, gpointer user_data)
{
	GtkWidget *label;
	GSList *contacts = NULL;
	GSList *list;

	/* Get current filter text */
	phone_state->filter = gtk_entry_get_text(GTK_ENTRY(entry));

	/* If it is an invalid filter, abort and close menu if needed */
	if (EMPTY_STRING(phone_state->filter) || isdigit(phone_state->filter[0]) || phone_state->discard || phone_state->filter[0] == '*' || phone_state->filter[0] == '#') {
		phone_state->discard = FALSE;
		if (phone_state->contact_menu) {
			gtk_widget_destroy(phone_state->contact_menu);
		}
		return;
	}

	/* if needed, create contact menu */
	if (!phone_state->contact_menu) {
		GtkWidget *placeholder;
		GtkWidget *image;
		GtkWidget *text;
		gint width, height;

		/* Create popover */
		phone_state->contact_menu = gtk_popover_new(NULL);
		gtk_popover_set_modal(GTK_POPOVER(phone_state->contact_menu), FALSE);
		gtk_popover_set_position(GTK_POPOVER(phone_state->contact_menu), GTK_POS_BOTTOM);
		gtk_popover_set_relative_to(GTK_POPOVER(phone_state->contact_menu), GTK_WIDGET(entry));

		width = gtk_widget_get_allocated_width(phone_state->grid);
		height = gtk_widget_get_allocated_height(phone_state->child_frame);

		gtk_widget_set_size_request(phone_state->contact_menu, width, height);

		g_signal_connect(G_OBJECT(phone_state->contact_menu), "closed", G_CALLBACK(phone_contact_menu_closed_cb), phone_state);

		phone_state->scrolled_win = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(phone_state->contact_menu), phone_state->scrolled_win);

		phone_state->box = gtk_list_box_new();

		placeholder = gtk_grid_new();
		gtk_widget_set_sensitive(placeholder, FALSE);
		gtk_widget_set_halign(placeholder, GTK_ALIGN_CENTER);
		gtk_widget_set_valign(placeholder, GTK_ALIGN_CENTER);

		image = gtk_image_new_from_icon_name(AVATAR_DEFAULT, GTK_ICON_SIZE_DIALOG);
		gtk_grid_attach(GTK_GRID(placeholder), image, 0, 0, 1, 1);

		text = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(text), _("<b>No contact found</b>"));
		gtk_widget_set_sensitive(text, FALSE);
		gtk_grid_attach(GTK_GRID(placeholder), text, 0, 1, 1, 1);

		gtk_widget_show_all(placeholder);
		gtk_list_box_set_placeholder(GTK_LIST_BOX(phone_state->box), placeholder);
		gtk_list_box_set_filter_func(GTK_LIST_BOX(phone_state->box), phone_search_entry_filter_cb, phone_state, NULL);
		gtk_container_add(GTK_CONTAINER(phone_state->scrolled_win), phone_state->box);
		g_signal_connect(G_OBJECT(phone_state->box), "row-activated", G_CALLBACK(phone_list_box_activated_cb), phone_state);
		gtk_widget_show_all(phone_state->contact_menu);
	}

	/* Clean previous contacts */
	gtk_container_foreach(GTK_CONTAINER(phone_state->box), phone_destroy_contacts, NULL);

	phone_state->filter = gtk_entry_get_text(GTK_ENTRY(entry));

	/* Add contacts to entry completion */
	contacts = address_book_get_contacts();

	for (list = contacts; list; list = list->next) {
		struct contact *contact = list->data;
		GSList *numbers;
		GtkWidget *num;
		GtkWidget *grid;
		gchar *name;

		if (!g_strcasestr(contact->name, phone_state->filter) || !contact->numbers) {
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

			gtk_list_box_insert(GTK_LIST_BOX(phone_state->box), grid, -1);
		}

		g_free(name);
	}
}

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

/**
 * \brief icon of search entry pressed
 * \param entry search entry widget
 * \param icon_pos icon position
 * \param event active event
 * \param user_data pointer to phone_state structure
 */
void phone_search_entry_icon_press_cb(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
	if (icon_pos == GTK_ENTRY_ICON_PRIMARY) {
		/* If primary icon has been pressed, toggle menu */
		if (!phone_state->contact_menu) {
			phone_search_entry_search_changed_cb(GTK_SEARCH_ENTRY(entry), user_data);
		} else {
			gtk_widget_destroy(phone_state->contact_menu);
		}
	} else if (icon_pos == GTK_ENTRY_ICON_SECONDARY) {
		g_object_set_data(G_OBJECT(phone_state->search_entry), "number", NULL);
	}
}

/**
 * \brief Handle cursor up/down in name entry field which controls the open popover menu (listbox) without losing focus
 * \param entry entry widget
 * \param event active event
 * \param user_data UNUSED
 * \return TRUE if should be propagated, otherwise FALSE
 */
gboolean phone_search_entry_key_press_event_cb(GtkWidget *entry, GdkEvent *event, gpointer user_data)
{
	GtkListBoxRow *row = NULL;
	GList *childs;
	guint keyval = ((GdkEventKey *)event)->keyval;
	gint length;

	if (!phone_state->box) {
		return FALSE;
	}

	/* Escape pressed, unselect all and close menu */
	if (keyval == GDK_KEY_Escape) {
		gtk_list_box_unselect_all(GTK_LIST_BOX(phone_state->box));
		gtk_widget_destroy(phone_state->contact_menu);
		return TRUE;
	}

	/* Get selected row */
	row = gtk_list_box_get_selected_row(GTK_LIST_BOX(phone_state->box));
	if (!row) {
		row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(phone_state->box), 0);

		if (!row) {
			return FALSE;
		}
	}

	/* If we have a selected row and return/enter is pressed, set number and exit */
	if (gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row)) && (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter || keyval == GDK_KEY_ISO_Enter)) {
		phone_search_entry_set_contact_by_row(phone_state, row);

		return TRUE;
	}

	/* Get length of listbox childs */
	childs = gtk_container_get_children(GTK_CONTAINER(phone_state->box));
	g_assert(childs != NULL);

	length = g_list_length(childs);

	if (keyval == GDK_KEY_Up) {
		/* Handle key up */
		if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(phone_state->box), length - 1);
			gtk_list_box_select_row(GTK_LIST_BOX(phone_state->box), GTK_LIST_BOX_ROW(row));
			phone_list_box_set_focus(phone_state->scrolled_win, phone_state->box, row);
			return FALSE;
		}

		if (gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) > 0) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(phone_state->box), gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) - 1);
			gtk_list_box_select_row(GTK_LIST_BOX(phone_state->box), GTK_LIST_BOX_ROW(row));
		} else {
			gtk_list_box_unselect_all(GTK_LIST_BOX(phone_state->box));
		}
	} else if (keyval == GDK_KEY_Down) {
		/* Handle key down */
		if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
			gtk_list_box_select_row(GTK_LIST_BOX(phone_state->box), GTK_LIST_BOX_ROW(row));
			phone_list_box_set_focus(phone_state->scrolled_win, phone_state->box, row);
			return FALSE;
		}

		if (gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) < (length - 1)) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(phone_state->box), gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) + 1);
			gtk_list_box_select_row(GTK_LIST_BOX(phone_state->box), GTK_LIST_BOX_ROW(row));

		} else {
			gtk_list_box_unselect_all(GTK_LIST_BOX(phone_state->box));
		}
	}

	phone_list_box_set_focus(phone_state->scrolled_win, phone_state->box, row);

	return FALSE;
}

/**
 * \brief Phone toggled callback - sets selected port type to profile settings
 * \param item percieved check menu item
 * \param user_data containing pointer to corresponding phone type
 */
void phone_item_toggled_cb(GtkCheckMenuItem *item, gpointer user_data)
{
	/* If item is active, adjust phone port accordingly and set sensitivity of phone buttons */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item))) {
		gint type = GPOINTER_TO_INT(user_data);
		gchar *name;

		/* Set active phone port */
		router_set_phone_port(profile_get_active(), type);
		name = phone_get_active_name();
		gtk_window_set_title(GTK_WINDOW(phone_state->window), name);
		g_free(name);
	}
}

/**
 * \brief Create phone window menu which contains phone selection and suppress number toggle
 * \param profile pointer to current profile
 * \return newly create phone menu
 */
static GtkWidget *phone_window_create_menu(struct profile *profile)
{
	GtkWidget *menu;
	GtkWidget *item;
	GSList *phone_list = NULL;
	struct phone *phone = NULL;
	gint type = 0;

	GtkWidget *box;
	GSList *phone_radio_list = NULL;

	/* Create vertical box */
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin(box, 6, 6, 6, 6);

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
		g_object_set_data(G_OBJECT(item), "phone_state", phone_state);

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

	return menu;
}

GtkWidget *phone_window_new(enum phone_type type, struct contact *contact, struct connection *connection, gpointer priv)
{
	g_warning("FIXME");
	return NULL;
}

/**
 * \brief Dialpad button pressed - send either dtmf code or add number to entry
 * \param widget button widget
 * \param user_data phone state pointer
 */
void phone_dtmf_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	const gchar *name = gtk_widget_get_name(widget);
	gint num = name[7];

	if (phone_state->connection) {
		/* Active connection is available, send dtmf code */
		phone_send_dtmf_code(phone_state->connection, num);
	} else {
		/* Add number to entry */
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(phone_state->search_entry));
		gchar *tmp = g_strdup_printf("%s%c", text, num);

		gtk_entry_set_text(GTK_ENTRY(phone_state->search_entry), tmp);
		g_free(tmp);
	}
}

/**
 * \brief Record button clicked callback - toggle record call state
 * \param widget button widget
 * \param user_data phone state pointer
 */
void phone_record_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	const gchar *user_plugins;
	gchar *path;

	/* Create roger record directory */
	user_plugins = g_get_user_data_dir();
	path = g_build_filename(user_plugins, "roger", G_DIR_SEPARATOR_S, NULL);
	g_mkdir_with_parents(path, 0700);

	/* Toggle record state */
	g_warning("%s(): TODO", __FUNCTION__);
	//phone_record(state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)), path);

	g_free(path);
}

/**
 * \brief Hold button clicked callback - toggle hold call state
 * \param widget button widget
 * \param user_data phone state pointer
 */
void phone_hold_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	/* Toggle hold call */
	phone_hold(phone_state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/**
 * \brief Mute button clicked callback - toggle mute call state
 * \param widget button widget
 * \param user_data phone state pointer
 */
void phone_mute_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	/* Toggle mute call */
	phone_mute(phone_state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}


/**
 * \brief Clear button clicked callback - removes last char of number/name
 * \param widget button widget
 * \param user_data phone state pointer
 */
void phone_clear_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(phone_state->search_entry));

	/* If there is text within the entry, remove last char */
	if (!EMPTY_STRING(text)) {
		gchar *new = g_strdup(text);

		new[strlen(text) - 1] = '\0';
		gtk_entry_set_text(GTK_ENTRY(phone_state->search_entry), new);
		g_free(new);
	}
}

/**
 * \brief Pick up incoming connection
 * \param connection incoming connection
 */
static void phone_pickup_incoming(struct connection *connection)
{
	/* Pick up */
	if (!phone_pickup(connection)) {
		/* Set internal connection */
		phone_state->connection = connection;

		/* Disable dial button */
		phone_dial_buttons_set_dial(FALSE);
	}
}

/**
 * \brief Phone window delete event callback
 * \param widget window widget
 * \param event event structure
 * \param data pointer to phone_state structure
 * \return FALSE if window should be deleted, otherwise TRUE
 */
gboolean phone_window_delete_event_cb(GtkWidget *window, GdkEvent *event, gpointer data)
{
	/* If there is an active connection, warn user and refuse to delete the window */
	if (phone_state && phone_state->connection) {
		phone_active_call_dialog(window);

		/* Keep window */
		return TRUE;
	}

	/* Disconnect all signal handlers */
	g_signal_handlers_disconnect_by_data(app_object, phone_state);

	phone_state->window = NULL;

	/* Free phone state structure and release phone window */
	g_slice_free(struct phone_state, phone_state);

	phone_state = NULL;

	return FALSE;
}


void app_show_phone_window(struct contact *contact, struct connection *connection)
{
	GtkBuilder *builder;

	/* If there is already an open phone window, present it to the user and return */
	if (phone_state) {
		/* Incoming call? */
		if (!phone_state->connection && connection) {
			phone_pickup_incoming(connection);

			/* Set contact name */
			phone_search_entry_set_contact(phone_state, contact, TRUE);
		}

		gtk_window_present(GTK_WINDOW(phone_state->window));
		return;
	}

	builder = gtk_builder_new_from_resource("/org/tabos/roger/phone.glade");
	if (!builder) {
		g_warning("Could not load phone ui");
		return;
	}

	/* Allocate phone state structure */
	phone_state = g_slice_alloc0(sizeof(struct phone_state));

	/* Connect to builder objects */
	phone_state->window = GTK_WIDGET(gtk_builder_get_object(builder, "phone_window"));
	gtk_window_set_transient_for(GTK_WINDOW(phone_state->window), GTK_WINDOW(journal_get_window()));

	phone_state->grid = GTK_WIDGET(gtk_builder_get_object(builder, "phone_grid"));

	phone_state->search_entry = GTK_WIDGET(gtk_builder_get_object(builder, "call_search_entry"));

	phone_state->pickup_button = GTK_WIDGET(gtk_builder_get_object(builder, "call_start_button"));
	phone_state->hangup_button = GTK_WIDGET(gtk_builder_get_object(builder, "call_stop_button"));

	phone_state->mute_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_mute_button"));
	phone_state->hold_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_hold_button"));
	phone_state->record_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_record_button"));

	phone_state->child_frame = GTK_WIDGET(gtk_builder_get_object(builder, "child_frame"));

	phone_state->menu_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_menu_button"));

	GtkWidget *menu = phone_window_create_menu(profile_get_active());
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(phone_state->menu_button), menu);

	phone_state->header_bar = GTK_WIDGET(gtk_builder_get_object(builder, "phone_headerbar"));

	if (roger_uses_headerbar()) {
		/* Create header bar and set it to window */
		gtk_header_bar_set_title(GTK_HEADER_BAR(phone_state->header_bar), phone_get_active_name());
		gtk_window_set_titlebar(GTK_WINDOW(phone_state->window), phone_state->header_bar);
	} else {
		GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(builder, "phone_grid"));

		gtk_window_set_title(GTK_WINDOW(phone_state->window), phone_get_active_name());
		gtk_grid_attach(GTK_GRID(grid), phone_state->header_bar, 0, 0, 3, 1);
	}

	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	/* Connect connection signals */
	g_signal_connect(app_object, "connection-established", G_CALLBACK(connection_established_cb), NULL);
	g_signal_connect(app_object, "connection-terminated", G_CALLBACK(connection_terminated_cb), NULL);
	g_signal_connect(app_object, "connection-status", G_CALLBACK(connection_status_cb), NULL);

	/* In case there is an incoming connection, pick it up */
	if (connection) {
		phone_pickup_incoming(connection);
	} else {
		/* No incoming call, allow dialing */
		phone_dial_buttons_set_dial(TRUE);
	}

	gtk_widget_show_all(phone_state->window);
}
