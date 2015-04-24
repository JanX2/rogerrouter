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

/** Phone window */
static GtkWidget *phone_window = NULL;

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

static gboolean phone_session_timer_cb(gpointer data)
{
	struct phone_state *state = data;
	gchar *time_diff = timediff_str(state);
	gchar buf[32];

	snprintf(buf, sizeof(buf), _("Time: %s"), time_diff);

	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(state->headerbar), buf);

	if (state->connection) {
		/* Flush recording buffer (if needed) */
		phone_flush(state->connection);
	}

	return TRUE;
}

void phone_setup_timer(struct phone_state *state)
{
	if (state->phone_session_timer && state->phone_session_timer_id) {
		g_debug("%s(): Skip timer setup", __FUNCTION__);
		return;
	}

	g_debug("%s(): called", __FUNCTION__);
	state->phone_session_timer = g_timer_new();
	g_timer_start(state->phone_session_timer);
	state->phone_session_timer_id = g_timeout_add(250, phone_session_timer_cb, state);
}

void phone_remove_timer(struct phone_state *state)
{
	/* Make sure that the status bar information is updated */
	phone_session_timer_cb(state);

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

void phone_allow_dial(struct phone_state *state, gboolean allow)
{
	gtk_widget_set_sensitive(state->pickup_button, allow);
	gtk_widget_set_sensitive(state->hangup_button, !allow);
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

	/* Remove timer */
	phone_remove_timer(state);

	phone_allow_dial(state, TRUE);
	state->connection = NULL;
}

void phone_connection_notify_cb(AppObject *object, struct connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;

	g_debug("%s(): called", __FUNCTION__);

	if (connection->type == CONNECTION_TYPE_OUTGOING) {
		g_debug("Outgoing");
		if (!state->connection && state->number && !strcmp(state->number, connection->remote_number)) {
			phone_setup_timer(state);
		}
	} else if (connection->type == (CONNECTION_TYPE_OUTGOING | CONNECTION_TYPE_CONNECT)) {
		g_debug("Connect, check %p == %p?", state->connection, connection);
		if (state->connection != connection) {
			return;
		}

		g_debug("Established");
	} else {
		if (connection->type & CONNECTION_TYPE_DISCONNECT) {
			g_debug("Disconnect, check %p == %p?", state->connection, connection);
			if (state->connection && state->connection != connection) {
				return;
			}

			g_debug("Terminated");
			phone_remove_timer(state);

			state->connection = NULL;
			state->number = NULL;
		}
	}
}

/**
 * \brief Phone connection failed - Show error dialog including user support text
 */
static void phone_connection_failed(void)
{
	GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(phone_window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Connection problem"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(error_dialog), _("This is most likely due to a closed CAPI port or an invalid phone number.\n\nSolution: Dial #96*3* with your phone and check your numbers within the settings"));

	gtk_dialog_run(GTK_DIALOG(error_dialog));
	gtk_widget_destroy(error_dialog);
}

static void show_active_call_dialog(GtkWidget *window)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, _("A call is in progress, hangup first"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void phone_reset_buttons(struct phone_state *state)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(state->mute_button))) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(state->mute_button), FALSE);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(state->hold_button))) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(state->hold_button), FALSE);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(state->record_button))) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(state->record_button), FALSE);
	}
}

