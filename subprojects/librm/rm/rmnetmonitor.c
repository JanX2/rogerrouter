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

#include <gio/gio.h>
#include <rm/rmnetmonitor.h>
#include <rm/rmprofile.h>
#include <rm/rmssdp.h>

/**
 * SECTION:rmnetmonitor
 * @Title: RmNetMonitor
 * @Short_description: Network monitor - Handle network states
 *
 * A network monitor to react on network changes (offline/online).
 */

/** Internal network event list */
static GSList *rm_net_event_list = NULL;
/** Internal last known network state */
static gboolean rm_net_online = FALSE;

/**
 * rm_netmonitor_add_event:
 * @name: name describing event
 * @connect: a #RmNetConnect
 * @disconnect: a #RmNetDisconnect
 * @user_data: additional user data pointer
 *
 * Add network event which will be triggered for online and offline cases.
 *
 * Returns: a #RmNetEvent
 */
RmNetEvent *rm_netmonitor_add_event(gchar *name, RmNetConnect connect, RmNetDisconnect disconnect, gpointer user_data)
{
	RmNetEvent *event;

	/* Sanity checks */
	g_assert(connect != NULL);
	g_assert(disconnect != NULL);

	/* Allocate new event */
	event = g_slice_new(RmNetEvent);

	/* Set event functions */
	event->name = g_strdup(name);
	event->connect = connect;
	event->disconnect = disconnect;
	event->user_data = user_data;
	event->is_connected = FALSE;

	/* Add to network event list */
	rm_net_event_list = g_slist_append(rm_net_event_list, event);

	/* If current state is online, start connect() function */
	if (rm_net_online) {
		event->is_connected = event->connect(event->user_data);
	}

	return event;
}

/**
 * rm_netmonitor_remove_event:
 * @net_event: a #RmNetEvent to remove
 *
 * Remove network event @net_event from monitor.
 */
void rm_netmonitor_remove_event(RmNetEvent *net_event)
{
	rm_net_event_list = g_slist_remove(rm_net_event_list, net_event);

	if (rm_net_online) {
		net_event->is_connected = !net_event->disconnect(net_event->user_data);
	}

	g_free(net_event->name);
	g_slice_free(RmNetEvent, net_event);
}

/**
 * rm_netmonitor_state:
 * @state: network state (true = online, false = offline)
 *
 * Return network status text
 *
 * Rreturns: network state in text
 */
static inline gchar *rm_netmonitor_state(gboolean state)
{
	return state ? "online" : "offline";
}

/**
 * rm_netmonitor_is_online:
 *
 * Return network online status
 *
 * Returns: network online status as boolean (true = online, false = offline)
 */
gboolean rm_netmonitor_is_online(void)
{
	return rm_net_online;
}

/**
 * rm_netmonitor_state_changed:
 * @state: new network state (online/offline)
 *
 * Returns: Handle network state changes.
 */
static void rm_netmonitor_state_changed(gboolean state)
{
	GSList *list;

	g_debug("%s(): Network state changed from %s to %s", __FUNCTION__, rm_netmonitor_state(rm_net_online), rm_netmonitor_state(state));

	/* Set internal network state */
	rm_net_online = state;

	/* Call network function depending on network state */
	if (!rm_net_online) {
		/* Offline: disconnect all network events */
		for (list = rm_net_event_list; list != NULL; list = list->next) {
			RmNetEvent *event = list->data;

			if (event->is_connected) {
				g_debug("%s(): Calling disconnect for '%s'", __FUNCTION__, event->name);
				event->is_connected = !event->disconnect(event->user_data);
			}
		}

		/* Disable active profile */
		rm_profile_set_active(NULL);
	} else {
		RmProfile *profile;

		/* Do not switch profile if transition is online -> online, get current active profile */
		profile = rm_profile_get_active();
		if (!profile) {
			/* Online and no active profile: Try to detect active profile */
			profile = rm_profile_detect();
		}

		if (profile) {
			rm_profile_set_active(profile);

			/* We have an active profile: Connect to all network events */
			for (list = rm_net_event_list; list != NULL; list = list->next) {
				RmNetEvent *event = list->data;

				if (!event->is_connected) {
					g_debug("%s(): Calling connect for '%s'", __FUNCTION__, event->name);
					event->is_connected = event->connect(event->user_data);
				}
			}
		}
	}
}

/**
 * rm_netmonitor_reconnect:
 *
 * Trigger a reconnect (events are retriggerd so events that depends on profile are connected).
 */
void rm_netmonitor_reconnect(void)
{
	gboolean state = TRUE;

	rm_netmonitor_state_changed(FALSE);

#ifdef G_OS_UNIX
	GNetworkMonitor *monitor = g_network_monitor_get_default();

	state = g_network_monitor_get_network_available(monitor);
#endif

	rm_netmonitor_state_changed(state);
}

/**
 * rm_netmonitor_changed_cb:
 * @monitor: a #GNetworkMonitor
 * @available: network available
 * @unused: unused user data pointer
 *
 * Network monitor changed callback.
 */
void rm_netmonitor_changed_cb(GNetworkMonitor *monitor, gboolean available, gpointer unused)
{
	rm_netmonitor_state_changed(available);
}

/**
 * rm_netmonitor_init:
 *
 * Initialize network monitor.
 *
 * Returns: TRUE on success, otherwise FALSE
 */
gboolean rm_netmonitor_init(void)
{
	gboolean state = TRUE;

#ifdef G_OS_UNIX
	GNetworkMonitor *monitor = g_network_monitor_get_default();

	g_return_val_if_fail(monitor != NULL, FALSE);

	/* Connect signal handler */
	g_signal_connect(monitor, "network-changed", G_CALLBACK(rm_netmonitor_changed_cb), NULL);

	rm_ssdp_init();

	state = g_network_monitor_get_network_available(monitor);
#endif

	rm_netmonitor_state_changed(state);

	return TRUE;
}

/**
 * rm_netmonitor_shutdown:
 *
 * Shutdown network monitor (disconnect signal and shutdown event callbacks).
 */
void rm_netmonitor_shutdown(void)
{
#ifdef G_OS_UNIX
	GNetworkMonitor *monitor = g_network_monitor_get_default();

	/* Disconnect signal handler */
	g_signal_handlers_disconnect_by_func(monitor, G_CALLBACK(rm_netmonitor_changed_cb), NULL);

	rm_netmonitor_state_changed(FALSE);
#endif
}
