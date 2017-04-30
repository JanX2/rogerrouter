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

#include <rm/rmobject.h>
#include <rm/rmobjectemit.h>
#include <rm/rmrouter.h>
#include <rm/rmphone.h>
#include <rm/rmaddressbook.h>
#include <rm/rmstring.h>
#include <rm/rmnumber.h>
#include <rm/rmphone.h>

#include <roger/main.h>
#include <roger/journal.h>
#include <roger/phone.h>
#include <roger/uitools.h>
#include <roger/icons.h>
#include <roger/contactsearch.h>

#include <roger/contacts.h>

static struct phone_state *phone_state = NULL;

static gchar *phone_get_active_name(void)
{
	//GSList *phone_list;
	RmProfile *profile = rm_profile_get_active();
	RmPhone *phone = rm_profile_get_phone(profile);

	return phone ? phone->name : g_strdup(_("Phone"));
}

/**
 * \brief Phone status timer
 * \param data UNUSED
 * \return TRUE
 */
static gboolean phone_status_timer_cb(gpointer data)
{
	gchar *time_diff;
	gchar *buf;
 
	time_diff = rm_connection_get_duration_time(phone_state->connection);
	buf = g_strdup_printf(_("Time: %s"), time_diff);
	g_free(time_diff);

	if (roger_uses_headerbar()) {
		gtk_header_bar_set_subtitle(GTK_HEADER_BAR(phone_state->header_bar), buf);
	} else {
		gchar *title = g_strdup_printf("%s - %s", phone_get_active_name(), buf);
		gtk_window_set_title(GTK_WINDOW(phone_state->window), title);
		g_free(title);
	}

	g_free(buf);

	return TRUE;
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
	gtk_widget_set_sensitive(phone_state->pickup_button, allow_dial);
	gtk_widget_set_sensitive(phone_state->hangup_button, !allow_dial);

	/* Toggle control buttons sensitive value */
	if (phone_state->connection && phone_state->connection->type & RM_CONNECTION_TYPE_SOFTPHONE) {
		phone_control_buttons_set_sensitive(!allow_dial);
	} else {
		phone_control_buttons_set_sensitive(FALSE);
	}
}

/**
 * \brief Connection changed callback
 * \param object appobject
 * \param connection connection pointer
 * \param user_data phone state pointer
 */
static void phone_connection_changed_cb(RmObject *obj, gint type, RmConnection *connection, gpointer user_data)
{
	g_debug("%s(): connection = %p", __FUNCTION__, connection);

	if (!connection || !phone_state || !phone_state->connection || phone_state->connection->id != connection->id) {
		return;
	}

	if (!(type & RM_CONNECTION_TYPE_DISCONNECT)) {
		return;
	}

	g_debug("%s(): disconnect", __FUNCTION__);
	/* Make sure that the status information is updated */
	phone_status_timer_cb(NULL);

	/* Remove timer source */
	if (phone_state->status_timer_id) {
		g_source_remove(phone_state->status_timer_id);
		phone_state->status_timer_id = 0;
	}

	phone_dial_buttons_set_dial(TRUE);
	phone_state->connection = NULL;
}

/**
 * \brief Phone connection failed - Show error dialog including user support text
 */
void phone_connection_failed(void)
{
	GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(phone_state->window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Connection problem"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(error_dialog), _("This is most likely due to a closed CAPI port or an invalid phone number.\n\nSolution: Dial #96*3* with your phone and check your numbers within the settings"));

	gtk_dialog_run(GTK_DIALOG(error_dialog));
	gtk_widget_destroy(error_dialog);
}

/**
 * phone_active_call_dialog:
 * @window: parent window
 *
 * Dialog showing that an active call is running.
 *
 * Returns: %TRUE if used clicked on YES, %FALSE if no is clicked
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
	RmProfile *profile = rm_profile_get_active();
	gchar *scramble;

	/* Get selected number (either number format or based on the selected name) */
	phone_state->number = contact_search_get_number(CONTACT_SEARCH(phone_state->contact_search));

	if (RM_EMPTY_STRING(phone_state->number)) {
		return;
	}

	scramble = rm_number_scramble(phone_state->number);
	g_debug("%s(): Dialing '%s'", __FUNCTION__, scramble);
	g_free(scramble);

	phone_state->phone = rm_profile_get_phone(rm_profile_get_active());
	phone_state->connection = rm_phone_dial(phone_state->phone, phone_state->number, rm_router_get_suppress_state(profile));
	if (phone_state->connection) {
		phone_dial_buttons_set_dial(FALSE);

		if (!phone_state->status_timer_id) {
			phone_state->status_timer_id = g_timeout_add(250, phone_status_timer_cb, NULL);
		}
	}
}

