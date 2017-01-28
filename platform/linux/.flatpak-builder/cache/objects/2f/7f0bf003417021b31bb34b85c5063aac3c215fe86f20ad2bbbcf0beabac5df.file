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

#ifndef __RM_FTP_H
#define __RM_FTP_H

G_BEGIN_DECLS

typedef struct rm_ftp {
	gchar *server;
	gint code;
	gchar *response;
	GIOChannel *control;
	GIOChannel *data;
	GTimer *timer;
} RmFtp;

gchar *rm_ftp_read_response(GIOChannel *channel, gsize *len);
gboolean rm_ftp_send_command(RmFtp *client, gchar *command);
gboolean rm_ftp_login(RmFtp *client, const gchar *user, const gchar *password);
gboolean rm_ftp_passive(RmFtp *client);
gchar *rm_ftp_list_dir(RmFtp *client, const gchar *dir);
gchar *rm_ftp_get_file(RmFtp *client, const gchar *file, gsize *len);
gboolean rm_ftp_put_file(RmFtp *client, const gchar *file, const gchar *path, gchar *data, gsize size);
RmFtp *rm_ftp_init(const gchar *server);
gboolean rm_ftp_delete_file(RmFtp *client, const gchar *file);
gboolean rm_ftp_shutdown(RmFtp *client);

G_END_DECLS

#endif
