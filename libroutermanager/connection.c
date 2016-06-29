/**
 * The libroutermanager project
 * Copyright (c) 2012-2014 Jan-Michael Brummer
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

#include <connection.h>

/** Global connect list pointer */
static GSList *connection_list = NULL;

/**
 * \brief Create elapsed time string from two times
 * \param connection connection pointer
 * \return time string
 */
static gchar *timediff_str(struct connection *connection)
{
	gint duration;
	static gchar buf[9];
	gint hours;
	gint minutes;
	gint seconds;
	gdouble time;

	if (!connection->status_timer) {
		strncpy(buf, "00:00:00", sizeof(buf));
		return buf;
	}

	time = g_timer_elapsed(connection->status_timer, NULL);

	duration = (gint) time;
	seconds = duration % 60;
	minutes = (duration / 60) % 60;
	hours = duration / (60 * 60);

	if (snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hours, minutes, seconds) >= 9) {
		buf[8] = '\0';
	}

	return buf;
}

/**
 * \brief Setup connection status timer
 * \param connection connection pointer
 */
void connection_setup_status_timer(struct connection *connection)
{
	if (connection->status_timer) {
		g_debug("%s(): Skip status timer setup, already active", __FUNCTION__);
		return;
	}

	connection->status_timer = g_timer_new();
	g_timer_start(connection->status_timer);
}

/**
 * \brief Remove phone status timer
 * \param connection connection pointer
 */
void connection_remove_status_timer(struct connection *connection)
{
	/* Remove gtimer */
	if (connection->status_timer) {
		g_timer_destroy(connection->status_timer);
		connection->status_timer = NULL;
	}
}

gchar *connection_get_time(struct connection *connection)
{
	return timediff_str(connection);
}

/**
 * \brief Add connection structure to connect list
 * \param id connection id
 */
struct connection *connection_add_call(gint id, gint type, const gchar *local_number, const gchar *remote_number)
{
	struct connection *connection = g_slice_new0(struct connection);
	gchar *scramble_local;
	gchar *scramble_remote;

	connection->id = id;
	connection->type = type;
	connection->local_number = g_strdup(local_number);
	connection->remote_number = g_strdup(remote_number);

	scramble_local = call_scramble_number(connection->local_number);
	scramble_remote = call_scramble_number(connection->remote_number);

	g_debug("Adding connection: type %d, local %s, remote %s", connection->type, scramble_local, scramble_remote);

	g_free(scramble_local);
	g_free(scramble_remote);

	connection_list = g_slist_append(connection_list, connection);

	return connection;
}

/**
 * \brief Find connection entry by id
 * \param id connection id
 * \return connection pointer or NULL on error
 */
struct connection *connection_find_by_id(gint id)
{
	GSList *list = connection_list;
	struct connection *connection;

	while (list) {
		connection = list->data;

		if (connection->id == id) {
			return connection;
		}

		list = list->next;
	}

	return NULL;
}

/**
 * \brief Find connection entry by number
 * \param remote_number connection number
 * \return connection pointer or NULL on error
 */
struct connection *connection_find_by_number(const gchar *remote_number)
{
	GSList *list = connection_list;
	struct connection *connection;

	while (list) {
		connection = list->data;

		if (!strcmp(connection->remote_number, remote_number)) {
			return connection;
		}

		list = list->next;
	}

	return NULL;
}

/**
 * \brief Remove connection from connection list
 * \param connection connection pointer
 */
void connection_remove(struct connection *connection)
{
	g_assert(connection_list != NULL);
	g_assert(connection != NULL);

	connection_list = g_slist_remove(connection_list, connection);
	g_free(connection->local_number);
	g_free(connection->remote_number);

	g_slice_free(struct connection, connection);
}

/**
 * \brief Set connection type
 * \param connection connection pointer
 * \param type additional connection type to add
 */
void connection_set_type(struct connection *connection, gint type)
{
	connection->type |= type;
}
