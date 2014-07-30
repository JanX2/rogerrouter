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
#include <string.h>
#include <stdlib.h>

#ifndef G_OS_WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/call.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/router.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/net_monitor.h>
#include <libroutermanager/connection.h>

#define ROUTERMANAGER_TYPE_CALLMONITOR_PLUGIN (routermanager_callmonitor_plugin_get_type ())
#define ROUTERMANAGER_CALLMONITOR_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_CALLMONITOR_PLUGIN, RouterManagerCallMonitorPlugin))

#define CALLMONITOR_DEBUG 1

typedef struct {
	gconstpointer net_event_id;

	GIOChannel *channel;
	guint id;
} RouterManagerCallMonitorPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_CALLMONITOR_PLUGIN, RouterManagerCallMonitorPlugin, routermanager_callmonitor_plugin)

/**
 * \brief Convert text line and emit connection-notify signal
 * \param text text line from telnet port
 */
static inline void callmonitor_convert(gchar *text)
{
	gchar **fields = g_strsplit(text, ";", -1);
	struct connection *connection;

	if (!g_strcmp0(fields[1], "CALL")) {
		connection = connection_add_call(atoi(fields[2]), CONNECTION_TYPE_OUTGOING, fields[4], fields[5]);

		emit_connection_notify(connection);
	} else if (!g_strcmp0(fields[1], "RING")) {
		connection = connection_add_call(atoi(fields[2]), CONNECTION_TYPE_INCOMING, fields[4], fields[3]);

		emit_connection_notify(connection);
	} else if (!g_strcmp0(fields[1], "CONNECT")) {
		connection = connection_find_by_id(atoi(fields[2]));

		if (connection) {
			connection_set_type(connection, CONNECTION_TYPE_CONNECT);

			emit_connection_notify(connection);
		}
	} else if (!g_strcmp0(fields[1], "DISCONNECT")) {
		connection = connection_find_by_id(atoi(fields[2]));
		if (connection) {
			connection_set_type(connection, CONNECTION_TYPE_DISCONNECT);

			emit_connection_notify(connection);

			connection_remove(connection);
		}
	}

	g_strfreev(fields);
}

/**
 * \brief Call monitor socket callback
 * \param source source channel pointer
 * \param condition io condition
 * \param data user data (UNUSED)
 * \return TRUE
 */
gboolean callmonitor_io_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	GIOStatus ret;
	GError *error = NULL;
	gsize len;
	gchar *msg;

	if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
		/* A big problem occured and we've lost the connection */
		//callmonitor_reconnect(data);
		g_warning("Error in callmonitor io cb, abort");
		return FALSE;
	}

	switch (condition) {
	case G_IO_IN:
	case G_IO_PRI:
		ret = g_io_channel_read_line(source, &msg, &len, NULL, &error);
		if (ret != G_IO_STATUS_NORMAL) {
			g_error("Error reading: %s", error->message);
		}

		gchar **lines = g_strsplit(msg, "\n", -1);
		gint count = 0;
		for (count = 0; count < g_strv_length(lines); count++) {
			if (strlen(lines[count]) > 0) {
				callmonitor_convert(lines[count]);
			}
		}
		g_strfreev(lines);

		g_free(msg);
		break;
	case G_IO_ERR:
	case G_IO_HUP:
		g_error("Read end of pipe died!");
		break;
	default:
		g_debug("Unhandled case: %d", condition);
		break;
	}

	return TRUE;
}

/**
 * \brief Call monitor connect
 * \param user_data callmonitor plugin pointer
 * \return error code
 */
