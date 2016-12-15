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

#include <libroutermanager/rmprofile.h>
#include <libroutermanager/rmobjectemit.h>
#include <libroutermanager/rmcsv.h>
#include <libroutermanager/rmlog.h>
#include <libroutermanager/rmnetwork.h>
#include <libroutermanager/rmrouter.h>
#include <libroutermanager/rmplugins.h>

#include <libsoup/soup.h>

#include "firmware-common.h"
#include "firmware-04-74.h"
#include "firmware-05-50.h"
#include "firmware-06-35.h"
#include "firmware-04-00.h"
#include "firmware-query.h"
#include "csv.h"

#define RM_TYPE_FRITZBOX_PLUGIN        (routermanager_fritzbox_plugin_get_type ())
#define RM_FRITZBOX_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), RM_TYPE_FRITZBOX_PLUGIN, RmFritzboxPlugin))

typedef struct {
	guint dummy;
} RmFritzBoxPluginPrivate;

RM_PLUGIN_REGISTER(RM_TYPE_FRITZBOX_PLUGIN, RmFritzBoxPlugin, routermanager_fritzbox_plugin)

GSettings *fritzbox_settings = NULL;

/**
 * \brief Main login function (depending on box type)
 * \param profile profile information structure
 * \return error code
 */
gboolean fritzbox_login(RmProfile *profile)
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
		return fritzbox_login_04_00(profile);
	}

	return FALSE;
}

/**
 * \brief Main get settings functions
 * \param profile profile information structure
 * \return error code
 */
gboolean fritzbox_get_settings(RmProfile *profile)
{
	if (fritzbox_get_settings_query(profile)) {
		return TRUE;
	}

	if (FIRMWARE_IS(6, 35)) {
		return fritzbox_get_settings_06_35(profile);
	}

	if (FIRMWARE_IS(5, 50)) {
		return fritzbox_get_settings_05_50(profile);
	}

	if (FIRMWARE_IS(4, 0)) {
		return fritzbox_get_settings_04_74(profile);
	}

	return FALSE;
}

/**
 * \brief Main load journal function (big switch for each supported router)
 * \param profile profile info structure
 * \param data_ptr data pointer to optional store journal to
 * \return error code
 */
gboolean fritzbox_load_journal(RmProfile *profile, gchar **data_ptr)
{
	gboolean ret = FALSE;

	g_debug("%s(): called (%d.%d.%d)", __FUNCTION__, profile->router_info->maj_ver_id, profile->router_info->min_ver_id, profile->router_info->maj_ver_id);
	if (FIRMWARE_IS(5, 50)) {
		ret = fritzbox_load_journal_05_50(profile, data_ptr);
	} else if (FIRMWARE_IS(4, 0)) {
		ret = fritzbox_load_journal_04_74(profile, data_ptr);
	}

	return ret;
}

/**
 * \brief Main clear journal function (big switch for each supported router)
 * \param profile profile info structure
 * \return error code
 */
gboolean fritzbox_clear_journal(RmProfile *profile)
{
	if (!profile) {
		return FALSE;
	}

	if (FIRMWARE_IS(5, 50)) {
		return fritzbox_clear_journal_05_50(profile);
	}

	if (FIRMWARE_IS(4, 0)) {
		return fritzbox_clear_journal_04_74(profile);
	}

	return FALSE;
}

/**
 * \brief Dial number using router ui
 * \param profile profile information structure
 * \param port dial port
 * \param number remote number
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_dial_number(RmProfile *profile, gint port, const gchar *number)
{
	if (!profile) {
		return FALSE;
	}

	if (FIRMWARE_IS(6, 30)) {
		return fritzbox_dial_number_06_35(profile, port, number);
	}

	if (FIRMWARE_IS(4, 0)) {
		return fritzbox_dial_number_04_00(profile, port, number);
	}

	return FALSE;
}

/**
 * \brief Hangup call using router ui
 * \param profile profile information structure
 * \param port dial port
 * \param number remote number
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_hangup(RmProfile *profile, gint port, const gchar *number)
{
	if (!profile) {
		return FALSE;
	}

	if (FIRMWARE_IS(6, 30)) {
		return fritzbox_hangup_06_35(profile, port, number);
	}

	if (FIRMWARE_IS(4, 0)) {
		return fritzbox_hangup_04_00(profile, port, number);
	}

	return FALSE;
}

void fritzbox_set_active(RmProfile *profile)
{
	fritzbox_settings = rm_settings_new_profile("org.tabos.routermanager.plugins.fritzbox", "fritzbox", rm_profile_get_name(profile));
	g_debug("%s(): Router set active, settings created", __FUNCTION__);
}

/** FRITZ!Box router functions */
static RmRouter fritzbox = {
	"FRITZ!Box",
	fritzbox_present,
	fritzbox_set_active,
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

static RmConnection *dialer_connection = NULL;

RmConnection *fritzbox_phone_dialer_get_connection(void)
{
	return dialer_connection;
}

extern RmDevice *telnet_device;

RmConnection *dialer_dial(const gchar *target, gboolean anonymous)
{
	RmConnection *connection = NULL;

	if (fritzbox_dial_number(rm_profile_get_active(), -1, target)) {
		connection = rm_connection_add(telnet_device, 0, RM_CONNECTION_TYPE_OUTGOING, "", target);
	}

	dialer_connection = connection;

	return connection;
}

void dialer_hangup(RmConnection *connection)
{
	fritzbox_hangup(rm_profile_get_active(), -1, connection->remote_number);

	dialer_connection = NULL;
}

static RmPhone dialer_phone = {
    "Phone dialer",
    dialer_dial,
    NULL,
    dialer_hangup,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/**
 * \brief Activate plugin (register fritzbox router)
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	/* Register router structure */
	rm_router_register(&fritzbox);

	fritzbox_init_callmonitor();

	rm_phone_register(&dialer_phone);

	/*RmPhone *phone = g_malloc0(sizeof(RmPhone));

	phone->name = g_strdup("MOEP");
	rm_phone_register(phone);*/
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	fritzbox_shutdown_callmonitor();

	rm_phone_unregister(&dialer_phone);
}
