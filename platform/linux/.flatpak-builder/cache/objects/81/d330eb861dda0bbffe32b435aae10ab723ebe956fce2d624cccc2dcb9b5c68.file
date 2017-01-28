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

#ifndef LIBROUTERMANAGER_NET_MONITOR_H
#define LIBROUTERMANAGER_NET_MONITOR_H

G_BEGIN_DECLS

typedef gboolean (*net_connect_func)(gpointer user_data);
typedef gboolean (*net_disconnect_func)(gpointer user_data);

struct net_event {
	net_connect_func connect;
	net_disconnect_func disconnect;
	gboolean is_connected;
	gpointer user_data;
};

gboolean net_monitor_init(void);
void net_monitor_shutdown(void);

void net_monitor_state_changed(gboolean state);
gconstpointer net_add_event(net_connect_func connect, net_disconnect_func disconnect, gpointer user_data);
void net_remove_event(gconstpointer net_event_id);
gboolean net_is_online(void);
void net_monitor_reconnect(void);

G_END_DECLS

#endif
