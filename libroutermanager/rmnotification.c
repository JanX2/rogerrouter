#include <glib.h>

#include <libroutermanager/rmconnection.h>
#include <libroutermanager/rmobject.h>
#include <libroutermanager/rmnotification.h>

static GSList *rm_notification_plugins = NULL;
static gint rm_notification_signal_id = 0;

RmNotification *rm_notification_new(void)
{
	return g_slice_alloc0(sizeof(RmNotification));
}

void rm_notification_remove(RmNotification *notification)
{
	g_slice_free(RmNotification, notification);
}

void rm_notification_add_connection(RmNotification *notification, RmConnection *connection)
{
	notification->connection = connection;
}

void rm_notification_set_uid(RmNotification *notification, gpointer uid)
{
	notification->priv = uid;
}

void rm_notification_register(RmNotificationPlugin *plugin)
{
	rm_notification_plugins = g_slist_prepend(rm_notification_plugins, plugin);
}

void rm_notification_unregister(RmNotificationPlugin *plugin)
{
	rm_notification_plugins = g_slist_remove(rm_notification_plugins, plugin);
}

void rm_notification_connection_notify_cb(RmObject *obj, RmConnection *connection, gpointer unused_pointer)
{
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
		numbers = NULL;//g_settings_get_strv(notification_gtk_settings, "outgoing-numbers");
	} else if (connection->type & RM_CONNECTION_TYPE_INCOMING) {
		numbers = NULL;//g_settings_get_strv(notification_gtk_settings, "incoming-numbers");
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
		gchar *scramble_local = rm_call_scramble_number(connection->local_number);
		gchar *tmp = rm_call_full_number(connection->local_number, FALSE);
		gchar *scramble_tmp = rm_call_scramble_number(tmp);

		g_debug("type: %d, number '%s' not found", connection->type, scramble_local);

		/* Match numbers against local number and check if we should show a notification */
		for (count = 0; count < g_strv_length(numbers); count++) {
			gchar *scramble_number = rm_call_scramble_number(numbers[count]);

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
		//ringtone_stop();

		/*if (connection->notification) {
			g_debug("%s(): destroy notification = %p", __FUNCTION__, connection->notification);
			gtk_widget_destroy(connection->notification);
			connection->notification = NULL;
		}*/
		return;
	}

	/*if (g_settings_get_boolean(notification_gtk_settings, "play-ringtones")) {
		ringtone_play(connection->type);
	}*/
}

void rm_notification_init(void)
{
	/*gchar **incoming_numbers = g_settings_get_strv(rm_settings, "notification-incoming-numbers");
	gchar **outgoing_numbers = g_settings_get_strv(rm_settings, "notification-outgoing-numbers");

	if ((!incoming_numbers || !g_strv_length(incoming_numbers)) && (!outgoing_numbers || !g_strv_length(outgoing_numbers))) {
		g_settings_set_strv(rm_settings, "notification-incoming-numbers", (const gchar * const *) router_get_numbers(rm_profile_get_active()));
		g_settings_set_strv(rm_settings, "notification-outgoing-numbers", (const gchar * const *) router_get_numbers(rm_profile_get_active()));
	}*/

	/* Connect to "call-notify" signal */
	rm_notification_signal_id = g_signal_connect(G_OBJECT(rm_object), "connection-notify", G_CALLBACK(rm_notification_connection_notify_cb), NULL);
}

