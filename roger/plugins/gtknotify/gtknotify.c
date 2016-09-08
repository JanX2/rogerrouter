#if 0

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

#include <libroutermanager/rmplugins.h>
#include <libroutermanager/rmcall.h>
#include <libroutermanager/rmobject.h>
#include <libroutermanager/rmobjectemit.h>
#include <libroutermanager/plugins/capi/phone.h>
#include <libroutermanager/plugins/capi/ringtone.h>
#include <libroutermanager/rmphone.h>
#include <libroutermanager/rmrouter.h>
#include <libroutermanager/rmprofile.h>
#include <libroutermanager/rmlookup.h>
#include <libroutermanager/rmstring.h>
#include <libroutermanager/rmsettings.h>

#include <roger/main.h>
#include <roger/application.h>
#include <roger/phone.h>
#include <roger/uitools.h>
#include <roger/icons.h>
#include <roger/settings.h>

#define RM_TYPE_NOTIFICATION_GTK_PLUGIN (routermanager_notification_gtk_plugin_get_type ())
#define RM_NOTIFICATION_GTK_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), RM_TYPE_NOTIFICATION_GTK_PLUGIN, RmNotificationGtkPlugin))

typedef struct {
	guint signal_id;
} RmNotificationGtkPluginPrivate;

RM_PLUGIN_REGISTER(RM_TYPE_NOTIFICATION_GTK_PLUGIN, RmNotificationGtkPlugin, routermanager_notification_gtk_plugin)

/**
 * \brief Notify accept clicked
 * \param notify notification_gtk window
 * \param user_data user data - connection pointer
 */
static void notify_accept_clicked_cb(GtkWidget *notify, gpointer user_data)
{
	RmConnection *connection = user_data;
	RmContact *contact;

	g_assert(connection != NULL);

	/** Ask for contact information */
	contact = rm_contact_find_by_number(connection->remote_number);

	gtk_widget_destroy(connection->notification);
	connection->notification = NULL;

	app_show_phone_window(contact, connection);
}

/**
 * \brief Deny incoming call
 * \param notify notification_gtk window
 * \param user_data connection pointer
 */
static void notify_deny_clicked_cb(GtkWidget *notify, gpointer user_data)
{
	RmConnection *connection = user_data;

	g_assert(connection != NULL);

	gtk_widget_destroy(connection->notification);
	connection->notification = NULL;

	//faxophone_hangup(connection->priv ? connection->priv : active_capi_connection);
}

/**
 * \brief Close notification_gtk window
 */
static gboolean notification_gtk_close(gpointer window)
{
	gtk_widget_destroy(window);

	return FALSE;
}

/**
 * \brief Reverse lookup of connection data and notification redraw
 * \param data connection pointer
 * \return NULL
 */
static gpointer notification_reverse_lookup_thread(gpointer data)
{
	RmConnection *connection = data;
	gchar *name;
	gchar *address;
	gchar *zip;
	gchar *city;
	gchar *number;

	number = connection->remote_number;

	if (rm_lookup(number, &name, &address, &zip, &city)) {
		if (connection->notification) {
			GtkWidget *contact_name_label = g_object_get_data(connection->notification, "name");
			GtkWidget *contact_street_label = g_object_get_data(connection->notification, "street");
			GtkWidget *contact_city_label = g_object_get_data(connection->notification, "city");
			gchar *tmp;

			gtk_label_set_text(GTK_LABEL(contact_name_label), name ? name : "");
			gtk_label_set_text(GTK_LABEL(contact_street_label), address ? address : "");

			tmp = g_strdup_printf("%s%s%s", zip ? zip : "", zip ? " " : "", city ? city : "");
			gtk_label_set_text(GTK_LABEL(contact_city_label), tmp);
			g_free(tmp);
		}
	}

	return NULL;
}

/**
 * \brief "connection-notify" callback function
 * \param obj app object
 * \param connection connection structure
 * \param unused_pointer unused user pointer
 */
