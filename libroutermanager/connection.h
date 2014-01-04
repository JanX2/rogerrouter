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

#ifndef LIBROUTERMANAGER_CONNECTION_H
#define LIBROUTERMANAGER_CONNECTION_H

#include <libroutermanager/call.h>

G_BEGIN_DECLS

#define CONNECTION_TYPE_INCOMING   0x01
#define CONNECTION_TYPE_OUTGOING   0x02
#define CONNECTION_TYPE_CONNECT    0x04
#define CONNECTION_TYPE_DISCONNECT 0x08
#define CONNECTION_TYPE_MISS       (CONNECTION_TYPE_INCOMING | CONNECTION_TYPE_DISCONNECT)

/** connection structure */
struct connection {
	/* Unique ID */
	guint id;
	/* Type */
	guchar type;
	/* Local number */
	gchar *local_number;
	/* Remote number */
	gchar *remote_number;
	/* Notification widget */
	void *notification;
	/* Private data */
	void *priv;
};

struct connection *connection_add_call(gint id, gint type, const gchar *local_number, const gchar *remote_number);
struct connection *connection_find_by_id(gint id);
struct connection *connection_find_by_number(const gchar *number);
void connection_set_type(struct connection *connection, gint type);
void connection_remove(struct connection *connection);
void connection_ignore(struct connection *connection);

G_END_DECLS

#endif
