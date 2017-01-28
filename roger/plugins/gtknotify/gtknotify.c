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

#include <gtk/gtk.h>

#include <rm/rmplugins.h>
#include <rm/rmconnection.h>
#include <rm/rmcontact.h>
#include <rm/rmnotification.h>
#include <rm/rmstring.h>

#include <roger/main.h>
#include <roger/uitools.h>

#define RM_TYPE_NOTIFICATION_GTK_PLUGIN (routermanager_notification_gtk_plugin_get_type ())
#define RM_NOTIFICATION_GTK_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), RM_TYPE_NOTIFICATION_GTK_PLUGIN, RmNotificationGtkPlugin))

typedef struct {
	guint signal_id;
} RmNotificationGtkPluginPrivate;

RM_PLUGIN_REGISTER(RM_TYPE_NOTIFICATION_GTK_PLUGIN, RmNotificationGtkPlugin, routermanager_notification_gtk_plugin)

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

gpointer gtknotify_show(RmConnection *connection, RmContact *contact)
{
	GtkWidget *window;
	GtkWidget *type_label;
	GtkWidget *local_label;
	GtkWidget *contact_company_label;
	GtkWidget *contact_number_label;
	GtkWidget *contact_name_label;
	GtkWidget *contact_street_label;
	GtkWidget *contact_city_label;
	GtkWidget *image;
	gchar *tmp;
	GtkBuilder *builder;

	builder = gtk_builder_new_from_resource("/org/tabos/roger/plugins/gtknotify/gtknotify.glade");
	if (!builder) {
		g_warning("Could not load gtknotify ui");
		return NULL;
	}

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	gtk_application_add_window(GTK_APPLICATION(g_application_get_default()), GTK_WINDOW(window));

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

	if (connection->type & RM_CONNECTION_TYPE_INCOMING) {
		tmp = ui_bold_text(_("Incoming call"));

		gtk_label_set_markup(GTK_LABEL(type_label), tmp);
		g_free(tmp);

		if (connection->type & RM_CONNECTION_TYPE_SOFTPHONE) {
			GtkWidget *accept_button;
			GtkWidget *decline_button;

			accept_button = GTK_WIDGET(gtk_builder_get_object(builder, "pickup_button"));
			gtk_actionable_set_action_name(GTK_ACTIONABLE(accept_button), "app.pickup");
			gtk_actionable_set_action_target(GTK_ACTIONABLE(accept_button), "i", connection->id);
			gtk_widget_set_visible(accept_button, TRUE);

			decline_button = GTK_WIDGET(gtk_builder_get_object(builder, "hangup_button"));
			gtk_actionable_set_action_name(GTK_ACTIONABLE(decline_button), "app.hangup");
			gtk_actionable_set_action_target(GTK_ACTIONABLE(decline_button), "i", connection->id);
			gtk_widget_set_visible(decline_button, TRUE);
		}
	} else if (connection->type == RM_CONNECTION_TYPE_OUTGOING) {
		gint duration = 5;
		tmp = ui_bold_text(_("Outgoing call"));
		gtk_label_set_markup(GTK_LABEL(type_label), tmp);
		g_free(tmp);

		g_timeout_add_seconds(duration, notification_gtk_close, window);
	}

	gtk_window_stick(GTK_WINDOW(window));
	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

	gtk_widget_show(window);

	return window;
}

void gtknotify_update(RmConnection *connection, RmContact *contact)
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