static void pickup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct profile *profile = profile_get_active();
	struct phone_state *state = user_data;

	/* Get selected number (either number format or based on the selected name) */
	state->number = gtk_entry_get_text(GTK_ENTRY(state->name_entry));
	if (EMPTY_STRING(state->number) || !(isdigit(state->number[0]) || state->number[0] == '*' || state->number[0] == '#' || state->number[0] == '+')) {
		state->number = g_object_get_data(G_OBJECT(state->name_entry), "number");
	}

	if (EMPTY_STRING(state->number)) {
		return;
	}

	g_debug("%s(): Want to dial '%s'", __FUNCTION__, call_scramble_number(state->number));

	switch (state->type) {
	case PHONE_TYPE_VOICE:
		if (router_get_phone_port(profile) == PORT_SOFTPHONE) {
			if (state->connection) {
				show_active_call_dialog(phone_window);
				return;
			}
			state->connection = phone_dial(state->number, router_get_suppress_state(profile));

			if (!state->connection) {
				phone_connection_failed();
			} else {
				phone_allow_dial(state, FALSE);
			}
		} else {
			gchar *number;

			number = g_strdup_printf("%s%s", router_get_suppress_state(profile) ? "*31#" : "", state->number);
			router_dial_number(profile, router_get_phone_port(profile), number);
			g_free(number);
		}
		break;
	case PHONE_TYPE_FAX: {
		struct fax_ui *fax_ui = state->priv;

		state->connection = fax_dial(fax_ui->file, state->number, router_get_suppress_state(profile));
		if (state->connection) {
			fax_window_clear(fax_ui);
			phone_allow_dial(state, FALSE);
		} else {
			phone_connection_failed();
		}
		break;
	}
	default:
		g_warning("%s(): Unknown type selected", __FUNCTION__);
		break;
	}
}

static void hangup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct profile *profile = profile_get_active();
	struct phone_state *state = user_data;
	const gchar *number = gtk_entry_get_text(GTK_ENTRY(state->name_entry));

	if (state->type == PHONE_TYPE_FAX || router_get_phone_port(profile) == PORT_SOFTPHONE) {
		if (!state->connection) {
			return;
		}

		g_debug("%s(): Hangup requested for %p", __FUNCTION__, state->connection);
		phone_reset_buttons(state);
		phone_hangup(state->connection);
		phone_allow_dial(state, TRUE);
	} else {
		router_hangup(profile, router_get_phone_port(profile), number);
	}
}

void menu_set_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	GtkAllocation allocation;
	GtkRequisition request;

	gtk_widget_get_preferred_size(gtk_widget_get_toplevel(GTK_WIDGET(menu)), NULL, &request);

	gdk_window_get_origin(gtk_widget_get_window(user_data), x, y);

	gtk_widget_get_allocation(user_data, &allocation);
	*x += allocation.x;
	*y += allocation.y + allocation.height;

	if (request.width < allocation.width) {
		gtk_widget_set_size_request(GTK_WIDGET(menu), allocation.width, -1);
	}

	*push_in = TRUE;
}

static void phone_set_dial_number(GtkMenuItem *item, gpointer user_data)
{
	GtkWidget *entry = g_object_get_data(G_OBJECT(item), "entry");
	GtkWidget *menu = g_object_get_data(G_OBJECT(item), "menu");
	gchar *name = g_object_get_data(G_OBJECT(item), "name");

	g_object_set_data(G_OBJECT(entry), "number", user_data);

	if (!EMPTY_STRING(name)) {
		gtk_entry_set_text(GTK_ENTRY(entry), name);
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "go-down-symbolic");
	} else {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, NULL);
	}

	if (menu) {
		gtk_widget_destroy(menu);
	}
}

