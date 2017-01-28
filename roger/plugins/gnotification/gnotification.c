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

#include <gio/gio.h>
#include <gtk/gtk.h>

#include <rm/rmplugins.h>
#include <rm/rmcallentry.h>
#include <rm/rmobject.h>
#include <rm/rmobjectemit.h>
#include <rm/rmphone.h>
#include <rm/plugins/capi/ringtone.h>
#include <rm/rmrouter.h>
#include <rm/rmprofile.h>
#include <rm/rmlookup.h>
#include <rm/rmstring.h>
#include <rm/rmsettings.h>
#include <rm/rmnotification.h>

#include <roger/main.h>
#include <roger/application.h>
#include <roger/phone.h>
#include <roger/settings.h>
#include <roger/uitools.h>

#define RM_TYPE_GNOTIFICATION_PLUGIN (rm_gnotification_plugin_get_type ())
#define RM_GNOTIFICATION_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), RM_TYPE_GNOTIFICATION_PLUGIN, RmGNotificationPlugin))

typedef struct {
	guint signal_id;
} RmGNotificationPluginPrivate;

RM_PLUGIN_REGISTER(RM_TYPE_GNOTIFICATION_PLUGIN, RmGNotificationPlugin, rm_gnotification_plugin)

/**
 * \brief Close gnotification window
 */
void gnotification_close(gpointer priv)
{
	g_application_withdraw_notification(G_APPLICATION(g_application_get_default()), priv);
}

gboolean gnotification_timeout_close(gpointer priv)
{
	gnotification_close(priv);

	return G_SOURCE_REMOVE;
}

void gnotification_show_missed_calls(void)
{
	GNotification *notify = NULL;
	gchar *text = NULL;

	g_application_withdraw_notification(G_APPLICATION(g_application_get_default()), "missed-calls");

	/* Create notification message */
	text = g_strdup_printf(_("You have missed calls"));

	notify = g_notification_new(_("Missed call(s)"));

	g_notification_set_body(notify, text);
	g_free(text);

	g_notification_add_button_with_target(notify, _("Open journal"), "app.journal", NULL);
	g_application_send_notification(G_APPLICATION(g_application_get_default()), "missed-calls", notify);
	g_object_unref(notify);
}

gpointer gnotification_show(RmConnection *connection, RmContact *contact)
{
	GNotification *notify = NULL;
	GIcon *icon;
	gchar *title;
	gchar *text;
	gchar *uid;

	g_debug("%s(): called", __FUNCTION__);

	if (connection->type != RM_CONNECTION_TYPE_INCOMING && connection->type != RM_CONNECTION_TYPE_OUTGOING) {
		g_warning("Unhandled case in connection notify - gnotification!");

		return NULL;
	}
	/* Create notification message */
	text = g_markup_printf_escaped(_("Name:\t\t%s\nNumber:\t\t%s\nCompany:\t%s\nStreet:\t\t%s\nCity:\t\t\t%s%s%s\n"),
	                               contact->name ? contact->name : "",
	                               contact->number ? contact->number : "",
	                               contact->company ? contact->company : "",
	                               contact->street ? contact->street : "",
	                               contact->zip ? contact->zip : "",
	                               contact->zip ? " " : "",
	                               contact->city ? contact->city : ""
	                              );

	if (connection->type == RM_CONNECTION_TYPE_INCOMING) {
		title = g_strdup_printf(_("Incoming call (on %s)"), connection->local_number);
	} else {
		title = g_strdup_printf(_("Outgoing call (on %s)"), connection->local_number);
	}

	notify = g_notification_new(title);
	g_free(title);

	g_notification_set_body(notify, text);
	g_free(text);

	uid = g_strdup_printf("%s%s", connection->local_number, contact->number);

	if (connection->type == (RM_CONNECTION_TYPE_INCOMING | RM_CONNECTION_TYPE_SOFTPHONE)) {
		g_notification_add_button_with_target(notify, _("Accept"), "app.pickup", "i", connection->id);
		g_notification_add_button_with_target(notify, _("Decline"), "app.hangup", "i", connection->id);
	} else if (connection->type == RM_CONNECTION_TYPE_OUTGOING) {
		gint duration = 5;

		g_timeout_add_seconds(duration, gnotification_timeout_close, uid);
	}

	if (contact->image) {
		icon = G_ICON(contact->image);
		g_notification_set_icon(notify, icon);
	}

	g_notification_set_priority(notify, G_NOTIFICATION_PRIORITY_URGENT);
	g_application_send_notification(G_APPLICATION(g_application_get_default()), uid, notify);
	g_object_unref(notify);

	return uid;
}

void gnotification_update(RmConnection *connection, RmContact *contact)
{
}

RmNotification gnotification = {
	"GNotification",
	gnotification_show,
	gnotification_update,
	gnotification_close,
};

/**
 * \brief Activate plugin
 * \param plugin peas plugin
 */
void impl_activate(PeasActivatable *plugin)
{
	g_debug("%s(): gnotification", __FUNCTION__);
	rm_notification_register(&gnotification);
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
void impl_deactivate(PeasActivatable *plugin)
{
	g_debug("%s(): gnotification", __FUNCTION__);
	g_application_withdraw_notification(G_APPLICATION(g_application_get_default()), "missed-calls");

	rm_notification_unregister(&gnotification);
}
