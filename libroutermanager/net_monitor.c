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

#include <gio/gio.h>
#include <libroutermanager/net_monitor.h>
#include <libroutermanager/profile.h>

/** Internal network event list */
static GSList *net_event_list = NULL;
/** Internal last known network state */
static gboolean net_online = FALSE;

/**
 * \brief Add network event
 * \param connect network connect function
 * \param disconnect network disconnect function
 * \param user_data user data pointer
 * \return net event id
 */
gconstpointer net_add_event(net_connect_func connect, net_disconnect_func disconnect, gpointer user_data)
{
	struct net_event *event;

	/* Sanity checks */
	g_assert(connect != NULL);
	g_assert(disconnect != NULL);

	/* Allocate new event */
	event = g_slice_new(struct net_event);

	/* Set event functions */
	event->connect = connect;
	event->disconnect = disconnect;
	event->user_data = user_data;

	/* Add to network event list */
	net_event_list = g_slist_append(net_event_list, event);

	/* If current state is online, start connect() function */
	if (net_online) {
		event->connect(event->user_data);
	}

	return event;
}

/**
 * \brief Remove network event by id
 * \param net_event_id network event id
 */
void net_remove_event(gconstpointer net_event_id)
{
	struct net_event *event = (struct net_event *) net_event_id;

	net_event_list = g_slist_remove(net_event_list, net_event_id);

	if (net_online) {
		event->disconnect(event->user_data);
	}

	g_slice_free(struct net_event, event);
}

/**
 * \brief Return network status text
 * \param state network state
 * \return network state in text
 */
static inline gchar *net_state(gboolean state)
{
	return state ? "online" : "offline";
}

/**
 * \brief Return network online status
 * \return network online status
 */
gboolean net_is_online(void)
{
	return net_online;
}

/**
 * \brief Handle network state changes
 * \param state current network state
 */
void net_monitor_state_changed(gboolean state)
{
	GSList *list;

	/* Compare internal and new network state, only handle new states */
	if (net_online == state) {
		return;
	}

	g_debug("Network state changed from %s to %s", net_state(net_online), net_state(state));

	/* Set internal network state */
	net_online = state;

	/* Call network function depending on network state */
	if (!net_online) {
		for (list = net_event_list; list != NULL; list = list->next) {
			struct net_event *event = list->data;

			event->disconnect(event->user_data);
		}
	}

	if (net_online) {
		if (profile_detect_active()) {
			for (list = net_event_list; list != NULL; list = list->next) {
				struct net_event *event = list->data;

				event->connect(event->user_data);
			}
		}
	} else {
		profile_set_active(NULL);
	}
}

/**
 * \brief Issue a reconnect (events are retriggerd so events that depends on profile are connected)
 */
void net_monitor_reconnect(void)
{
#ifdef G_OS_UNIX
	GNetworkMonitor *monitor = g_network_monitor_get_default();

	net_monitor_state_changed(FALSE);
	net_monitor_state_changed(g_network_monitor_get_network_available(monitor));
#else
	net_monitor_state_changed(FALSE);
	net_monitor_state_changed(TRUE);
#endif
}

/**
 * \brief Network monitor changed callback
 * \param monitor GNetworkMonitor pointer
 * \param available network available flag
 * \param unused unused user data pointer
 */
void net_monitor_changed_cb(GNetworkMonitor *monitor, gboolean available, gpointer unused)
{
	net_monitor_state_changed(available);
}

/**
 * \brief Initialize network monitor
 * \return TRUE on success, otherwise FALSE
 */
gboolean net_monitor_init(void)
{
#ifdef G_OS_UNIX
	GNetworkMonitor *monitor = g_network_monitor_get_default();

	g_return_val_if_fail(monitor != NULL, FALSE);

	/* Connect signal handler */
	g_signal_connect(monitor, "network-changed", G_CALLBACK(net_monitor_changed_cb), NULL);

	net_monitor_state_changed(g_network_monitor_get_network_available(monitor));
#else
	net_monitor_state_changed(TRUE);
#endif

	return TRUE;
}

/**
 * \brief Shutdown network monitor (disconnect signal and shutdown event callbacks)
 */
void net_monitor_shutdown(void)
{
#ifdef G_OS_UNIX
	GNetworkMonitor *monitor = g_network_monitor_get_default();

	/* Disconnect signal handler */
	g_signal_handlers_disconnect_by_func(monitor, G_CALLBACK(net_monitor_changed_cb), NULL);
#endif

	/* Send offline state */
	net_monitor_state_changed(FALSE);
}
