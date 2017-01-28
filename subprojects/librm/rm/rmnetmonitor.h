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

#ifndef __RM_NETMONITOR_H
#define __RM_NETMONITOR_H

G_BEGIN_DECLS

typedef gboolean (*RmNetConnect)(gpointer user_data);
typedef gboolean (*RmNetDisconnect)(gpointer user_data);

typedef struct net_event {
	gchar *name;
	RmNetConnect connect;
	RmNetDisconnect disconnect;
	gboolean is_connected;
	gpointer user_data;
} RmNetEvent;

gboolean rm_netmonitor_init(void);
void rm_netmonitor_shutdown(void);

RmNetEvent *rm_netmonitor_add_event(gchar *name, RmNetConnect connect, RmNetDisconnect disconnect, gpointer user_data);
void rm_netmonitor_remove_event(RmNetEvent *net_event);
gboolean rm_netmonitor_is_online(void);
void rm_netmonitor_reconnect(void);

G_END_DECLS

#endif
