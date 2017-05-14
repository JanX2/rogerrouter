/*
 * The rm project
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

#include <rm/rmprofile.h>
#include <rm/rmsettings.h>
#include <rm/rmobjectemit.h>
#include <rm/rmmain.h>
#include <rm/rmplugins.h>
#include <rm/rmstring.h>

#include "fritzbox.h"
#include "firmware-common.h"
#include "firmware-04-74.h"
#include "firmware-05-50.h"
#include "firmware-06-35.h"
#include "firmware-04-00.h"
#include "firmware-query.h"
#include "firmware-tr64.h"
#include "csv.h"
#include "callmonitor.h"

/** Mapping between config value and port type */
FritzBoxPhonePort fritzbox_phone_ports[PORT_MAX] = {
	/* Analog */
	{"name-analog1", "telcfg:settings/MSN/Port0/Name", PORT_ANALOG1, 1},
	{"name-analog2", "telcfg:settings/MSN/Port1/Name", PORT_ANALOG2, 2},
	{"name-analog3", "telcfg:settings/MSN/Port2/Name", PORT_ANALOG3, 3},
	/* ISDN */
	{"name-isdn1", "telcfg:settings/NTHotDialList/Name1", PORT_ISDN1, 51},
	{"name-isdn2", "telcfg:settings/NTHotDialList/Name2", PORT_ISDN2, 52},
	{"name-isdn3", "telcfg:settings/NTHotDialList/Name3", PORT_ISDN3, 53},
	{"name-isdn4", "telcfg:settings/NTHotDialList/Name4", PORT_ISDN4, 54},
	{"name-isdn5", "telcfg:settings/NTHotDialList/Name5", PORT_ISDN5, 55},
	{"name-isdn6", "telcfg:settings/NTHotDialList/Name6", PORT_ISDN6, 56},
	{"name-isdn7", "telcfg:settings/NTHotDialList/Name7", PORT_ISDN7, 57},
	{"name-isdn8", "telcfg:settings/NTHotDialList/Name8", PORT_ISDN8, 58},
	/* DECT */
	{"name-dect1", "telcfg:settings/Foncontrol/User1/Name", PORT_DECT1, 60},
	{"name-dect2", "telcfg:settings/Foncontrol/User2/Name", PORT_DECT2, 61},
	{"name-dect3", "telcfg:settings/Foncontrol/User3/Name", PORT_DECT3, 62},
	{"name-dect4", "telcfg:settings/Foncontrol/User4/Name", PORT_DECT4, 63},
	{"name-dect5", "telcfg:settings/Foncontrol/User5/Name", PORT_DECT5, 64},
	{"name-dect6", "telcfg:settings/Foncontrol/User6/Name", PORT_DECT6, 65},
	/* IP-Phone */
	{"name-sip0", "telcfg:settings/VoipExtension0/Name", PORT_IP1, 620},
	{"name-sip1", "telcfg:settings/VoipExtension1/Name", PORT_IP2, 621},
	{"name-sip2", "telcfg:settings/VoipExtension2/Name", PORT_IP3, 622},
	{"name-sip3", "telcfg:settings/VoipExtension3/Name", PORT_IP4, 623},
	{"name-sip4", "telcfg:settings/VoipExtension4/Name", PORT_IP5, 624},
	{"name-sip5", "telcfg:settings/VoipExtension5/Name", PORT_IP6, 625},
	{"name-sip6", "telcfg:settings/VoipExtension6/Name", PORT_IP7, 626},
	{"name-sip7", "telcfg:settings/VoipExtension7/Name", PORT_IP8, 627},
	{"name-sip8", "telcfg:settings/VoipExtension8/Name", PORT_IP9, 628},
	{"name-sip9", "telcfg:settings/VoipExtension9/Name", PORT_IP10, 629},
};

/** FRITZ!Box plugin settings */
GSettings *fritzbox_settings = NULL;

/** Flag indicate whether tr64 functions should be used */
gboolean fritzbox_use_tr64 = FALSE;

/**
 * fritzbox_login:
 * @profile: a #RmProfile
 *
 * Main login function (depending on box type)
 *
 * Returns: error code
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
 * fritzbox_get_settings:
 * @profile: a #RmProfile
 *
 * Main get settings functions
 *
 * Returns: %TRUE when settings are read, otherwise %FALSE
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
 * fritzbox_load_journal:
 * @profile: a #RmProfile
 *
 * Main load journal function (big switch for each supported router)
 *
 * Returns: %TRUE when journal is loaded, %FALSE otherwise
 */
gboolean fritzbox_load_journal(RmProfile *profile)
{
	gboolean ret = FALSE;

	g_debug("%s(): called (%d.%d.%d)", __FUNCTION__, profile->router_info->maj_ver_id, profile->router_info->min_ver_id, profile->router_info->maj_ver_id);

	if (fritzbox_use_tr64) {
		ret = firmware_tr64_load_journal(profile);
	} else if (FIRMWARE_IS(5, 50)) {
		ret = fritzbox_load_journal_05_50(profile, NULL);
	} else if (FIRMWARE_IS(4, 0)) {
		ret = fritzbox_load_journal_04_74(profile, NULL);
	}

	return ret;
}

