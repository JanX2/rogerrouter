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

#include <libroutermanager/rmobject.h>
#include <libroutermanager/rmobjectemit.h>
#include <libroutermanager/rmconnection.h>

/**
 * rm_object_emit_connection_notify:
 * @connection: a #RmConnection
 *
 * Emit signal: connection-notify.
 */
void rm_object_emit_connection_notify(RmConnection *connection)
{
	g_signal_emit(rm_object, rm_object_signals[RM_ACB_CONNECTION_NOTIFY], 0, connection);
}

/**
 * rm_object_emit_journal_loaded
 * @journal: new journal that needs to be handled by application
 *
 * Emit signal: journal-loaded.
 */
void rm_object_emit_journal_loaded(GSList *journal)
{
	if (rm_object) {
		g_signal_emit(rm_object, rm_object_signals[RM_ACB_JOURNAL_LOADED], 0, journal);
	}
}

/**
 * rm_object_emit_contact_process:
 * @contact: a #RmContact
 *
 * Emit signal: contact-process.
 */
void rm_object_emit_contact_process(RmContact *contact)
{
	g_signal_emit(rm_object, rm_object_signals[RM_ACB_CONTACT_PROCESS], 0, contact);
}

/**
 * rm_object_emit_fax_process:
 * @filename: fax filename in spooler directory
 *
 * Emit signal: process-fax.
 */
void rm_object_emit_fax_process(const gchar *filename)
{
	g_signal_emit(rm_object, rm_object_signals[RM_ACB_FAX_PROCESS], 0, filename);
}

/**
 * rm_object_emit_connection_established:
 * @connection: a #RmConnection
 *
 * Emit signal: connection-established.
 */
void rm_object_emit_connection_established(RmConnection *connection)
{
	rm_connection_init_duration_timer(connection);

	g_signal_emit(rm_object, rm_object_signals[RM_ACB_CONNECTION_ESTABLISHED], 0, connection);
}

/**
 * rm_object_emit_connection_terminated:
 * @connection: a #RmConnection
 *
 * Emit signal: connection-terminated.
 */
void rm_object_emit_connection_terminated(RmConnection *connection)
{
	/* Remove timer */
	rm_connection_shutdown_duration_timer(connection);

	g_signal_emit(rm_object, rm_object_signals[RM_ACB_CONNECTION_TERMINATED], 0, connection);
}

/**
 * rm_object_emit_connection_status:
 * @status: connection status
 * @connection: a #RmConnection
 *
 * Emit signal: connection-status.
 */
void rm_object_emit_connection_status(gint status, struct connection *connection)
{
	g_signal_emit(rm_object, rm_object_signals[RM_ACB_CONNECTION_STATUS], 0, status, connection);
}

/**
 * rm_object_emit_message:
 * @title: title text
 * @message: message text
 *
 * Emit signal: message.
 */
void rm_object_emit_message(gchar *title, gchar *message)
{
	g_signal_emit(rm_object, rm_object_signals[RM_ACB_MESSAGE], 0, g_strdup(title), g_strdup(message));
}

/**
 * rm_object_emit_contacts_changed:
 *
 * Emit signal: contacts-changed.
 */
void rm_object_emit_contacts_changed(void)
{
	g_signal_emit(rm_object, rm_object_signals[RM_ACB_CONTACTS_CHANGED], 0);
}

/**
 * rm_object_emit_authenticate:
 * @auth_data: a #RmAuthData
 *
 * Emit signal: authenticate.
 */
void rm_object_emit_authenticate(RmAuthData *auth_data)
{
	g_signal_emit(rm_object, rm_object_signals[RM_ACB_AUTHENTICATE], 0, auth_data);
}
