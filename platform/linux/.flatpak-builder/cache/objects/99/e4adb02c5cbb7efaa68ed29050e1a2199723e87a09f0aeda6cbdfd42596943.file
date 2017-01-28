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

#include <libgupnp/gupnp.h>
#include <libgupnp/gupnp-device-info.h>

#include <libroutermanager/router.h>

static GUPnPContextManager *context_manager;

static GList *routers = NULL;

static void device_proxy_available_cb(GUPnPControlPoint *cp, GUPnPDeviceProxy *proxy)
{
	struct router_info *router_info = g_slice_new0(struct router_info);
	GUPnPDeviceInfo *info = GUPNP_DEVICE_INFO(proxy);
	const SoupURI *uri;

	uri = gupnp_device_info_get_url_base(info);
	router_info->host = g_strdup(soup_uri_get_host((SoupURI*)uri));

	/* Scan for router and add detected devices */
	if (router_present(router_info) == TRUE) {
		routers = g_list_append(routers, router_info);
	}
}

static void device_proxy_unavailable_cb(GUPnPControlPoint *cp, GUPnPDeviceProxy *proxy)
{
	g_debug("%s(): %s", __FUNCTION__, gupnp_device_info_get_model_name(GUPNP_DEVICE_INFO(proxy)));
}

GList *ssdp_get_routers(void)
{
	if (!routers) {
		struct router_info *router_info = g_slice_new0(struct router_info);

		/* Fallback - In case no routers have been detected, try at least fritz.box manually */
		router_info->host = g_strdup("fritz.box");

		/* Scan for router and add detected devices */
		if (router_present(router_info) == TRUE) {
			routers = g_list_append(routers, router_info);
		}
	}

	return routers;
}

static void on_context_available(GUPnPContextManager *manager, GUPnPContext *context, gpointer user_data)
{
	GUPnPControlPoint *cp;

	cp = gupnp_control_point_new(context, "urn:schemas-upnp-org:device:InternetGatewayDevice:1");

	g_signal_connect(cp, "device-proxy-available", G_CALLBACK(device_proxy_available_cb), NULL);
	g_signal_connect(cp, "device-proxy-unavailable", G_CALLBACK (device_proxy_unavailable_cb), NULL);

	gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(cp), TRUE);

	gupnp_context_manager_manage_control_point(context_manager, cp);

	g_object_unref (cp);
}

void ssdp_init(void)
{
	context_manager = gupnp_context_manager_new(NULL, 1900);
	g_signal_connect(context_manager, "context-available", G_CALLBACK(on_context_available), NULL);
}
