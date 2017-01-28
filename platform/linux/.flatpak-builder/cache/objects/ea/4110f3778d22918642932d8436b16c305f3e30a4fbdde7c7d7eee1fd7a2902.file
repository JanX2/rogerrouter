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
