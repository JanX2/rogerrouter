/*
 * Roger Router
 * Copyright (c) 2012-2017 Jan-Michael Brummer
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

#include <ctype.h>
#include <string.h>

#include <gtk/gtk.h>

#include <rm/rm.h>

#include <roger/main.h>
#include <roger/journal.h>
#include <roger/phone.h>
#include <roger/uitools.h>
#include <roger/icons.h>
#include <roger/contactsearch.h>

#include <roger/contacts.h>

typedef struct _phone_state {
	GtkWidget *window;
	GtkWidget *header_bar;
	GtkWidget *contact_search;
	GtkWidget *mute_button;
	GtkWidget *hold_button;
	GtkWidget *record_button;
	GtkWidget *clear_button;
	GtkWidget *pickup_button;
	GtkWidget *hangup_button;

	gint status_timer_id;

	RmPhone *phone;
	RmConnection *connection;
	RmProfile *profile;
} PhoneState;

/**
 * phone_status_timer_cb:
 * @data: a #PhoneState
 *
 * Phone status timer: Update time label in sub title
 *
 * Returns: %TRUE
 */
static gboolean phone_status_timer_cb(gpointer data)
{
	PhoneState *phone_state = data;
	gchar *time_diff;
	gchar *buf;

	time_diff = rm_connection_get_duration_time(phone_state->connection);
	buf = g_strdup_printf(_("Time: %s"), time_diff);
	g_free(time_diff);

	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(phone_state->header_bar), buf);

	g_free(buf);

	return TRUE;
}

/**
 * phone_dial_buttons_set_dial:
 * @phone_state: a #PhoneState
 * @allow_dial: flag indicating if pick up is allowed
 *
 * Set sensitive value of hangup/pickup button depending on @allow_dial
 */
static void phone_dial_buttons_set_dial(PhoneState *phone_state, gboolean allow_dial)
{
	gboolean control_buttons = FALSE;

	gtk_widget_set_sensitive(phone_state->pickup_button, allow_dial);
	gtk_widget_set_sensitive(phone_state->hangup_button, !allow_dial);

	/* Toggle control buttons sensitive value */
	if (phone_state->connection && phone_state->connection->type & RM_CONNECTION_TYPE_SOFTPHONE) {
		control_buttons = !allow_dial;
	}

	/* Set control button sensitive value */
	gtk_widget_set_sensitive(phone_state->mute_button, control_buttons);
	gtk_widget_set_sensitive(phone_state->hold_button, control_buttons);
	gtk_widget_set_sensitive(phone_state->record_button, control_buttons);
}

/**
 * phone_start_status_timer:
 * @phone_state: a #PhoneState
 *
 * Start status timer
 */
static void phone_start_status_timer(PhoneState *phone_state)
{
	if (!phone_state->status_timer_id) {
		phone_state->status_timer_id = g_timeout_add(250, phone_status_timer_cb, phone_state);
	}
}

/**
 * phone_remove_status_timer:
 * @phone_state: a #PhoneState
 *
 * Remove status timer
 */
static void phone_remove_status_timer(PhoneState *phone_state)
{
	if (phone_state && phone_state->status_timer_id) {
		g_source_remove(phone_state->status_timer_id);
		phone_state->status_timer_id = 0;
	}
}

/**
 * phone_connection_changed_cb:
 * @object: a #RmObject
 * @type: connection type
 * @connection: a #RmConnection
 * @phone_state: a #PhoneState
 *
 * Connection changed callback
 *  - Reacts on disconnects: Update status, remove timer and update button state
 */
static void phone_connection_changed_cb(RmObject *obj, gint type, RmConnection *connection, PhoneState *phone_state)
{
	if (!connection || !phone_state || !phone_state->connection || phone_state->connection != connection) {
		return;
	}

	if (!(type & RM_CONNECTION_TYPE_DISCONNECT)) {
		return;
	}

	/* Remove timer source */
	phone_remove_status_timer(phone_state);

	phone_state->connection = NULL;
	phone_dial_buttons_set_dial(phone_state, TRUE);
}

/**
 * phone_active_call_dialog:
 * @window: parent window
 *
 * Dialog showing that an active call is running.
 *
 * Returns: %TRUE if user clicked on YES, %FALSE if no is clicked
 */
