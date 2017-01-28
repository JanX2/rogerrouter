/*
 * The rm project
 * Copyright (c) 2012-2017 Jan-Michael Brummer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>

#include <glib.h>

#include <rm/rmconnection.h>
#include <rm/rmobject.h>
#include <rm/rmnotification.h>
#include <rm/rmprofile.h>
#include <rm/rmcallentry.h>
#include <rm/rmnumber.h>
#include <rm/rmrouter.h>
#include <rm/rmlookup.h>
#include <rm/rmstring.h>

/**
 * SECTION:rmnotification
 * @Title: RmNotification
 * @Short_description: Notification center - Keeps track of messages and displays them to the user
 *
 * Sends/Closes notification message and keeps tracks of messages and notification plugins.
 */

static GSList *rm_notification_plugins = NULL;
static gint rm_notification_signal_id = 0;
static GSList *rm_notification_messages = NULL;

/**
 * rm_notification_message_get:
 * @connection: a #RmConnection
 *
 * Get notification message with attached @connection.
 *
 * Returns: a #RmNotificationMessage or %NULL if not found
 */
RmNotificationMessage *rm_notification_message_get(RmConnection *connection)
{
	GSList *list;

	for (list = rm_notification_messages; list != NULL; list = list->next) {
		RmNotificationMessage *message = list->data;

		if (message && message->connection == connection) {
			return message;
		}
	}

	return NULL;
}

/**
 * rm_notification_show_message:
 * @notification: a #RmNotification
 * @connection: a #RmConnection
 * @contact: a #RmContact
 *
 * Creates a #RmNotificationMessage with linked @connection and shows them to the user through @notification.
 */
static void rm_notification_show_message(RmNotification *notification, RmConnection *connection, RmContact *contact)
{
	RmNotificationMessage *message;

	message = g_slice_alloc0(sizeof(RmNotificationMessage));
	message->connection = connection;
	message->notification = notification;

	rm_notification_messages = g_slist_prepend(rm_notification_messages, message);

	message->priv = notification->show(connection, contact);
}

/**
 * rm_notification_update_message:
 * @message: a #RmNotificationMessage
 * @contact: a #RmContact
 *
 * Update notification message
 */
static void rm_notification_update_message(RmNotificationMessage *message, RmContact *contact)
{
	message->notification->update(message->connection, contact);
}

/**
 * rm_notification_message_close:
 * @message: a #RmNotificationMessage
 *
 * Close #RmNotificationMessages defined by @connection and frees it.
 */
void rm_notification_message_close(RmNotificationMessage *message)
{
	if (!message) {
		return;
	}

	message->notification->close(message->priv);

	rm_notification_messages = g_slist_remove(rm_notification_messages, message);

	g_slice_free(RmNotificationMessage, message);
}

/**
 * rm_notification_get:
 * @name: name of notification service
 *
 * Get notification service with provided @name.
 *
 * Returns: a #RmNotification or %NULL if not found.
 */
RmNotification *rm_notification_get(gchar *name)
{
	GSList *list;

	/* Loop through list and try to find notification plugin */
	for (list = rm_notification_plugins; list != NULL; list = list->next) {
		RmNotification *notification = list->data;

		if (notification && notification->name && name && !strcmp(notification->name, name)) {
			return notification;
		}
	}

	return NULL;
}

/**
 * rm_notification_reverse_lookup_thread:
 * @data: a #RmConnection
 *
 * Reverse lookup of connection data and notification redraw
 *
 * Returns: %NULL
 */
static gpointer rm_notification_reverse_lookup_thread(gpointer data)
{
	RmConnection *connection = data;
	RmContact contact;
	gchar *number;

	number = connection->remote_number;

	if (rm_lookup_search(rm_profile_get_lookup(rm_profile_get_active()), number, &contact)) {
		RmNotificationMessage *message = rm_notification_message_get(connection);

		if (message) {
			rm_notification_update_message(message, &contact);
		}
	}

	return NULL;
}

/**
 * rm_notification_connection_notify_cb:
 * @obj: a #RmObject
 * @event: a #RmConnectionType
 * @connection: a #RmConnection
 * @unused_pointer: UNUSED pointer
 *
 * Notification handling based on ::conection-notify signal. Shows/Closes notifications for new connections.
 */