#define OLD_MENU 1
static void contact_number_menu(GtkWidget *entry, struct contact *contact)
{
	GtkWidget *menu;
	GSList *list;
	GtkWidget *item;
	gchar *tmp;
	gchar *prev_number = g_object_get_data(G_OBJECT(entry), "number");

#ifdef OLD_MENU
	menu = gtk_menu_new();
#else
	GtkWidget *box;

	menu = gtk_popover_new(entry);
	gtk_popover_set_position(GTK_POPOVER(menu), GTK_POS_BOTTOM);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin(box, 12, 12, 12, 12);
	gtk_container_add(GTK_CONTAINER(menu), box);
#endif

	g_object_set_data(G_OBJECT(entry), "contact", contact);

	gtk_entry_set_text(GTK_ENTRY(entry), contact->name);
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "go-down-symbolic");

	for (list = contact->numbers; list; list = list->next) {
		struct phone_number *number = list->data;

		switch (number->type) {
		case PHONE_NUMBER_HOME:
			tmp = g_strdup_printf("%s (%s)", _("HOME"), number->number);
			break;
		case PHONE_NUMBER_WORK:
			tmp = g_strdup_printf("%s (%s)", _("WORK"), number->number);
			break;
		case PHONE_NUMBER_MOBILE:
			tmp = g_strdup_printf("%s (%s)", _("MOBILE"), number->number);
			break;
		case PHONE_NUMBER_FAX_HOME:
			tmp = g_strdup_printf("%s (%s)", _("HOME FAX"), number->number);
			break;
		case PHONE_NUMBER_FAX_WORK:
			tmp = g_strdup_printf("%s (%s)", _("WORK FAX"), number->number);
			break;
		default:
			tmp = g_strdup_printf("%s (%s)", _("UNKNOWN"), number->number);
			break;
		}

#ifdef OLD_MENU
		item = gtk_check_menu_item_new_with_label(tmp);

		if (!EMPTY_STRING(prev_number) && !strcmp(prev_number, number->number)) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		}

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_object_set_data(G_OBJECT(item), "entry", entry);
		g_object_set_data(G_OBJECT(item), "name", contact->name);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(phone_set_dial_number), number->number);
#else
		item = gtk_check_button_new_with_label(tmp);

		if (!EMPTY_STRING(prev_number) && !strcmp(prev_number, number->number)) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item), TRUE);
		}
		gtk_box_pack_end(GTK_BOX(box), item, TRUE, TRUE, 0);

		g_object_set_data(G_OBJECT(item), "entry", entry);
		g_object_set_data(G_OBJECT(item), "name", contact->name);
		g_object_set_data(G_OBJECT(item), "menu", menu);
		g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(phone_set_dial_number), number->number);
#endif
		g_free(tmp);
	}

	gtk_widget_show_all(menu);

#ifdef OLD_MENU
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, menu_set_position, entry, 0, gtk_get_current_event_time());
#endif
}

void name_entry_icon_press_cb(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
	struct contact *contact = g_object_get_data(G_OBJECT(entry), "contact");

	contact_number_menu(GTK_WIDGET(entry), contact);
}

/**
 * \brief Number entry match selected callback
 * \param completion entry completion
 * \param model tree model
 * \param iter current tree iterator
 * \param user_data phone state pointer
 * \return TRUE if signal has been handled
 */
gboolean number_entry_match_selected_cb(GtkEntryCompletion *completion, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GValue value_contact = {0, };
	struct phone_state *state = user_data;
	GtkWidget *entry = state->name_entry;
	struct contact *contact;

	gtk_tree_model_get_value(model, iter, 1, &value_contact);
	contact = g_value_get_pointer(&value_contact);

	if (!contact->numbers) {
		return FALSE;
	}

	contact_number_menu(entry, contact);

	return TRUE;
}

gboolean name_entry_filter_cb(GtkListBoxRow *row, gpointer user_data)
{
	struct phone_state *state = user_data;
	GtkWidget *grid = gtk_bin_get_child(GTK_BIN(row));
	struct contact *contact = g_object_get_data(G_OBJECT(grid), "contact");

	if (EMPTY_STRING(state->filter)) {
		return TRUE;
	}

	return g_strcasestr(contact->name, state->filter) != NULL;
}

void menu_closed_cb(GtkWidget *menu, gpointer user_data)
{
	struct phone_state *state = user_data;

	state->menu = NULL;
	state->box = NULL;
}

void phone_destroy_contacts(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
}

gchar *number_type_to_string(enum phone_number_type type)
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

void name_box_set_focus_child_cb(GtkContainer *container, GtkWidget *widget, gpointer user_data)
{
	g_debug("MOEP");
}

