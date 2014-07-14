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

#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/csv.h>
#include <libroutermanager/logging.h>
#include <libroutermanager/network.h>
#include <libroutermanager/router.h>
#include <libroutermanager/plugins.h>

#include <libsoup/soup.h>

#define ROUTERMANAGER_TYPE_SPEEDPORT_PLUGIN        (routermanager_speedport_plugin_get_type ())
#define ROUTERMANAGER_SPEEDPORT_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_SPEEDPORT_PLUGIN, RouterManagerSpeedportPlugin))

typedef struct {
	guint dummy;
} RouterManagerSpeedportPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_SPEEDPORT_PLUGIN, RouterManagerSpeedportPlugin, routermanager_speedport_plugin)

gboolean speedport_present(struct router_info *router_info)
{
	g_debug("Scanning for speedport");
	return FALSE;
}

/** Speedport router functions */
static struct router speedport = {
	/* Name */
	"Speedport",
	/* Present */
	speedport_present,
	/* Login */
	NULL,
	/* Logout */
	NULL,
	/* Get settings */
	NULL,
	/* Load journal */
	NULL,
	/* Clear journal */
	NULL,
	/* Dial number */
	NULL,
	/* Hangup */
	NULL,
	/* Load fax */
	NULL,
	/* Load voice */
	NULL,
	/* Get IP */
	NULL,
	/* Reconnect */
	NULL,
	/* Delete fax */
	NULL,
	/* Delete voice */
	NULL,
};

/**
 * \brief Activate plugin (register speedport router)
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	/* Register router structure */
	routermanager_router_register(&speedport);
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	/* Currently does nothing */
}