/**
 * \brief Hangup button clicked callback
 * \param button hangup button
 * \param user_data UNUSED
 */
void hangup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	//RmProfile *profile = rm_profile_get_active();
	//const gchar *number = gtk_entry_get_text(GTK_ENTRY(phone_state->search_entry));

	if (!phone_state->connection) {
		return;
	}

	phone_control_buttons_reset();
	rm_phone_hangup(phone_state->connection);
	phone_dial_buttons_set_dial(TRUE);
}

/**
 * \brief Phone toggled callback - sets selected port type to profile settings
 * \param item percieved check menu item
 * \param user_data containing pointer to corresponding phone type
 */
void phone_item_toggled_cb(GtkCheckMenuItem *item, gpointer user_data)
{
	g_debug("%s(): called, %s", __FUNCTION__, (gchar *) user_data);

	/* If item is active, adjust phone port accordingly and set sensitivity of phone buttons */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item))) {
		gchar *name = user_data;

		rm_profile_set_phone(rm_profile_get_active(), name);

		/* Get active name and set window title */
		gtk_window_set_title(GTK_WINDOW(phone_state->window), name);
	}
}

/**
 * \brief Create phone window menu which contains phone selection and suppress number toggle
 * \param profile pointer to current profile
 * \return newly create phone menu
 */
static GtkWidget *phone_window_create_menu(RmProfile *profile, struct phone_state *state)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *box;
	GSList *phone_radio_list = NULL;
	GSList *phone_plugins = NULL;

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
	phone_plugins = rm_phone_get_plugins();
	//type = rm_profile_get_phone(profile);

	/* Traverse phone list and add each phone */
	while (phone_plugins) {
		RmPhone *phone = phone_plugins->data;

		item = gtk_radio_button_new_with_label(phone_radio_list, phone->name);

		phone_radio_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(item));

		if (!state->phone) {
			state->phone = phone;
			g_debug("%s(): Setting default phone", __FUNCTION__);
			rm_profile_set_phone(rm_profile_get_active(), phone->name);
		}

		if (phone == state->phone) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item), TRUE);
		}

		g_signal_connect(item, "toggled", G_CALLBACK(phone_item_toggled_cb), phone->name);

		gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);
		phone_plugins = phone_plugins->next;
	}

	/* Free phone list */
	//rm_phone_release_list(phone_list);

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
 * \brief Dialpad button pressed - send either dtmf code or add number to entry
 * \param widget button widget
 * \param user_data UNUSED
 */
void phone_dtmf_clicked_cb(GtkWidget *widget, gpointer user_data)
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
	rm_phone_record(phone_state->phone, phone_state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)), path);

	g_free(path);
}

/**
 * \brief Hold button clicked callback - toggle hold call state
 * \param widget button widget
 * \param user_data UNUSED
 */