void number_entry_search_changed_cb(GtkSearchEntry *entry, gpointer user_data)
{
	GtkWidget *label;
	struct phone_state *state = user_data;
	GSList *contacts = NULL;
	GSList *list;

	state->filter = gtk_entry_get_text(GTK_ENTRY(entry));

	if (EMPTY_STRING(state->filter) || isdigit(state->filter[0]) || state->discard || state->filter[0] == '*' || state->filter[0] == '#') {
		state->discard = FALSE;
		if (state->menu) {
			gtk_widget_destroy(state->menu);
		}
		return;
	}

	if (!state->menu) {
		/* Create popover */
		state->menu = gtk_popover_new(NULL);
		//gtk_widget_set_can_focus(state->menu, FALSE);
		gtk_popover_set_modal(GTK_POPOVER(state->menu), FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(state->menu), 6);
		gtk_popover_set_position(GTK_POPOVER(state->menu), GTK_POS_BOTTOM);
		gtk_popover_set_relative_to(GTK_POPOVER(state->menu), GTK_WIDGET(entry));

		state->scrolled_win = gtk_scrolled_window_new(NULL, NULL);
		//gtk_widget_set_can_focus(win, FALSE);
		gtk_widget_set_size_request(state->menu, 400, 280);
		gtk_container_add(GTK_CONTAINER(state->menu), state->scrolled_win);

		state->box = gtk_list_box_new();
		//gtk_widget_set_can_focus(state->box, FALSE);
		gtk_list_box_set_filter_func(GTK_LIST_BOX(state->box), name_entry_filter_cb, state, NULL);
		gtk_container_add(GTK_CONTAINER(state->scrolled_win), state->box);
		gtk_widget_show_all(state->menu);
		g_signal_connect(G_OBJECT(state->menu), "closed", G_CALLBACK(menu_closed_cb), state);
	}

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
			GdkPixbuf *buf = image_get_scaled(contact ? contact->image : NULL, 32, 32);
			GtkWidget *image;
			gchar *num_str;
			gchar *type;

			grid = gtk_grid_new();

			/* Set standard spacing */
			gtk_grid_set_row_spacing(GTK_GRID(grid), 2);
			gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

			g_object_set_data(G_OBJECT(grid), "contact", contact);
			g_object_set_data(G_OBJECT(grid), "number", number->number);

			image = gtk_image_new_from_pixbuf(buf);
			gtk_grid_attach(GTK_GRID(grid), image, 0, 0, 1, 2);

			label = gtk_label_new(NULL);
			gtk_label_set_markup(GTK_LABEL(label), name);
			gtk_widget_set_halign(label, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(grid), label, 1, 0, 1, 1);

			type = number_type_to_string(number->type);
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

/**
 * \brief Check if entry contains text
 * \param completion entry completion
 * \param norm_key unused
 * \param iter current tree iterator
 * \param user_data unused
 * \return TRUE if it is part of the entry, otherwise FALSE
 */
gboolean number_entry_contains_completion_cb(GtkEntryCompletion *completion, const gchar *norm_key, GtkTreeIter *iter, gpointer user_data)
{
	GtkTreeModel *model = gtk_entry_completion_get_model(completion);
	const gchar *key = gtk_entry_get_text(GTK_ENTRY(gtk_entry_completion_get_entry(completion)));
	gchar *name;
	gboolean found;

	gtk_tree_model_get(model, iter, 0, &name, -1);
	found = g_strcasestr(name, key) != 0;
	g_free(name);

	return found;
}

/**
 * \brief Set contact name in phone dialog
 * \param state phone state structure
 * \param contact contact structure
 */
void phone_set_contact_name(struct phone_state *state, struct contact *contact)
{
	struct contact *contact_copy;

	if (!state || !contact) {
		return;
	}

	contact_copy = contact_dup(contact);

	emit_contact_process(contact_copy);

	if (!EMPTY_STRING(contact_copy->name)) {
		gtk_entry_set_text(GTK_ENTRY(state->name_entry), contact_copy->name);
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(state->name_entry), GTK_ENTRY_ICON_SECONDARY, "go-down-symbolic");
		g_object_set_data(G_OBJECT(state->name_entry), "contact", contact_copy);
		g_object_set_data(G_OBJECT(state->name_entry), "number", contact_copy->number);
	} else if (contact_copy->number) {
		gtk_entry_set_text(GTK_ENTRY(state->name_entry), contact_copy->number);
		g_object_set_data(G_OBJECT(state->name_entry), "number", contact_copy->number);
	}
}

