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

/**
 * \TODO List
 * - Reduce all connection_xxx function to connection_notify
 */

#include <libroutermanager/appobject.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/connection.h>

/**
 * \brief Emit signal: connection-notify
 * \param connection connection structure
 */
void emit_connection_notify(struct connection *connection)
{
	g_signal_emit(app_object, app_object_signals[ACB_CONNECTION_NOTIFY], 0, connection);
}

/**
 * \brief Emit signal: journal-loaded
 * \param journal new journal needs to be handled by application
 */
void emit_journal_loaded(GSList *journal)
{
	if (app_object) {
		g_signal_emit(app_object, app_object_signals[ACB_JOURNAL_LOADED], 0, journal);
	}
}

/**
 * \brief Emit signal: contact-process
 * \param contact contact which needs to be processed
 */
void emit_contact_process(struct contact *contact)
{
	g_signal_emit(app_object, app_object_signals[ACB_CONTACT_PROCESS], 0, contact);
}

/**
 * \brief Emit signal: process-fax
 * \param filename fax filename in spooler directory
 */
void emit_fax_process(const gchar *filename)
{
	g_signal_emit(app_object, app_object_signals[ACB_FAX_PROCESS], 0, filename);
}

/**
 * \brief Emit signal: connection-established
 * \param connection capi connection pointer
 */
void emit_connection_established(struct capi_connection *connection)
{
	g_signal_emit(app_object, app_object_signals[ACB_CONNECTION_ESTABLISHED], 0, connection);
}

/**
 * \brief Emit signal: connection-terminated
 * \param connection capi connection pointer
 */
void emit_connection_terminated(struct capi_connection *connection)
{
	g_signal_emit(app_object, app_object_signals[ACB_CONNECTION_TERMINATED], 0, connection);
}

/**
 * \brief Emit signal: connection-status
 * \param status connection status
 * \param connection capi connection pointer
 */
void emit_connection_status(gint status, struct capi_connection *connection)
{
	g_signal_emit(app_object, app_object_signals[ACB_CONNECTION_STATUS], 0, status, connection);
}

/**
 * \brief Emit signal: message
 * \param type message type
 * \param message message text
 */
void emit_message(gint type, gchar *message)
{
	g_signal_emit(app_object, app_object_signals[ACB_MESSAGE], 0, type, g_strdup(message));
}
