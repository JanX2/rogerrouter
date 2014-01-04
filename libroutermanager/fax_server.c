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

#include <glib.h>

#include <unistd.h>
#include <errno.h>

#include <gio/gio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <appobject-emit.h>
#include <fax_printer.h>

#ifdef USE_PRINTER_SERVER

#define BUFFER_LENGTH 1024

gpointer print_server_thread(gpointer data)
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

		file_name = g_strdup_printf("%s/fax-XXXXXX", g_get_tmp_dir());
		int file_id = g_mkstemp(file_name);

		if (file_id == -1) {
			g_warning("Can't open temporary file");
			continue;
		}

		g_debug("file: %s (%d)", file_name, file_id);

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
			g_debug("Print job received on socket");

			emit_fax_process(file_name);
		}

		g_socket_close(sock, &error);
		g_assert_no_error(error);
	}

	return NULL;
}

gboolean fax_printer_init(GError **error)
{
	GSocket *socket = NULL;
	GInetAddress *inet_address = NULL;
	GSocketAddress *sock_address = NULL;
	GError *fax_error = NULL;

	socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &fax_error);
	if (socket == NULL) {
		g_debug("Could not create socket. Error: '%s'", fax_error->message);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "Could not create socket. Error: '%s'", fax_error ? fax_error->message : "");
		g_error_free(fax_error);
		return FALSE;
	}

	inet_address = g_inet_address_new_from_string("127.0.0.1");

	sock_address = g_inet_socket_address_new(inet_address, 9100);
	if (sock_address == NULL) {
		g_debug("Could not create sock address on port 9100");
		g_object_unref(socket);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "%s", "Could not create sock address on port 9100");
		return FALSE;
	}

	if (g_socket_bind(socket, sock_address, TRUE, &fax_error) == FALSE) {
		g_debug("Could not bind to socket. Error: %s", fax_error->message);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "Could not bind to socket. Error: %s", fax_error->message);
		g_error_free(fax_error);
		g_object_unref(socket);
		return FALSE;
	}

	if (g_socket_listen(socket, &fax_error) == FALSE) {
		g_debug("Could not listen on socket. Error: %s", fax_error->message);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "Could not listen on socket. Error: %s", fax_error->message);
		g_error_free(fax_error);
		g_object_unref(socket);
		return FALSE;
	}

	g_debug("Fax Server running on port 9100");

	g_thread_new("printserver", print_server_thread, socket);

	return TRUE;
}

#endif