void listbox_set_focus(GtkWidget *scrolled_win, GtkWidget *box, GtkListBoxRow *row)
{
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_win));
	gint wleft, wtop, wbottom;
	gdouble top, bottom, value;

	gtk_widget_translate_coordinates(GTK_WIDGET(row), box, 0, 0, &wleft, &wtop);
	wbottom = wtop + gtk_widget_get_allocated_height(GTK_WIDGET(row));

	value = top = gtk_adjustment_get_value(vadj);
	bottom = top + gtk_adjustment_get_page_size(vadj);

	if (wtop < top) {
		value = wtop;
	} else if (wbottom > bottom) {
		value = wbottom - gtk_adjustment_get_page_size(vadj);
	}

	gtk_adjustment_set_value(vadj, value);
}

gboolean number_entry_key_press_event_cb(GtkWidget *entry, GdkEvent *event, gpointer user_data)
{
	GtkListBoxRow *row;
	struct phone_state *state = user_data;
	GList *childs;
	guint keyval = ((GdkEventKey *)event)->keyval;
	gint length;

	if (!state->box) {
		return FALSE;
	}

	row = gtk_list_box_get_selected_row(GTK_LIST_BOX(state->box));
	if (!row) {
		row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->box), 0);
	}

	if (!row) {
		return FALSE;
	}

	if (keyval == GDK_KEY_Escape) {
		gtk_list_box_unselect_all(GTK_LIST_BOX(state->box));
		gtk_widget_destroy(state->menu);
		return TRUE;
	}

	if (gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
		GtkWidget *grid = gtk_bin_get_child(GTK_BIN(row));
		struct contact *contact;

		if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter || keyval == GDK_KEY_ISO_Enter) {
			gchar *number = g_object_get_data(G_OBJECT(grid), "number");

			g_object_set_data(G_OBJECT(entry), "number", number);

			contact = g_object_get_data(G_OBJECT(grid), "contact");
			state->discard = TRUE;
			gtk_entry_set_text(GTK_ENTRY(entry), contact->name);

			gtk_widget_destroy(state->menu);

			return TRUE;
		}
	}

	childs = gtk_container_get_children(GTK_CONTAINER(state->box));
	g_assert(childs != NULL);

	length = g_list_length(childs);

	if (keyval == GDK_KEY_Up) {
		if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->box), length - 1);
			gtk_list_box_select_row(GTK_LIST_BOX(state->box), GTK_LIST_BOX_ROW(row));
			return FALSE;
		}

		if (gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) > 0) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->box), gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) - 1);
			gtk_list_box_select_row(GTK_LIST_BOX(state->box), GTK_LIST_BOX_ROW(row));
		} else {
			gtk_list_box_unselect_all(GTK_LIST_BOX(state->box));
		}
	} else if (keyval == GDK_KEY_Down) {
		if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
			gtk_list_box_select_row(GTK_LIST_BOX(state->box), GTK_LIST_BOX_ROW(row));
			return FALSE;
		}

		if (gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) < (length - 1)) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(state->box), gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) + 1);
			gtk_list_box_select_row(GTK_LIST_BOX(state->box), GTK_LIST_BOX_ROW(row));

		} else {
			gtk_list_box_unselect_all(GTK_LIST_BOX(state->box));
		}
	}

	listbox_set_focus(state->scrolled_win, state->box, row);

	return FALSE;
}

/**
 * \brief Create name (number) frame
 * \param window phone window
 * \param contact contact structure (fill name entry if present)
 * \param state phone state pointer
 * \return name frame
 */
GtkWidget *phone_create_name_frame(GtkWidget *window, struct contact *contact, struct phone_state *state)
{
	state->name_entry = gtk_search_entry_new();
	gtk_widget_set_tooltip_text(state->name_entry, _("Use autocompletion while typing names from your address book"));
	gtk_entry_set_placeholder_text(GTK_ENTRY(state->name_entry), _("Enter name or number"));
	gtk_widget_set_hexpand(state->name_entry, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(state->name_entry), TRUE);
	g_signal_connect(state->name_entry, "search-changed", G_CALLBACK(number_entry_search_changed_cb), state);
	g_signal_connect(state->name_entry, "key-press-event", G_CALLBACK(number_entry_key_press_event_cb), state);

	/* Set contact name */
	phone_set_contact_name(state, contact);

	return state->name_entry;
}

