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

#ifndef LIBROUTERMANAGER_FTP_H
#define LIBROUTERMANAGER_FTP_H

G_BEGIN_DECLS

struct ftp {
	gchar *server;
	gint code;
	gchar *response;
	GIOChannel *control;
	GIOChannel *data;
	GTimer *timer;
};

gchar *ftp_read_response(GIOChannel *channel, gsize *len);
gboolean ftp_send_command(struct ftp *client, gchar *command);
gboolean ftp_login(struct ftp *client, const gchar *user, const gchar *password);
gboolean ftp_passive(struct ftp *client);
gchar *ftp_list_dir(struct ftp *client, const gchar *dir);
gchar *ftp_get_file(struct ftp *client, const gchar *file, gsize *len);
gboolean ftp_put_file(struct ftp *client, const gchar *file, const gchar *path, gchar *data, gsize size);
struct ftp *ftp_init(const gchar *server);
gboolean ftp_delete_file(struct ftp *client, const gchar *file);
gboolean ftp_shutdown(struct ftp *client);

G_END_DECLS

#endif