gboolean callmonitor_connect(gpointer user_data)
{
	RouterManagerCallMonitorPlugin *callmonitor_plugin = user_data;
	GSocket *socket;
	GInetAddress *inet_address = NULL;
	GSocketAddress *sock_address;
	GError *error;
	GResolver *resolver;
	GList *list;
	GList *iter;
	struct profile *profile;
	gint sock = -1;
	const gchar *hostname;
	gint tcp_keepalive_time = 600;

	profile = profile_get_active();
	if (!profile) {
		g_debug("No active profile");
		return FALSE;
	}

	hostname = router_get_host(profile);
	if (!hostname || strlen(hostname) <= 0) {
		g_debug("Invalid hostname");
		return FALSE;
	}

#ifdef CALLMONITOR_DEBUG
	g_debug("Trying to connect to '%s' on port 1012", hostname);
#endif

	resolver = g_resolver_get_default();
	list = g_resolver_lookup_by_name(resolver, hostname, NULL, NULL);
	g_object_unref(resolver);

	if (!list) {
		g_warning("Cannot resolve ip from hostname!");
		return FALSE;
	}

	for (iter = list; iter != NULL; iter = iter->next) {
		if (g_inet_address_get_family(iter->data) == G_SOCKET_FAMILY_IPV4) {
			inet_address = iter->data;
			break;
		}
	}

	if (!inet_address) {
		g_warning("Could not get required IPV4 connection to telnet port!");
		g_resolver_free_addresses(list);
		return FALSE;
	}

	sock_address = g_inet_socket_address_new(inet_address, 1012);
	if (!sock_address) {
		g_warning("Could not create sock address on port %s:1012", g_inet_address_to_string(inet_address));
		g_resolver_free_addresses(list);
		return FALSE;
	}

	error = NULL;
	socket = g_socket_new(g_inet_address_get_family(inet_address), G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &error);
	if (error) {
		g_warning("Could not create socket on %s:1012. Error: '%s'", g_inet_address_to_string(inet_address), error->message);
		g_error_free(error);
		g_object_unref(sock_address);
		g_resolver_free_addresses(list);
		return FALSE;
	}

	if (g_socket_connect(socket, sock_address, NULL, &error) == FALSE) {
		if (error) {
			g_debug("Could not connect to socket. Error: %s", error->message);
			g_error_free(error);
		} else {
			g_debug("Could not connect to socket: Error: unknown");
		}
		g_object_unref(socket);
		g_object_unref(sock_address);

		g_resolver_free_addresses(list);
		return FALSE;
	}

#ifdef CALLMONITOR_DEBUG
	g_debug("Connected to '%s' on port 1012", g_inet_address_to_string(inet_address));
#endif

	sock = g_socket_get_fd(socket);

	/* Set keep alive, otherwise the connection might drop silently */
	g_socket_set_keepalive(socket, TRUE);

	/* this is a bit of a mess: LINUX uses TCP_KEEP_IDLE, where OSX uses TCP_KEEPALIVE */
#ifdef TCP_KEEPIDLE
	setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &tcp_keepalive_time, sizeof(tcp_keepalive_time));
#else
#ifdef TCP_KEEPALIVE
	setsockopt(sock, IPPROTO_TCP, TCP_KEEPALIVE, &tcp_keepalive_time, sizeof(tcp_keepalive_time));
#endif
#endif

#ifdef G_OS_WIN32
	callmonitor_plugin->priv->channel = g_io_channel_win32_new_socket(sock);
#else
	callmonitor_plugin->priv->channel = g_io_channel_unix_new(sock);
#endif
	g_io_channel_set_encoding(callmonitor_plugin->priv->channel, NULL, NULL);

	callmonitor_plugin->priv->id = g_io_add_watch(callmonitor_plugin->priv->channel, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, callmonitor_io_cb, NULL);

	g_resolver_free_addresses(list);

	return TRUE;
}

/**
 * \brief Network disconnect callback
 * \param user_data callmonitor plugin pointer
 * \return TRUE
 */
gboolean callmonitor_disconnect(gpointer user_data)
{
	RouterManagerCallMonitorPlugin *callmonitor_plugin = user_data;
	GError *error = NULL;

	if (callmonitor_plugin->priv->id > 0) {
		g_source_remove(callmonitor_plugin->priv->id);
	}

	if (callmonitor_plugin->priv->channel) {
		if (g_io_channel_shutdown(callmonitor_plugin->priv->channel, FALSE, &error) != G_IO_STATUS_NORMAL) {
			g_warning("Could not shutdown callmonitor channel: '%s'", error->message);
			g_error_free(error);
			return FALSE;
		}

		g_io_channel_unref(callmonitor_plugin->priv->channel);
	}

	return TRUE;
}

/**
 * \brief Activate plugin (add net event)
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	RouterManagerCallMonitorPlugin *callmonitor_plugin = ROUTERMANAGER_CALLMONITOR_PLUGIN(plugin);

	/* Add network event */
	callmonitor_plugin->priv->net_event_id = net_add_event(callmonitor_connect, callmonitor_disconnect, callmonitor_plugin);
}

/**
 * \brief Deactivate plugin (remote net event)
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerCallMonitorPlugin *callmonitor_plugin = ROUTERMANAGER_CALLMONITOR_PLUGIN(plugin);

	/* Remove network event */
	net_remove_event(callmonitor_plugin->priv->net_event_id);
}