void notification_gtk_connection_notify_cb(RmObject *obj, RmConnection *connection, gpointer unused_pointer)
{
	GtkWidget *notify = NULL;
	GtkWidget *main_frame;
	GtkWidget *main_grid;
	GtkWidget *contact_grid;
	GtkWidget *phone_grid;
	GtkWidget *type_label;
	GtkWidget *local_label;
	GtkWidget *number_label;
	GtkWidget *company_label;
	GtkWidget *direction_label;
	GtkWidget *contact_company_label;
	GtkWidget *contact_number_label;
	GtkWidget *contact_name_label;
	GtkWidget *image;
	GtkWidget *street_label;
	GtkWidget *contact_street_label;
	GtkWidget *city_label;
	GtkWidget *contact_city_label;
	RmContact *contact;
	gchar **numbers = NULL;
	gchar *tmp;
	gint count;
	gboolean found = FALSE;
	gint width, height;
	gint line = 0;
	gint position = 0;

	g_debug("%s(): type = %d", __FUNCTION__, connection->type);
	/* Get notification numbers */
	if (connection->type & RM_CONNECTION_TYPE_OUTGOING) {
		numbers = g_settings_get_strv(notification_gtk_settings, "outgoing-numbers");
	} else if (connection->type & RM_CONNECTION_TYPE_INCOMING) {
		numbers = g_settings_get_strv(notification_gtk_settings, "incoming-numbers");
	}

	/* If numbers are NULL, exit */
	if (!numbers || !g_strv_length(numbers)) {
		g_debug("type: %d, numbers is empty", connection->type);
		return;
	}

	/* Match numbers against local number and check if we should show a notification */
	for (count = 0; count < g_strv_length(numbers); count++) {
		if (!strcmp(connection->local_number, numbers[count])) {
			found = TRUE;
			break;
		}
	}

	if (!found && connection->local_number[0] != '0') {
		gchar *scramble_local = rm_number_scramble(connection->local_number);
		gchar *tmp = rm_number_full(connection->local_number, FALSE);
		gchar *scramble_tmp = rm_number_scramble(tmp);

		g_debug("type: %d, number '%s' not found", connection->type, scramble_local);

		/* Match numbers against local number and check if we should show a notification */
		for (count = 0; count < g_strv_length(numbers); count++) {
			gchar *scramble_number = rm_number_scramble(numbers[count]);

			g_debug("type: %d, number '%s'/'%s' <-> '%s'", connection->type, scramble_local, scramble_tmp, scramble_number);
			g_free(scramble_number);

			if (!strcmp(tmp, numbers[count])) {
				found = TRUE;
				break;
			}
		}

		g_free(scramble_local);
		g_free(scramble_tmp);
		g_free(tmp);
	}

	/* No match found? -> exit */
	if (!found) {
		return;
	}

	/* If its a disconnect or a connect, close previous notification window */
	if ((connection->type & RM_CONNECTION_TYPE_DISCONNECT) || (connection->type & RM_CONNECTION_TYPE_CONNECT)) {
		ringtone_stop();
		g_debug("%s(): notification = %p", __FUNCTION__, connection->notification);
		if (connection->notification) {
			g_debug("%s(): destroy notification = %p", __FUNCTION__, connection->notification);
			gtk_widget_destroy(connection->notification);
			connection->notification = NULL;
		}
		return;
	}

	if (g_settings_get_boolean(notification_gtk_settings, "play-ringtones")) {
		ringtone_play(connection->type);
	}

	/** Ask for contact information */
	contact = rm_contact_find_by_number(connection->remote_number);

	notify = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	main_frame = gtk_frame_new(NULL);

	main_grid = gtk_grid_new();
	gtk_widget_set_margin(main_grid, 10, 10, 10, 10);

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(main_grid), 15);
	gtk_grid_set_column_spacing(GTK_GRID(main_grid), 15);

	contact_grid = gtk_grid_new();
	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(contact_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(contact_grid), 5);

	/* Type */
	type_label = gtk_label_new("");
	gtk_widget_set_hexpand(type_label, TRUE);
	gtk_grid_attach(GTK_GRID(contact_grid), type_label, 0, line, 3, 1);

	/* Local */
	line++;
	local_label = gtk_label_new("");
	gtk_widget_set_hexpand(local_label, TRUE);
	gtk_grid_attach(GTK_GRID(contact_grid), local_label, 0, line, 3, 1);

	image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_DIALOG);
	gtk_widget_set_hexpand(image, TRUE);
	gtk_widget_set_vexpand(image, TRUE);
	gtk_grid_attach(GTK_GRID(contact_grid), image, 0, 2, 1, 4);

	/* Name */
	line++;
	direction_label = gtk_label_new(_("Name:"));
	gtk_widget_set_halign(direction_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), direction_label, 1, line, 1, 1);

	contact_name_label = gtk_label_new("");
	gtk_widget_set_halign(contact_name_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_name_label, 2, line, 1, 1);

	/* Number */
	line++;
	number_label = gtk_label_new(_("Number:"));
	gtk_widget_set_halign(number_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), number_label, 1, line, 1, 1);

	contact_number_label = gtk_label_new("");
	gtk_widget_set_halign(contact_number_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_number_label, 2, line, 1, 1);

	/* Company */
	line++;
	company_label = gtk_label_new(_("Company:"));
	gtk_widget_set_halign(company_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), company_label, 1, line, 1, 1);

	contact_company_label = gtk_label_new("");
	gtk_widget_set_halign(contact_company_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_company_label, 2, line, 1, 1);

	/* Street */
	line++;
	street_label = gtk_label_new(_("Street:"));
	gtk_widget_set_halign(street_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), street_label, 1, line, 1, 1);

	contact_street_label = gtk_label_new("");
	gtk_widget_set_halign(contact_street_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_street_label, 2, line, 1, 1);

	/* City */
	line++;
	city_label = gtk_label_new(_("City:"));
	gtk_widget_set_halign(city_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), city_label, 1, line, 1, 1);

	contact_city_label = gtk_label_new("");
	gtk_widget_set_halign(contact_city_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_city_label, 2, line, 1, 1);

	gtk_grid_attach(GTK_GRID(main_grid), contact_grid, 0, 0, 1, 1);

	gtk_container_add(GTK_CONTAINER(main_frame), main_grid);
	gtk_container_add(GTK_CONTAINER(notify), main_frame);

	tmp = connection->local_number ? g_strdup_printf(_("(on %s)"), connection->local_number) : g_strdup(_("(on ?)"));
	gtk_label_set_text(GTK_LABEL(local_label), tmp);
	g_free(tmp);

	gtk_label_set_text(GTK_LABEL(contact_name_label), contact->name ? contact->name : "");
	gtk_label_set_text(GTK_LABEL(contact_number_label), contact->number ? contact->number : "");
	gtk_label_set_text(GTK_LABEL(contact_company_label), contact->company ? contact->company : "");
	gtk_label_set_text(GTK_LABEL(contact_street_label), contact->street ? contact->street : "");
	tmp = g_strdup_printf("%s%s%s", contact->zip ? contact->zip : "", contact->zip ? " " : "", contact->city ? contact->city : "");
	gtk_label_set_text(GTK_LABEL(contact_city_label), tmp);
	g_free(tmp);

	g_object_set_data(G_OBJECT(notify), "name", contact_name_label);
	g_object_set_data(G_OBJECT(notify), "street", contact_street_label);
	g_object_set_data(G_OBJECT(notify), "city", contact_city_label);

	if (contact->image) {
		GdkPixbuf *buf = image_get_scaled(contact->image, 96, 96);
		gtk_image_set_from_pixbuf(GTK_IMAGE(image), buf);
	}

	if (connection->type == RM_CONNECTION_TYPE_INCOMING) {
		GtkWidget *accept_button;
		GtkWidget *decline_button;

		tmp = ui_bold_text(_("Incoming call"));
		gtk_label_set_markup(GTK_LABEL(type_label), tmp);
		g_free(tmp);

		phone_grid = gtk_grid_new();
		/* Set standard spacing to 5 */
		gtk_grid_set_row_spacing(GTK_GRID(phone_grid), 15);
		gtk_grid_set_column_spacing(GTK_GRID(phone_grid), 5);

		accept_button = gtk_button_new_with_label(_("Accept"));
		g_signal_connect(accept_button, "clicked", G_CALLBACK(notify_accept_clicked_cb), connection);
		gtk_widget_set_hexpand(accept_button, TRUE);
		gtk_grid_attach(GTK_GRID(phone_grid), accept_button, 0, 0, 1, 1);

		decline_button = gtk_button_new_with_label(_("Decline"));
		g_signal_connect(decline_button, "clicked", G_CALLBACK(notify_deny_clicked_cb), connection);
		gtk_widget_set_hexpand(decline_button, TRUE);
		gtk_grid_attach(GTK_GRID(phone_grid), decline_button, 1, 0, 1, 1);

		gtk_grid_attach(GTK_GRID(main_grid), phone_grid, 0, 1, 1, 1);
	} else if (connection->type == RM_CONNECTION_TYPE_OUTGOING) {
		gint duration = g_settings_get_int(notification_gtk_settings, "duration");
		tmp = ui_bold_text(_("Outgoing call"));
		gtk_label_set_markup(GTK_LABEL(type_label), tmp);
		g_free(tmp);

		g_timeout_add_seconds(duration, notification_gtk_close, notify);
	}

	gtk_window_set_decorated(GTK_WINDOW(notify), FALSE);
	gtk_widget_show_all(main_frame);

	gtk_window_get_size(GTK_WINDOW(notify), &width, &height);

	position = g_settings_get_int(notification_gtk_settings, "position");
	switch (position) {
	case 0:
		/* Top Left */
		gtk_window_move(GTK_WINDOW(notify), 0, 0);
		break;
	case 1:
		/* Top Right */
		gtk_window_move(GTK_WINDOW(notify), gdk_screen_width() - width, 0);
		break;
	case 2:
		/* Bottom Left */
		gtk_window_move(GTK_WINDOW(notify), 0, gdk_screen_height() - height);
		break;
	case 3:
		/* Bottom Right */
		gtk_window_move(GTK_WINDOW(notify), gdk_screen_width() - width, gdk_screen_height() - height);
		break;
	}

	gtk_window_stick(GTK_WINDOW(notify));
	gtk_window_set_keep_above(GTK_WINDOW(notify), TRUE);
	gtk_widget_show(notify);

	connection->notification = notify;

	if (RM_EMPTY_STRING(contact->name)) {
		g_thread_new("notification reverse lookup", notification_reverse_lookup_thread, connection);
	}
}

/**
 * \brief Activate plugin (init libnotify and connect to call-notify signal)
 * \param plugin peas plugin
 */
void impl_activate(PeasActivatable *plugin)
{
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
void impl_deactivate(PeasActivatable *plugin)
{
}

#endif