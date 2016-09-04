/**
 * The libroutermanager project
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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
#include <stdio.h>

#include <glib.h>

#include <libroutermanager/rmconnection.h>

/**
 * SECTION:rmconnection
 * @title: RmConnection
 * @short_description: Connection handling functions
 *
 * Connections are used to keep track of ongoing calls (phone/fax/...).
 */

#define RM_CONNECTION_DEBUG 1

/** Global connect list pointer */
static GSList *rm_connection_list = NULL;

/**
 * rm_connection_init_duration_timer:
 * @connection: a #RmConnection
 *
 * Setup connection duration timer.
 */
void rm_connection_init_duration_timer(RmConnection *connection)
{
	if (connection->duration_timer) {
		g_warning("%s(): Skip duration timer setup, already active", __FUNCTION__);

		return;
	}

	connection->duration_timer = g_timer_new();
}

/**
 * rm_connection_shutdown_duration_timer:
 * @connection: a #RmConnection
 *
 * Remove phone duration timer.
 */
void rm_connection_shutdown_duration_timer(RmConnection *connection)
{
	/* Remove timer */
	if (connection->duration_timer) {
		g_timer_destroy(connection->duration_timer);
		connection->duration_timer = NULL;
	}
}

/**
 * rm_connection_get_duration_time:
 * @connection: a #RmConnection
 *
 * Retrieves duration time for given @connection.
 *
 * Returns: (transfer full) duration time string
 */
gchar *rm_connection_get_duration_time(RmConnection *connection)
{
	gint duration;
	gint hours;
	gint minutes;
	gint seconds;
	gdouble time;

	/* In case we do not have a duration timer, return predefined string */
	if (!connection->duration_timer) {
		return g_strdup("00:00:00");
	}

	time = g_timer_elapsed(connection->duration_timer, NULL);

	duration = (gint) time;
	seconds = duration % 60;
	minutes = (duration / 60) % 60;
	hours = duration / (60 * 60);

	return g_strdup_printf("%02d:%02d:%02d", hours, minutes, seconds);
}

/**
 * rm_connection_add:
 * @id: unique connection id
 * @type: type of #RmConnectionType
 * @local_number: local phone number (callee)
 * @remote_number: remote phone number (caller)
 *
 * Create and add a new #RmConnection to connection list
 */
RmConnection *rm_connection_add(gint id, RmConnectionType type, const gchar *local_number, const gchar *remote_number)
{
	RmConnection *connection = g_slice_new0(RmConnection);

	connection->id = id;
	connection->type = type;
	connection->local_number = g_strdup(local_number);
	connection->remote_number = g_strdup(remote_number);

	rm_connection_list = g_slist_append(rm_connection_list, connection);

#ifdef RM_CONNECTION_DEBUG
	{
		gchar *scramble_local;
		gchar *scramble_remote;

		scramble_local = rm_call_scramble_number(connection->local_number);
		scramble_remote = rm_call_scramble_number(connection->remote_number);

		g_debug("Adding connection: type %d, local %s, remote %s", connection->type, scramble_local, scramble_remote);

		g_free(scramble_local);
		g_free(scramble_remote);
	}
#endif

	return connection;
}

/**
 * rm_connection_find_by_id:
 * @id: unique connection id
 *
 * Find #RmConnection by @id.
 *
 * Returns: a #RmConnection, or NULL if not found.
 */
RmConnection *rm_connection_find_by_id(gint id)
{
	GSList *list = rm_connection_list;
	RmConnection *connection;

	while (list != NULL) {
		connection = list->data;

		if (connection && connection->id == id) {
			return connection;
		}

		list = list->next;
	}

	return NULL;
}

/**
 * rm_connection_find_by_remote_number:
 * @remote_number: remote phone number
 *
 * Find #RmConnection by @remote_number.
 *
 * Returns: a #RmConnection, or NULL if not found.
 */
RmConnection *rm_connection_find_by_remote_number(const gchar *remote_number)
{
	GSList *list = rm_connection_list;
	RmConnection *connection;

	g_return_val_if_fail(remote_number != NULL, NULL);

	while (list != NULL) {
		connection = list->data;

		if (connection && connection->remote_number && !strcmp(connection->remote_number, remote_number)) {
			return connection;
		}

		list = list->next;
	}

	return NULL;
}

/**
 * rm_connection_remove:
 * @connection: a #RmConnection
 *
 * Removes @connection from connection list and frees structure.
 */
void rm_connection_remove(RmConnection *connection)
{
	g_assert(rm_connection_list != NULL);
	g_assert(connection != NULL);

	rm_connection_list = g_slist_remove(rm_connection_list, connection);
	g_free(connection->local_number);
	g_free(connection->remote_number);

	g_slice_free(RmConnection, connection);
}

/**
 * rm_connection_set_type:
 * @connection: a #RmConnection
 * @type: a RmConnectionType
 *
 * Set additional connection type.
 */
void rm_connection_set_type(RmConnection *connection, RmConnectionType type)
{
	connection->type |= type;
}