/**
 * \brief Create dial button frame (pickup/hangup)
 * \param window window widget in order to set the dedault action
 * \param state phone state pointer
 * \return dial button frame
 */
GtkWidget *phone_dial_button_frame(GtkWidget *window, struct phone_state *state)
{
	GtkWidget *dial_frame_grid;
	GtkWidget *image;

	dial_frame_grid = gtk_grid_new();
	gtk_widget_set_can_focus(dial_frame_grid, FALSE);

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(dial_frame_grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(dial_frame_grid), 12);

	/* Pickup button */
	state->pickup_button = gtk_button_new();
	image = get_icon(APP_ICON_CALL, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(state->pickup_button), image);
	gtk_style_context_add_class(gtk_widget_get_style_context(state->pickup_button), GTK_STYLE_CLASS_SUGGESTED_ACTION);

	gtk_widget_set_tooltip_text(state->pickup_button, _("Pick up"));
	g_signal_connect(state->pickup_button, "clicked", G_CALLBACK(pickup_button_clicked_cb), state);
	gtk_widget_set_can_default(state->pickup_button, TRUE);
	gtk_window_set_default(GTK_WINDOW(window), state->pickup_button);
	gtk_grid_attach(GTK_GRID(dial_frame_grid), state->pickup_button, 0, 0, 1, 1);

	/* Hangup button */
	state->hangup_button = gtk_button_new();
	image = get_icon(APP_ICON_HANGUP, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(state->hangup_button), image);
	gtk_style_context_add_class(gtk_widget_get_style_context(state->hangup_button), GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);

	gtk_widget_set_tooltip_text(state->hangup_button, _("Hang up"));
	g_signal_connect(state->hangup_button, "clicked", G_CALLBACK(hangup_button_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(dial_frame_grid), state->hangup_button, 1, 0, 1, 1);

	return dial_frame_grid;
}

/**
 * \brief Create dial pad/control button
 * \param text_hi text high
 * \param text_lo text low
 * \param icon button icon
 * \return button widget
 */
static inline GtkWidget *phone_create_button(const gchar *text_hi, const gchar *text_lo, GtkWidget *icon)
{
	GtkWidget *button;
	GtkWidget *label_hi;
	GtkWidget *label_lo;
	GtkWidget *grid;

	grid = gtk_grid_new();
	gtk_widget_set_can_focus(grid, FALSE);
	gtk_widget_set_hexpand(grid, TRUE);
	gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);

	if (!icon) {
		button = gtk_button_new();

		label_hi = gtk_label_new(text_hi);
		gtk_label_set_use_markup(GTK_LABEL(label_hi), TRUE);
		gtk_grid_attach(GTK_GRID(grid), label_hi, 0, 0, 1, 1);
	} else {
		button = gtk_toggle_button_new();

		gtk_grid_attach(GTK_GRID(grid), icon, 0, 0, 1, 1);
		gtk_widget_set_margin(icon, 4, 3, 4, 4);
	}

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

	if (state->connection) {
		/* Active connection is available, send dtmf code */
		phone_send_dtmf_code(state->connection, num);
	} else {
		/* Add number to entry */
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(state->name_entry));
		gchar *tmp = g_strdup_printf("%s%c", text, num);

		gtk_entry_set_text(GTK_ENTRY(state->name_entry), tmp);
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
GtkWidget *phone_dialpad_frame(struct phone_state *state)
{
	GtkWidget *frame;
	GtkWidget *grid;
	GtkWidget *button;
	gint index;

	frame = gtk_frame_new(NULL);
	gtk_widget_set_can_focus(frame, FALSE);

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
		button = phone_create_button(phone_labels[index].text, phone_labels[index].subtext, NULL);
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
static void hold_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;

	if (!state->connection) {
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
static void mute_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;

	if (!state->connection) {
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
static void record_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;
	const gchar *user_plugins;
	gchar *path;

	if (!state->connection) {
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
static void c_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;
	GtkWidget *entry = state->name_entry;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	if (text && strlen(text)) {
		gchar *new = g_strdup(text);

		new[strlen(text) - 1] = '\0';
		gtk_entry_set_text(GTK_ENTRY(entry), new);
		g_free(new);
	}
}

/**
 * \brief Create phone control frame
 * \param state phone state pointer
 * \return phone control frame
 */
GtkWidget *phone_control_frame(struct phone_state *state)
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
	button = phone_create_button("<b>C</b>", _("<small>Clear</small>"), NULL);
	g_signal_connect(button, "clicked", G_CALLBACK(c_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

	/* Add mute button */
	image = gtk_image_new_from_icon_name("audio-input-microphone-symbolic", GTK_ICON_SIZE_BUTTON);
	state->mute_button = phone_create_button(NULL, _("<small>Mute</small>"), image);
	g_signal_connect(state->mute_button, "clicked", G_CALLBACK(mute_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(grid), state->mute_button, 0, 1, 1, 1);

	/* Add hold button */
	image = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
	state->hold_button = phone_create_button(NULL, _("<small>Hold</small>"), image);
	g_signal_connect(state->hold_button, "clicked", G_CALLBACK(hold_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(grid), state->hold_button, 0, 2, 1, 1);

	/* Add record button */
	image = gtk_image_new_from_icon_name("media-record-symbolic", GTK_ICON_SIZE_BUTTON);
	state->record_button = phone_create_button(NULL, _("<small>Record</small>"), image);
	g_signal_connect(state->record_button, "clicked", G_CALLBACK(record_clicked_cb), state);
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
gboolean phone_window_delete_event_cb(GtkWidget *window, GdkEvent *event, gpointer data)
{
	struct phone_state *state = data;

	if (state && state->connection) {
		show_active_call_dialog(window);

		/* Keep window */
		return TRUE;
	}

	/* Disconnect all signal handlers */
	g_signal_handlers_disconnect_by_data(app_object, state);

	g_slice_free(struct phone_state, state);
	phone_window = NULL;

	return FALSE;
}

/**
 * \brief Set sensitive value of buttons (type SOFTPHONE = true, else false)
 * \param state phone state pointer
 * \param type active phone type
 */
static void phone_button_set_sensitive(struct phone_state *state, gint type)
{
	gboolean status = (type == PORT_SOFTPHONE);

	gtk_widget_set_sensitive(state->mute_button, status);
	gtk_widget_set_sensitive(state->hold_button, status);
	gtk_widget_set_sensitive(state->record_button, status);
}

/**
 * \brief Phone toggled callback - sets selected port type to profile settings
 * \param item percieved check menu item
 * \param user_data containing pointer to corresponding phone type
 */
static void phone_toggled_cb(GtkCheckMenuItem *item, gpointer user_data)
{
	/* If item is active, adjust phone port accordingly and set sensitivity of phone buttons */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item))) {
		gint type = GPOINTER_TO_INT(user_data);
		struct phone_state *state = g_object_get_data(G_OBJECT(item), "phone_state");

		router_set_phone_port(profile_get_active(), type);
		phone_button_set_sensitive(state, type);
	}
}

/**
 * \brief Create phone menu which contains phone selection and suppress number toggle
 * \param profile pointer to current profile
 * \param state phone state pointer
 * \return newly create phone menu
 */
static GtkWidget *phone_create_menu(struct profile *profile, struct phone_state *state)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *box;
	GSList *phone_list = NULL;
	GSList *phone_radio_list = NULL;
	struct phone *phone = NULL;
	gint type = 0;

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin(box, 12, 12, 12, 12);

	/* Create popover */
	menu = gtk_popover_new(NULL);
	gtk_container_add(GTK_CONTAINER(menu), box);

	/* Fill menu */
	item = gtk_label_new(_("Phones"));
	gtk_widget_set_sensitive(item, FALSE);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

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
		g_signal_connect(item, "toggled", G_CALLBACK(phone_toggled_cb), GINT_TO_POINTER(phone->type));
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

	return menu;
}

/**
 * \brief Pick up incoming connection
 * \param state phone state pointer
 * \param connection incoming connection
 */
static void phone_pickup_incoming(struct phone_state *state, struct connection *connection)
{
	struct capi_connection *capi_connection = connection->priv ? connection->priv : active_capi_connection;

	/* Pick up */
	if (!phone_pickup(capi_connection)) {
		/* Set internal connection */
		state->connection = capi_connection;

		phone_allow_dial(state, FALSE);
	}
}

/**
 * \brief Create and show phone window
 * \param contact pointer to contact structure
 * \param connection incoming connection to pickup
 */
void app_show_phone_window(struct contact *contact, struct connection *connection)
{
	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *menubutton;
	GtkWidget *grid;
	GtkWidget *frame;
	gint pos_y;
	struct phone_state *state;
	struct profile *profile = profile_get_active();

	if (!profile) {
		return;
	}

	/* If there is already an open phone window, present it to the user and return */
	if (phone_window) {
		state = g_object_get_data(G_OBJECT(phone_window), "state");

		if (state && !state->connection && connection) {
			phone_pickup_incoming(state, connection);

			/* Set contact name */
			phone_set_contact_name(state, contact);
		}

		gtk_window_present(GTK_WINDOW(phone_window));
		return;
	}

	/* Allocate phone state structure */
	state = g_slice_alloc0(sizeof(struct phone_state));
	state->type = PHONE_TYPE_VOICE;

	/* Create window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Phone"));
	gtk_window_set_default_size(GTK_WINDOW(window), 450, -1);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	g_object_set_data(G_OBJECT(window), "state", state);
	g_signal_connect(window, "delete_event", G_CALLBACK(phone_window_delete_event_cb), state);

	/* Create header bar and set it to window */
	header = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), _("Phone"));
	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header), _("Time: 00:00:00"));
	gtk_window_set_titlebar(GTK_WINDOW(window), header);

	/* Create and add menu button to header bar */
	menubutton = gtk_menu_button_new();
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(menubutton), GTK_ARROW_NONE);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), menubutton);

	/* Create menu and add it to menu button */
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menubutton), phone_create_menu(profile, state));
	state->headerbar = header;

	/* Create grid and attach it to the window */
	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), grid);

	/* Set margin to 12px for all sides */
	gtk_widget_set_margin(grid, 12, 12, 12, 12);

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 18);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	/* Yes, we start at pos_y = 1, pos_y = 0 will be set below */
	pos_y = 1;
	state->dialpad_frame = phone_dialpad_frame(state);
	gtk_grid_attach(GTK_GRID(grid), state->dialpad_frame, 0, pos_y, 1, 1);

	state->control_frame = phone_control_frame(state);
	gtk_grid_attach(GTK_GRID(grid), state->control_frame, 1, pos_y, 1, 1);

	/* We set the dial frame last, so that all other widgets are in place */
	frame = phone_create_name_frame(window, contact, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 0, 1, 1);

	/* Add dial button frame */
	frame = phone_dial_button_frame(window, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 1, 0, 1, 1);

	/* Toggle phone button depending on active phone port */
	phone_button_set_sensitive(state, router_get_phone_port(profile));

	/* Connect connection signals */
	g_signal_connect(app_object, "connection-notify", G_CALLBACK(phone_connection_notify_cb), state);
	g_signal_connect(app_object, "connection-established", G_CALLBACK(capi_connection_established_cb), state);
	g_signal_connect(app_object, "connection-terminated", G_CALLBACK(capi_connection_terminated_cb), state);

	/* In case there is an incoming connection, pick it up */
	if (connection) {
		phone_pickup_incoming(state, connection);
	} else {
		phone_allow_dial(state, TRUE);
	}

	/* Show window and set phone_window */
	gtk_widget_show_all(window);
	phone_window = window;
}