static gboolean phone_active_call_dialog(GtkWidget *window)
{
	GtkWidget *dialog;
	gint result;

	/* Create dialog and run it */
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_YES_NO, _("A call is in progress, hangup first"));
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	return result == GTK_RESPONSE_YES;
}

/**
 * phone_pickup_button_clicked_cb:
 * @button: pickup button
 * @phone_state: a #PhoneState
 *
 * Pickup button clicked callback
 */
static void phone_pickup_button_clicked_cb(GtkWidget *button, PhoneState *phone_state)
{
	gchar *number;

	/* Get selected number (either number format or based on the selected name) */
	number = contact_search_get_number(CONTACT_SEARCH(phone_state->contact_search));

	if (RM_EMPTY_STRING(number)) {
		return;
	}

	/* Set phone and dial out */
	phone_state->connection = rm_phone_dial(phone_state->phone, number, rm_router_get_suppress_state(phone_state->profile));

	/* Check if we have a connection */
	if (phone_state->connection) {
		phone_dial_buttons_set_dial(phone_state, FALSE);

		phone_start_status_timer(phone_state);
	}
}

/**
 * phone_hangup_button_clicked_cb:
 * @button: hangup button
 * @user_data: a #PhoneState
 *
 * Hangup button clicked callback
 */
static void phone_hangup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	PhoneState *phone_state = user_data;

	g_return_if_fail(phone_state->connection != NULL);

	/* Hangup connection */

	rm_phone_hangup(phone_state->connection);

	/* Allow dialing and remove status timer */
	phone_dial_buttons_set_dial(phone_state, TRUE);
	phone_remove_status_timer(phone_state);
}

/**
 * phone_item_toggled_cb:
 * @item: a #GtkToggleButton
 * @phone_state: a #PhoneState
 *
 * Phone toggled callback - sets selected port type to profile settings
 */
static void phone_item_toggled_cb(GtkToggleButton *button, PhoneState *phone_state)
{
	/* If item is active, adjust phone port accordingly */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
		const gchar *name = gtk_button_get_label(GTK_BUTTON(button));
		RmPhone *phone = rm_phone_get(name);

		rm_profile_set_phone(phone_state->profile, phone);
		phone_state->phone = phone;

		/* Get active name and set window title */
		gtk_header_bar_set_title(GTK_HEADER_BAR(phone_state->header_bar), name);
	}
}

/**
 * phone_create_menu:
 * @phone_state: a #PhoneState
 *
 * Create phone window menu which contains phone selection and suppress number toggle
 *
 * Returns: newly create phone menu
 */
static GtkWidget *phone_create_menu(PhoneState *phone_state)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *box;
	GSList *phone_radio_list = NULL;
	GSList *list = NULL;

	/* Create popover */
	menu = gtk_popover_new(NULL);

	/* Create vertical box */
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin(box, 6, 6, 6, 6);

	gtk_container_add(GTK_CONTAINER(menu), box);

	/* Fill menu */
	item = gtk_label_new(_("Phones"));
	gtk_widget_set_sensitive(item, FALSE);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

	/* Traverse phone list and add each phone */
	for (list = rm_phone_get_plugins(); list != NULL && list->data != NULL; list = list->next) {
		RmPhone *phone = list->data;

		item = gtk_radio_button_new_with_label(phone_radio_list, phone->name);

		phone_radio_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(item));

		if (phone == phone_state->phone) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item), TRUE);
		}

		g_signal_connect(item, "toggled", G_CALLBACK(phone_item_toggled_cb), phone_state);

		gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);
	}

	/* Add separator */
	item = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

	/* Add suppress check item */
	item = gtk_check_button_new_with_label(_("Suppress number"));
	g_settings_bind(phone_state->profile->settings, "suppress", item, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

	gtk_widget_show_all(box);

	return menu;
}

/**
 * phone_dtmf_button_clicked_cb:
 * @widget: button widget
 * @phone_state: a #PhoneState
 *
 * Dialpad button pressed - send either dtmf code or add number to entry
 */