static void rm_notification_connection_changed_cb(RmObject *obj, gint event, RmConnection *connection, gpointer unused_pointer)
{
	RmProfile *profile = rm_profile_get_active();
	RmNotification *notification;
	gchar **numbers = NULL;
	gint count;
	gboolean found = FALSE;

	if (!profile) {
		return;
	}

	notification = rm_profile_get_notification(profile);
	if (!notification) {
		notification = rm_notification_plugins->data;
		numbers = rm_router_get_numbers(profile);
		g_warning("%s(): TODO", __FUNCTION__);
		//return;
	}

	/* Get notification numbers */
	if (connection->type & RM_CONNECTION_TYPE_OUTGOING) {
		numbers = rm_profile_get_notification_outgoing_numbers(profile);
		g_debug("%s(): Outgoing", __FUNCTION__);
	} else if (connection->type & RM_CONNECTION_TYPE_INCOMING) {
		numbers = rm_profile_get_notification_incoming_numbers(profile);
		g_debug("%s(): Incoming", __FUNCTION__);
	} else {
		g_warning("%s(): Type does not include incoming or outgoing: %d", __FUNCTION__, connection->type);
		return;
	}

	/* If numbers are NULL, exit */
	if (!numbers || !g_strv_length(numbers)) {
		g_debug("%s(): type = %d, numbers is empty", __FUNCTION__, connection->type);
		return;
	}

	/* Match numbers against local number and check if we should show a notification */
	for (count = 0; count < g_strv_length(numbers); count++) {
		g_debug("%s(): Checking %s <-> %s", __FUNCTION__, connection->local_number, numbers[count]);
		if (!strcmp(connection->local_number, numbers[count])) {
			found = TRUE;
			break;
		}
	}

	if (!found && connection->local_number[0] != '0') {
		gchar *scramble_local = rm_number_scramble(connection->local_number);
		gchar *tmp = rm_number_full(connection->local_number, FALSE);
		gchar *scramble_tmp = rm_number_scramble(tmp);

		g_debug("%s(): type = %d, number = '%s' not found", __FUNCTION__, connection->type, scramble_local);

		/* Match numbers against local number and check if we should show a notification */
		for (count = 0; count < g_strv_length(numbers); count++) {
			gchar *scramble_number = rm_number_scramble(numbers[count]);

			g_debug("%s(): type = %d, number = '%s'/'%s' <-> '%s'", __FUNCTION__, connection->type, scramble_local, scramble_tmp, scramble_number);
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
		RmNotificationMessage *message = rm_notification_message_get(connection);

		if (message) {
			rm_notification_message_close(message);
		}
		return;
	}

	if (rm_profile_get_notification_ringtone(profile)) {
		//ringtone_play(connection->type);
	}

	RmContact *contact = rm_contact_find_by_number(connection->remote_number);

	rm_notification_show_message(notification, connection, contact);

	if (RM_EMPTY_STRING(contact->name)) {
		g_thread_new("rmnotification reverse lookup", rm_notification_reverse_lookup_thread, connection);
	}
}

/**
 * rm_notification_register:
 * @notification: a #RmNotification
 *
 * Register a new notification.
 */
void rm_notification_register(RmNotification *notification)
{
	rm_notification_plugins = g_slist_prepend(rm_notification_plugins, notification);
}

/**
 * rm_notification_unregister:
 * @notification: a #RmNotification
 *
 * Unregister a notification.
 */
void rm_notification_unregister(RmNotification *notification)
{
	rm_notification_plugins = g_slist_remove(rm_notification_plugins, notification);
}

/**
 * rm_notification_init:
 *
 * Initializes notification handling. Connects to ::connection-notify signal.
 */
void rm_notification_init(void)
{
	/* Connect to "connection-changed" signal */
	rm_notification_signal_id = g_signal_connect(G_OBJECT(rm_object), "connection-changed", G_CALLBACK(rm_notification_connection_changed_cb), NULL);
}

/**
 * rm_notification_shutdown:
 *
 * Shuts notification service down. Disconnects from ::connection-notify signal.
 */
void rm_notification_shutdown(void)
{
	if (rm_notification_signal_id) {
		g_signal_handler_disconnect(G_OBJECT(rm_object), rm_notification_signal_id);
	}
}

/**
 * rm_notification_get_plugins:
 *
 * Get notification plugin list.
 *
 * Returns: a GSList of notification plugins.
 */
GSList *rm_notification_get_plugins(void)
{
	return rm_notification_plugins;
}

/**
 * rm_notification_get_name:
 * @notification: a #RmNotification
 *
 * Get name of @notification.
 *
 * Returns: name of notification
 */
gchar *rm_notification_get_name(RmNotification *notification)
{
	return g_strdup(notification->name);
}
