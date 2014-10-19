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

#include "firmware-common.h"
#include "firmware-04-74.h"
#include "firmware-05-50.h"
#include "firmware-plain.h"
#include "csv.h"

#define ROUTERMANAGER_TYPE_FRITZBOX_PLUGIN        (routermanager_fritzbox_plugin_get_type ())
#define ROUTERMANAGER_FRITZBOX_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_FRITZBOX_PLUGIN, RouterManagerFritzboxPlugin))

typedef struct {
	guint dummy;
} RouterManagerFritzBoxPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_FRITZBOX_PLUGIN, RouterManagerFritzBoxPlugin, routermanager_fritzbox_plugin)

/**
 * \brief Main login function (depending on box type)
 * \param profile profile information structure
 * \return error code
 */
gboolean fritzbox_login(struct profile *profile)
{
	if (FIRMWARE_IS(5, 50)) {
		/* Session-ID based on login_sid.lua */
		return fritzbox_login_05_50(profile);
	}

	if (FIRMWARE_IS(4, 74)) {
		/* Session-ID based on login_sid.xml */
		return fritzbox_login_04_74(profile);
	}

	if (FIRMWARE_IS(4, 0)) {
		/* Plain login method */
		return fritzbox_login_plain(profile);
	}

	return FALSE;
}

/**
 * \brief Main get settings functions
 * \param profile profile information structure
 * \return error code
 */
gboolean fritzbox_get_settings(struct profile *profile)
{
	if (FIRMWARE_IS(5, 50)) {
		return fritzbox_get_settings_05_50(profile);
	}

	if (FIRMWARE_IS(4, 74)) {
		return fritzbox_get_settings_04_74(profile);
	}

	if (FIRMWARE_IS(4, 0)) {
		return fritzbox_get_settings_plain(profile);
	}

	return FALSE;
}

/**
 * \brief Main load journal function (big switch for each supported router)
 * \param profile profile info structure
 * \param data_ptr data pointer to optional store journal to
 * \return error code
 */
gboolean fritzbox_load_journal(struct profile *profile, gchar **data_ptr)
{
	gboolean ret = FALSE;

	if (FIRMWARE_IS(5, 50)) {
		ret = fritzbox_load_journal_05_50(profile, data_ptr);
	} else if (FIRMWARE_IS(4, 74)) {
		ret = fritzbox_load_journal_04_74(profile, data_ptr);
	}

	return ret;
}

/**
 * \brief Main clear journal function (big switch for each supported router)
 * \param profile profile info structure
 * \return error code
 */
gboolean fritzbox_clear_journal(struct profile *profile)
{
	if (!profile) {
		return FALSE;
	}

	if (FIRMWARE_IS(5, 50)) {
		return fritzbox_clear_journal_05_50(profile);
	}

	if (FIRMWARE_IS(4, 74)) {
		return fritzbox_clear_journal_04_74(profile);
	}

	return FALSE;
}

/** FRITZ!Box router functions */
static struct router fritzbox = {
	"FRITZ!Box",
	fritzbox_present,
	fritzbox_login,
	fritzbox_logout,
	fritzbox_get_settings,
	fritzbox_load_journal,
	fritzbox_clear_journal,
	fritzbox_dial_number,
	fritzbox_hangup,
	fritzbox_load_fax,
	fritzbox_load_voice,
	fritzbox_get_ip,
	fritzbox_reconnect,
	fritzbox_delete_fax,
	fritzbox_delete_voice,
};

/**
 * \brief Activate plugin (register fritzbox router)
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	/* Register router structure */
	routermanager_router_register(&fritzbox);
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	/* Currently does nothing */
}