/**
 * fritzbox_clear_journal:
 * @profile: a #RmProfile
 *
 * Main clear journal function (big switch for each supported router)
 *
 * Returns: %TRUE if journal could be cleared, otherwise %FALSE
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

	if (fritzbox_use_tr64) {
		return firmware_tr64_dial_number(profile, port, number);
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


static RmConnection *dialer_connection = NULL;

RmConnection *fritzbox_phone_dialer_get_connection(void)
{
	return dialer_connection;
}

extern RmDevice *telnet_device;

gint fritzbox_get_phone_type(gchar *name)
{
	gint type = -1;
	gint index;

	for (index = 0; index < PORT_MAX - 2; index++) {
		gchar *fon = g_settings_get_string(fritzbox_settings, fritzbox_phone_ports[index].setting_name);

		if (!g_strcmp0(fon, name)) {
			type = fritzbox_phone_ports[index].type;
			break;
		}
	}

	return type;
}

RmConnection *dialer_dial(RmPhone *phone, const gchar *target, gboolean anonymous)
{
	gint type = fritzbox_get_phone_type(rm_phone_get_name(phone));

	if (fritzbox_dial_number(rm_profile_get_active(), type, target)) {
		rm_object_emit_message(R_("Phone dialer"), R_("Pickup phone when ringing"));
	}

	return NULL;
}

void dialer_hangup(RmConnection *connection)
{
	fritzbox_hangup(rm_profile_get_active(), -1, connection->remote_number);

	dialer_connection = NULL;
}

RmPhone dialer_phone = {
	NULL,
    "Phone dialer",
    dialer_dial,
    NULL,
    dialer_hangup,
    NULL,
    NULL,
    NULL,
    NULL
};

static void fritzbox_add_phone(gpointer name, gpointer user_data)
{
	RmPhone *phone = g_slice_new0(RmPhone);

	phone->dial = dialer_dial;
	phone->hangup = dialer_hangup;

	phone->name = name;
	phone->device = telnet_device;

	g_debug("%s(): Adding '%s'", __FUNCTION__, phone->name);

	rm_phone_register(phone);
}

static GPtrArray *fritzbox_get_phone_list(RmProfile *profile)
{
	GPtrArray *array = NULL;
	gchar *fon;
	gint index;

	if (!profile) {
		return array;
	}

	array = g_ptr_array_new();

	for (index = 0; index < PORT_MAX - 2; index++) {
		fon = g_settings_get_string(fritzbox_settings, fritzbox_phone_ports[index].setting_name);

		if (!RM_EMPTY_STRING(fon)) {
			g_ptr_array_add(array, fon);
			g_debug("%s(): Adding '%s'", __FUNCTION__, fon);
		}
	}

	return array;
}

/**
 * fritzbox_set_active:
 * @profile: a #RmProfile
 *
 * Set profiles selected router as active
 */
void fritzbox_set_active(RmProfile *profile)
{
	GPtrArray *array;

	fritzbox_settings = rm_settings_new_profile("org.tabos.rm.plugins.fritzbox", "fritzbox", (gchar*)rm_profile_get_name(profile));
	g_debug("%s(): Router set active, settings created", __FUNCTION__);

	array = fritzbox_get_phone_list(rm_profile_get_active());
	if (array) {
		g_ptr_array_foreach(array, fritzbox_add_phone, NULL);
		g_ptr_array_unref(array);
	}

	/* Check whether tr64 is available */
	fritzbox_use_tr64 = firmware_tr64_is_available(profile);
	g_debug("%s(): TR-064 %s", __FUNCTION__, fritzbox_use_tr64 ? "enabled" : "disabled");
}

static gboolean fritzbox_need_ftp(RmProfile *profile)
{
	return !fritzbox_use_tr64;
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
	fritzbox_need_ftp
};

/**
 * fritzbox_plugin_init:
 * @plugin: a #RmPlugin
 *
 * Activate plugin (register fritzbox router)
 *
 * Returns: %TRUE
 */
static gboolean fritzbox_plugin_init(RmPlugin *plugin)
{
	/* Register router structure */
	rm_router_register(&fritzbox);

	fritzbox_init_callmonitor();

	return TRUE;
}

/**
 * fritzbox_plugin_shutdown:
 * @plugin: a #RmPlugin
 *
 * Deactivate plugin
 *
 * Returns: %TRUE
 */
static gboolean fritzbox_plugin_shutdown(RmPlugin *plugin)
{
	fritzbox_shutdown_callmonitor();

	rm_phone_unregister(&dialer_phone);

	return TRUE;
}

PLUGIN(fritzbox);
