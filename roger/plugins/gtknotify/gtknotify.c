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
#if 0
	RmConnection *connection = user_data;
	RmContact *contact;

	g_assert(connection != NULL);

	/** Ask for contact information */
	contact = rm_contact_find_by_number(connection->remote_number);

	gtk_widget_destroy(connection->notification);
	connection->notification = NULL;

	app_show_phone_window(contact, connection);
#endif
}

/**
 * \brief Deny incoming call
 * \param notify notification_gtk window
 * \param user_data connection pointer
 */
static void notify_deny_clicked_cb(GtkWidget *notify, gpointer user_data)
{
#if 0
	RmConnection *connection = user_data;

	g_assert(connection != NULL);

	gtk_widget_destroy(connection->notification);
	connection->notification = NULL;

	//faxophone_hangup(connection->priv ? connection->priv : active_capi_connection);
#endif
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
 * \brief Close gnotification window
 */
void gtknotify_close(gpointer priv)
{
	gtk_widget_destroy(priv);
}

/**
 * \brief Reverse lookup of connection data and notification redraw
 * \param data connection pointer
 * \return NULL
 */
static gpointer notification_reverse_lookup_thread(gpointer data)
{
#if 0
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
#endif

	return NULL;
}

gpointer gtknotify_show(RmConnection *connection)
{
	GtkWidget *window;
	GtkWidget *type_label;
	GtkWidget *local_label;
	GtkWidget *direction_label;
	GtkWidget *contact_company_label;
	GtkWidget *contact_number_label;
	GtkWidget *contact_name_label;
	GtkWidget *contact_street_label;
	GtkWidget *contact_city_label;
	GtkWidget *image;
	gchar *tmp;
	gint width, height;
	gint line = 0;
	GtkBuilder *builder;
	//RmLookup *lookup = rm_profile_get_lookup(rm_profile_get_active());

	/** Ask for contact information */
	RmContact *contact = rm_contact_find_by_number(connection->remote_number);

	builder = gtk_builder_new_from_resource("/org/tabos/roger/plugins/gtknotify/gtknotify.glade");
	if (!builder) {
		g_warning("Could not load gtknotify ui");
		return;
	}

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	type_label = GTK_WIDGET(gtk_builder_get_object(builder, "type_label"));
	local_label = GTK_WIDGET(gtk_builder_get_object(builder, "local_label"));
	contact_name_label = GTK_WIDGET(gtk_builder_get_object(builder, "name_label"));
	contact_number_label = GTK_WIDGET(gtk_builder_get_object(builder, "number_label"));
	contact_company_label = GTK_WIDGET(gtk_builder_get_object(builder, "company_label"));
	contact_street_label = GTK_WIDGET(gtk_builder_get_object(builder, "street_label"));
	contact_city_label = GTK_WIDGET(gtk_builder_get_object(builder, "city_label"));
	image = GTK_WIDGET(gtk_builder_get_object(builder, "image"));

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

	g_object_set_data(G_OBJECT(window), "name", contact_name_label);
	g_object_set_data(G_OBJECT(window), "street", contact_street_label);
	g_object_set_data(G_OBJECT(window), "city", contact_city_label);

	if (contact->image) {
		//GdkPixbuf *buf = image_get_scaled(contact->image, 96, 96);
		//gtk_image_set_from_pixbuf(GTK_IMAGE(image), buf);

		gtk_image_set_from_pixbuf(GTK_IMAGE(image), contact->image);
	}

	if (connection->type == RM_CONNECTION_TYPE_INCOMING) {
		GtkWidget *accept_button;
		GtkWidget *decline_button;

		tmp = ui_bold_text(_("Incoming call"));

		gtk_label_set_markup(GTK_LABEL(type_label), tmp);
		g_free(tmp);

		accept_button = GTK_WIDGET(gtk_builder_get_object(builder, "pickup_button"));
		g_signal_connect(accept_button, "clicked", G_CALLBACK(notify_accept_clicked_cb), connection);
		gtk_widget_set_visible(accept_button, TRUE);

		decline_button = GTK_WIDGET(gtk_builder_get_object(builder, "hangup_button"));
		g_signal_connect(decline_button, "clicked", G_CALLBACK(notify_deny_clicked_cb), connection);
		gtk_widget_set_visible(decline_button, TRUE);
	} else if (connection->type == RM_CONNECTION_TYPE_OUTGOING) {
		gint duration = 5;
		tmp = ui_bold_text(_("Outgoing call"));
		gtk_label_set_markup(GTK_LABEL(type_label), tmp);
		g_free(tmp);

		g_timeout_add_seconds(duration, notification_gtk_close, window);
	}


	/* Top Right */
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);
	gtk_window_move(GTK_WINDOW(window), gdk_screen_width() - width, gdk_screen_height() - height);

	gtk_window_stick(GTK_WINDOW(window));
	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

	gtk_widget_show(window);

	if (RM_EMPTY_STRING(contact->name)) {
		g_thread_new("notification reverse lookup", notification_reverse_lookup_thread, connection);
	}

	return window;
}

void gtknotify_update(RmConnection *connection)
{
}

RmNotification gtknotify = {
	"GTK Notify",
	gtknotify_show,
	gtknotify_update,
	gtknotify_close,
};

/**
 * \brief Activate plugin (init libnotify and connect to call-notify signal)
 * \param plugin peas plugin
 */
void impl_activate(PeasActivatable *plugin)
{
	g_debug("%s(): gtknotify", __FUNCTION__);
	rm_notification_register(&gtknotify);
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
void impl_deactivate(PeasActivatable *plugin)
{
	g_debug("%s(): gtknotify", __FUNCTION__);
	rm_notification_unregister(&gtknotify);
}