void phone_hold_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	/* Toggle hold call */
	rm_phone_hold(phone_state->phone, phone_state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/**
 * \brief Mute button clicked callback - toggle mute call state
 * \param widget button widget
 * \param user_data phone state pointer
 */
void phone_mute_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	/* Toggle mute call */
	rm_phone_mute(phone_state->phone, phone_state->connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}


/**
 * \brief Clear button clicked callback - removes last char of number/name
 * \param widget button widget
 * \param user_data UNUSED
 */
void phone_clear_button_clicked_cb(GtkWidget *widget, gpointer user_data)
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
 * \brief Pick up incoming connection
 * \param connection incoming connection
 */
static void phone_pickup(RmConnection *connection)
{
	/* Pick up */
	if (!rm_phone_pickup(connection)) {
		/* Set internal connection */
		phone_state->connection = connection;

		/* Disable dial button */
		phone_dial_buttons_set_dial(FALSE);

		if (!phone_state->status_timer_id) {
			phone_state->status_timer_id = g_timeout_add(250, phone_status_timer_cb, NULL);
		}
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
	if (!phone_state) {
		return FALSE;
	}

	/* If there is an active connection, warn user and refuse to delete the window */
	if (phone_state && phone_state->connection) {
		if (phone_active_call_dialog(window)) {
			/* Keep window */
			return TRUE;
		}

		/* Remove timer source */
		if (phone_state->status_timer_id) {
			g_source_remove(phone_state->status_timer_id);
			phone_state->status_timer_id = 0;
	}
	}

	/* Disconnect all signal handlers */
	g_signal_handlers_disconnect_by_data(rm_object, phone_state);

	phone_state->window = NULL;

	/* Free phone state structure and release phone window */
	g_slice_free(struct phone_state, phone_state);

	phone_state = NULL;

	return FALSE;
}


void app_show_phone_window(RmContact *contact, RmConnection *connection)
{
	GtkBuilder *builder;
	RmProfile *profile = rm_profile_get_active();

	if (!profile) {
		return;
	}

	/* If there is already an open phone window, present it to the user and return */
	if (phone_state) {
		/* Incoming call? */
		if (!phone_state->connection && connection) {
			phone_pickup(connection);

			/* Set contact name */
			contact_search_set_contact(CONTACT_SEARCH(phone_state->contact_search), contact, TRUE);
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

	phone_state->phone = rm_profile_get_phone(profile);

	/* Connect to builder objects */
	phone_state->window = GTK_WIDGET(gtk_builder_get_object(builder, "phone_window"));
	gtk_window_set_transient_for(GTK_WINDOW(phone_state->window), GTK_WINDOW(journal_get_window()));

	phone_state->grid = GTK_WIDGET(gtk_builder_get_object(builder, "phone_grid"));

	GtkWidget *grid2 = GTK_WIDGET(gtk_builder_get_object(builder, "phone_grid_intern"));
	phone_state->contact_search = contact_search_new();
	gtk_grid_attach(GTK_GRID(grid2), phone_state->contact_search, 0, 0, 1, 1);

	phone_state->pickup_button = GTK_WIDGET(gtk_builder_get_object(builder, "call_start_button"));
	phone_state->hangup_button = GTK_WIDGET(gtk_builder_get_object(builder, "call_stop_button"));

	phone_state->mute_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_mute_button"));
	phone_state->hold_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_hold_button"));
	phone_state->record_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_record_button"));

	phone_state->child_frame = GTK_WIDGET(gtk_builder_get_object(builder, "child_frame"));

	phone_state->menu_button = GTK_WIDGET(gtk_builder_get_object(builder, "phone_menu_button"));

	GtkWidget *menu = phone_window_create_menu(rm_profile_get_active(), phone_state);
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(phone_state->menu_button), menu);

	phone_state->header_bar = GTK_WIDGET(gtk_builder_get_object(builder, "phone_headerbar"));

	if (roger_uses_headerbar()) {
		/* Create header bar and set it to window */
		gtk_header_bar_set_title(GTK_HEADER_BAR(phone_state->header_bar), phone_get_active_name());
		gtk_header_bar_set_subtitle(GTK_HEADER_BAR(phone_state->header_bar), "");
		gtk_window_set_titlebar(GTK_WINDOW(phone_state->window), phone_state->header_bar);
	} else {
		GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(builder, "phone_grid"));

		gtk_window_set_title(GTK_WINDOW(phone_state->window), phone_get_active_name());
		gtk_grid_attach(GTK_GRID(grid), phone_state->header_bar, 0, 0, 3, 1);
	}

	gtk_builder_add_callback_symbol(builder, "phone_window_delete_event_cb", G_CALLBACK(phone_window_delete_event_cb));
	gtk_builder_add_callback_symbol(builder, "phone_dtmf_clicked_cb", G_CALLBACK(phone_dtmf_clicked_cb));
	gtk_builder_add_callback_symbol(builder, "phone_record_button_clicked_cb", G_CALLBACK(phone_record_button_clicked_cb));
	gtk_builder_add_callback_symbol(builder, "phone_hold_button_clicked_cb", G_CALLBACK(phone_hold_button_clicked_cb));
	gtk_builder_add_callback_symbol(builder, "phone_mute_button_clicked_cb", G_CALLBACK(phone_mute_button_clicked_cb));
	gtk_builder_add_callback_symbol(builder, "phone_clear_button_clicked_cb", G_CALLBACK(phone_clear_button_clicked_cb));
	gtk_builder_add_callback_symbol(builder, "hangup_button_clicked_cb", G_CALLBACK(hangup_button_clicked_cb));
	gtk_builder_add_callback_symbol(builder, "pickup_button_clicked_cb", G_CALLBACK(pickup_button_clicked_cb));
	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	/* Connect connection signal */
	g_signal_connect(rm_object, "connection-changed", G_CALLBACK(phone_connection_changed_cb), NULL);

	if (contact) {
		/* Set contact name */
		phone_state->discard = TRUE;
		contact_search_set_contact(CONTACT_SEARCH(phone_state->contact_search), contact, TRUE);
	}

	/* In case there is an incoming connection, pick it up */
	if (connection) {
		phone_pickup(connection);
	} else {
		/* No incoming call, allow dialing */
		phone_dial_buttons_set_dial(TRUE);
	}

	gtk_widget_show_all(phone_state->window);
}