static void phone_dtmf_button_clicked_cb(GtkWidget *widget, PhoneState *phone_state)
{
	const gchar *name = gtk_widget_get_name(widget);
	gint num = name[7];

	if (phone_state->connection) {
		/* Active connection is available, send dtmf code */
		rm_phone_dtmf(phone_state->phone, phone_state->connection, num);
	} else {
		/* Add number to entry */
		const gchar *text = contact_search_get_text(CONTACT_SEARCH(phone_state->contact_search));
		gchar *tmp = g_strdup_printf("%s%c", text, num);

		contact_search_set_text(CONTACT_SEARCH(phone_state->contact_search), tmp);
		g_free(tmp);
	}
}

/**
 * phone_record_button_clicked_cb:
 * @widget: button widget
 * @phone_state: a #PhoneState
 *
 * Record button clicked callback - toggle record call state
 */
static void phone_record_button_clicked_cb(GtkWidget *widget, PhoneState *phone_state)
{
	/* Toggle record state */
	rm_phone_record(phone_state->phone, phone_state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/**
 * phone_hold_button_clicked_cb:
 * @widget: button widget
 * @phone_state: a #PhoneState
 *
 * Hold button clicked callback - toggle hold call state
 */
static void phone_hold_button_clicked_cb(GtkWidget *widget, PhoneState *phone_state)
{
	/* Toggle hold call */
	rm_phone_hold(phone_state->phone, phone_state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/**
 * phone_mute_button_clicked_cb:
 * @widget: button widget
 * @phone_state: a #PhoneState
 *
 * Mute button clicked callback - toggle mute call state
 */
static void phone_mute_button_clicked_cb(GtkWidget *widget, PhoneState *phone_state)
{
	/* Toggle mute call */
	rm_phone_mute(phone_state->phone, phone_state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/**
 * phone_clear_button_clicked_cb:
 * @widget: button widget
 * @phone_state: a #PhoneState
 *
 * Clear button clicked callback - removes last char of number/name
 */
static void phone_clear_button_clicked_cb(GtkWidget *widget, PhoneState *phone_state)
{
	const gchar *text = contact_search_get_text(CONTACT_SEARCH(phone_state->contact_search));

	/* If there is text within the entry, remove last char */
	if (!RM_EMPTY_STRING(text)) {
		gchar *new = g_strdup(text);

		new[strlen(text) - 1] = '\0';
		contact_search_set_text(CONTACT_SEARCH(phone_state->contact_search), new);
		g_free(new);
	}
}

/**
 * phone_pickup;
 * @phone_state: a #PhoneState
 * @connection: incoming #RmConnection
 *
 * Pick up incoming connection
 */
static void phone_pickup(PhoneState *phone_state, RmConnection *connection)
{
	/* Pick up */
	if (!rm_phone_pickup(connection)) {
		/* Set internal connection */
		phone_state->connection = connection;

		/* Disable dial button */
		phone_dial_buttons_set_dial(phone_state, FALSE);

		phone_start_status_timer(phone_state);
	}
}

/**
 * phone_window_delete_event_cb:
 * @window: window widget
 * @event: a #GdkEvent
 * @phone_state: a #PhoneState
 *
 * Phone window delete event callback
 *
 * Returns: %FALSE if window should be deleted
 */
static gboolean phone_window_delete_event_cb(GtkWidget *window, GdkEvent *event, PhoneState *phone_state)
{
	if (!phone_state) {
		return FALSE;
	}

	/* If there is an active connection, warn user and refuse to delete the window */
	if (phone_state->connection) {
		if (phone_active_call_dialog(window)) {
			/* Keep window */
			return TRUE;
		}

		/* Remove timer source */
		phone_remove_status_timer(phone_state);
	}

	/* Disconnect all signal handlers */
	g_signal_handlers_disconnect_by_data(rm_object, phone_state);

	phone_state->window = NULL;

	/* Free phone state structure and release phone window */
	g_slice_free(PhoneState, phone_state);

	phone_state = NULL;

	return FALSE;
}

/**
 * app_phone:
 * @contact: a #RmContact
 * @connection: a #RmConnection
 *
 * Opens phone dialog
 *  - If there is an incoming call, pick it up
 */
void app_phone(RmContact *contact, RmConnection *connection)
{
	GtkBuilder *builder;
	GtkWidget *menu_button;
	RmProfile *profile = rm_profile_get_active();
	PhoneState *phone_state = NULL;

	g_return_if_fail(profile != NULL);

	builder = gtk_builder_new_from_resource("/org/tabos/roger/phone.glade");
	if (!builder) {
		g_warning("Could not load phone ui");
		return;
	}

	/* Allocate phone state structure */
	phone_state = g_slice_alloc0(sizeof(PhoneState));

	phone_state->profile = profile;
	phone_state->phone = rm_profile_get_phone(phone_state->profile);

	/* Connect to builder objects */
	phone_state->window = GTK_WIDGET(gtk_builder_get_object(builder, "phone_window"));
	gtk_window_set_transient_for(GTK_WINDOW(phone_state->window), GTK_WINDOW(journal_get_window()));
	gtk_window_set_position(GTK_WINDOW(phone_state->window), GTK_WIN_POS_CENTER_ALWAYS);

	GtkWidget *grid2 = GTK_WIDGET(gtk_builder_get_object(builder, "phone_grid"));
	phone_state->contact_search = contact_search_new();
	gtk_grid_attach(GTK_GRID(grid2), phone_state->contact_search, 0, 0, 1, 1);

	phone_state->pickup_button = GTK_WIDGET(gtk_builder_get_object(builder, "call_start_button"));
	phone_state->hangup_button = GTK_WIDGET(gtk_builder_get_object(builder, "call_stop_button"));
	phone_state->mute_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_mute_button"));
	phone_state->hold_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_hold_button"));
	phone_state->record_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_record_button"));
	phone_state->clear_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_clear_button"));

	menu_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_menu_button"));

	GtkWidget *menu = phone_create_menu(phone_state);
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), menu);

	phone_state->header_bar = GTK_WIDGET(gtk_builder_get_object(builder, "phone_headerbar"));

	/* Create header bar and set it to window */
	gtk_header_bar_set_title(GTK_HEADER_BAR(phone_state->header_bar), phone_state->phone ? g_strdup(phone_state->phone->name) : g_strdup(_("Phone")));
	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(phone_state->header_bar), "");

	g_signal_connect(G_OBJECT(phone_state->window), "delete-event", G_CALLBACK(phone_window_delete_event_cb), phone_state);
	g_signal_connect(G_OBJECT(phone_state->hangup_button), "clicked", G_CALLBACK(phone_hangup_button_clicked_cb), phone_state);
	g_signal_connect(G_OBJECT(phone_state->pickup_button), "clicked", G_CALLBACK(phone_pickup_button_clicked_cb), phone_state);
	g_signal_connect(G_OBJECT(phone_state->record_button), "clicked", G_CALLBACK(phone_record_button_clicked_cb), phone_state);
	g_signal_connect(G_OBJECT(phone_state->hold_button), "clicked", G_CALLBACK(phone_hold_button_clicked_cb), phone_state);
	g_signal_connect(G_OBJECT(phone_state->mute_button), "clicked", G_CALLBACK(phone_mute_button_clicked_cb), phone_state);
	g_signal_connect(G_OBJECT(phone_state->clear_button), "clicked", G_CALLBACK(phone_clear_button_clicked_cb), phone_state);

	GtkWidget *tmp;
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_0"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_1"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_2"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_3"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_4"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_5"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_6"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_7"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_8"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_9"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_*"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);
	tmp = GTK_WIDGET(gtk_builder_get_object(builder, "button_#"));
	g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(phone_dtmf_button_clicked_cb), phone_state);

	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	/* Connect connection signal */
	g_signal_connect(rm_object, "connection-changed", G_CALLBACK(phone_connection_changed_cb), phone_state);

	if (contact) {
		/* Set contact name */
		contact_search_set_contact(CONTACT_SEARCH(phone_state->contact_search), contact, TRUE);
	}

	/* In case there is an incoming connection, pick it up */
	if (connection) {
		phone_pickup(phone_state, connection);
	} else {
		/* No incoming call, allow dialing */
		phone_dial_buttons_set_dial(phone_state, TRUE);
	}

	gtk_widget_show_all(phone_state->window);
}

