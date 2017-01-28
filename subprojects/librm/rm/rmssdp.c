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

#include <rmconfig.h>

#include <glib.h>

/**
 * SECTION:rmssdp
 * @Title: RmSsdp
 * @Short_description: SSDP - Scans for router using SSDP
 *
 * SSDP scans for routers within the network using UPnP SSDP.
 */

#ifdef HAVE_SSDP

#include <libgupnp/gupnp.h>
#include <libgupnp/gupnp-device-info.h>

#include <rm/rmrouter.h>
#include <rm/rmssdp.h>

static GUPnPContextManager *rm_context_manager;

static GList *rm_routers = NULL;

/**
 * rm_device_proxy_available_cb:
 * @cp: a #GUPnPControlPoint
 * @proxy: a #GUPnPDeviceProxy
 *
 * Scan for available routers once a new network device is attached.
 */
static void rm_device_proxy_available_cb(GUPnPControlPoint *cp, GUPnPDeviceProxy *proxy)
{
	RmRouterInfo *router_info = g_slice_new0(RmRouterInfo);
	GUPnPDeviceInfo *info = GUPNP_DEVICE_INFO(proxy);
	const SoupURI *uri;

	uri = gupnp_device_info_get_url_base(info);
	router_info->host = g_strdup(soup_uri_get_host((SoupURI*)uri));

	/* Scan for router and add detected devices */
	if (rm_router_present(router_info) == TRUE) {
		rm_routers = g_list_append(rm_routers, router_info);
	}
}

/**
 * rm_device_proxy_unavailable_cb:
 * @cp: a #GUPnPControlPoint
 * @proxy: a #GUPnPDeviceProxy
 *
 * Currently does nothing....
 */
static void rm_device_proxy_unavailable_cb(GUPnPControlPoint *cp, GUPnPDeviceProxy *proxy)
{
	g_debug("%s(): %s", __FUNCTION__, gupnp_device_info_get_model_name(GUPNP_DEVICE_INFO(proxy)));
}

/**
 * rm_ssdp_get_routers:
 *
 * Returns: a list of detected routers
 */
GList *rm_ssdp_get_routers(void)
{
	if (!rm_routers) {
		RmRouterInfo *router_info = g_slice_new0(RmRouterInfo);

		/* Fallback - In case no rm_routers have been detected, try at least fritz.box manually */
		router_info->host = g_strdup("fritz.box");

		/* Scan for router and add detected devices */
		if (rm_router_present(router_info) == TRUE) {
			rm_routers = g_list_append(rm_routers, router_info);
		}
	}

	return rm_routers;
}

/**
 * rm_on_context_available:
 * @manager: a #GUPnPContextManager
 * @context: a GUPnPContext
 * @user_data: unused
 *
 * Activated once a new upnp context is available. Scans for new upnp internet gateway devices.
 */
static void rm_on_context_available(GUPnPContextManager *manager, GUPnPContext *context, gpointer user_data)
{
	GUPnPControlPoint *cp;

	cp = gupnp_control_point_new(context, "urn:schemas-upnp-org:device:InternetGatewayDevice:1");

	g_signal_connect(cp, "device-proxy-available", G_CALLBACK(rm_device_proxy_available_cb), NULL);
	g_signal_connect(cp, "device-proxy-unavailable", G_CALLBACK(rm_device_proxy_unavailable_cb), NULL);

	gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(cp), TRUE);

	gupnp_context_manager_manage_control_point(rm_context_manager, cp);

	g_object_unref (cp);
}

/**
 * rm_ssdp_init:
 *
 * Initialize ssdp handling
 */
void rm_ssdp_init(void)
{
	g_debug("%s(): Initialize upnp context manager", __FUNCTION__);
	rm_context_manager = gupnp_context_manager_new(NULL, 1900);

	g_signal_connect(rm_context_manager, "context-available", G_CALLBACK(rm_on_context_available), NULL);
}

#else

/**
 * rm_ssdp_get_routers:
 *
 * Dummy function - SSDP not compiled in
 *
 * Returns: NULL
 */
GList *rm_ssdp_get_routers(void)
{
	return NULL;
}

/**
 * rm_ssdp_init:
 *
 * Dummy function - SSDP not compiled in
 */
void rm_ssdp_init(void)
{
}

#endif
