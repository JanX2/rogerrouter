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

#include <glib.h>
#include <glib/gstdio.h>

#include <unistd.h>
#include <errno.h>

#include <gio/gio.h>

#include <rmconfig.h>

#include <rmobjectemit.h>
#include <rmfaxserver.h>
#include <rmmain.h>

#define BUFFER_LENGTH 1024

/**
 * rm_faxserver_thread:
 * @data: a #GSocket
 *
 * Server thread which accepts new data as fax files
 *
 * Returns: %NULL
 */
gpointer rm_faxserver_thread(gpointer data)
{
	GSocket *server = data;
	GSocket *sock;
	GError *error = NULL;
	gsize len;
	gchar *file_name;
	char buffer[BUFFER_LENGTH];
	ssize_t write_result;
	ssize_t written;

	while (TRUE) {
		sock = g_socket_accept(server, NULL, &error);
		g_assert_no_error(error);

		file_name = g_build_filename(rm_get_user_cache_dir(), "fax-XXXXXX", NULL);
		int file_id = g_mkstemp(file_name);

		if (file_id == -1) {
			g_warning("%s(): Can't open temporary file '%s'", __FUNCTION__, file_name);
			g_free(file_name);
			continue;
		}

		g_debug("%s(): file: %s (%d)", __FUNCTION__, file_name, file_id);

		do {
			len = g_socket_receive(sock, buffer, BUFFER_LENGTH, NULL, &error);

			if (len > 0) {
				written = 0;
				do {
					write_result = write(file_id, buffer + written, len);
					if (write_result > 0) {
						written += write_result;
					}
				} while (len != written && (write_result != -1 || errno == EINTR));
			}
		} while (len > 0);

		if (len == 0) {
			g_close(file_id, &error);
		 	g_debug("%s(): Print job received on socket (%s)", __FUNCTION__, file_name);

			rm_object_emit_fax_process(file_name);
		}

		g_socket_close(sock, &error);
		g_assert_no_error(error);
	}

	return NULL;
}

/**
 * rm_faxserver_init:
 *
 * Initialize fax network spooler
 *
 * Returns: %TRUE if network spooler could be started, %FALSE on error
 */
gboolean rm_faxserver_init(void)
{
	GSocket *socket = NULL;
	GInetAddress *inet_address = NULL;
	GSocketAddress *sock_address = NULL;
	GError *fax_error = NULL;

	socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &fax_error);
	if (socket == NULL) {
		g_warning("%s(): %s'", __FUNCTION__, fax_error->message);
		g_error_free(fax_error);
		return FALSE;
	}

	inet_address = g_inet_address_new_loopback(G_SOCKET_FAMILY_IPV4);

	sock_address = g_inet_socket_address_new(inet_address, 9100);
	if (sock_address == NULL) {
		g_warning("%s(): Could not create sock address on port 9100", __FUNCTION__);
		g_object_unref(socket);
		return FALSE;
	}

	if (g_socket_bind(socket, sock_address, TRUE, &fax_error) == FALSE) {
		g_warning("%s(): %s", __FUNCTION__, fax_error->message);
		g_error_free(fax_error);
		g_object_unref(socket);
		return FALSE;
	}

	if (g_socket_listen(socket, &fax_error) == FALSE) {
		g_warning("%s(): Error: %s", __FUNCTION__, fax_error->message);
		g_error_free(fax_error);
		g_object_unref(socket);
		return FALSE;
	}

	g_debug("%s(): Fax Server running on port 9100", __FUNCTION__);

	g_thread_new("printserver", rm_faxserver_thread, socket);

	return TRUE;
}

