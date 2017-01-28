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

#include <string.h>
#include <stdio.h>

#include <glib.h>

#include <libroutermanager/call.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/router.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/file.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/gstring.h>

#include "csv.h"

#define ROUTERMANAGER_TYPE_GLOBAL_AREACODES_PLUGIN (routermanager_global_areacodes_plugin_get_type ())
#define ROUTERMANAGER_GLOBAL_AREACODES_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_GLOBAL_AREACODES_PLUGIN, RouterManagerGlobalAreaCodesPlugin))

typedef struct {
	guint signal_id;
	GHashTable *table;
} RouterManagerGlobalAreaCodesPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_GLOBAL_AREACODES_PLUGIN, RouterManagerGlobalAreaCodesPlugin, routermanager_global_areacodes_plugin)

/**
 * \brief Get area code pointer based on full_number
 * \param areacodes_plugin private plugin data
 * \param full_number full phone number
 * \return pointer to areacode entry
 */
struct areacode *areacodes_get_area_code(RouterManagerGlobalAreaCodesPlugin *areacodes_plugin, const gchar *full_number)
{
	gchar sub_string[6];
	gint index;
	gint len = strlen(full_number);
	struct areacode *areacode;

	index = len > 6 ? 6 : len;

	while (index-- > 0) {
		memset(sub_string, 0, sizeof(sub_string));
		strncpy(sub_string, full_number + 2, index);

		//g_debug("sub_string: '%s'", sub_string);
		areacode = g_hash_table_lookup(areacodes_plugin->priv->table, sub_string);
		if (areacode) {
			return areacode;
		}
	}

	return NULL;
}

/**
 * \brief Lookup city name by number
 * \param areacodes_plugin areacodes plugin structure
 * \param number remote caller number
 * \return city name or empty string
 */
static gchar *areacodes_get_city(RouterManagerGlobalAreaCodesPlugin *areacodes_plugin, gchar *number)
{
	gchar *full_number;
	gchar *ret = NULL;
	gint len;
	gchar sub_string[6];
	gchar *local_number;
	struct areacode *areacode = NULL;

	if (!areacodes_plugin->priv->table) {
		return g_strdup("");
	}

	full_number = call_full_number(number, TRUE);
	//g_debug("full_number: '%s'", full_number);

	/* Find area code */
	areacode = areacodes_get_area_code(areacodes_plugin, full_number);

	if (!areacode) {
		g_free(full_number);
		return g_strdup("");
	}

	//g_debug("country: '%s'", areacode->country);

	local_number = g_strdup_printf("0%s", full_number + 2 + areacode->skip);
	g_free(full_number);

	len = strlen(local_number);

	/* Check for a 3 char match */
	if (len >= 3 && !ret) {
		memset(sub_string, 0, sizeof(sub_string));
		strncpy(sub_string, local_number, 3);

		//g_debug("Checking 3-match: %s", sub_string);

		ret = g_hash_table_lookup(areacode->table, sub_string);
	}

	/* Check for a 4 char match */
	if (len >= 4 && !ret) {
		memset(sub_string, 0, sizeof(sub_string));
		strncpy(sub_string, local_number, 4);

		//g_debug("Checking 4-match: %s", sub_string);

		ret = g_hash_table_lookup(areacode->table, sub_string);
	}

	/* Check for a 5 char match */
	if (len >= 5 && !ret) {
		memset(sub_string, 0, sizeof(sub_string));
		strncpy(sub_string, local_number, 5);

		//g_debug("Checking 5-match: %s", sub_string);

		ret = g_hash_table_lookup(areacode->table, sub_string);
	}

	g_free(local_number);

	if (!ret) {
		//g_debug("Not found");
		return g_strdup("");
	}

	//g_debug("Ret: %s", ret);
	return g_convert_utf8(ret, -1);
}

/**
 * \brief Contact process callback (searches for areacodes and set city name)
 * \param obj app object
 * \param contact contact structure
 * \param user_data pointer to areacodes plugin structure
 */
static void global_areacodes_contact_process_cb(AppObject *obj, struct contact *contact, gpointer user_data)
{
	RouterManagerGlobalAreaCodesPlugin *areacodes_plugin = user_data;

	if (EMPTY_STRING(contact->number)) {
		return;
	}

	/* Free old city name */
	g_free(contact->city);

	/* Set new city name */
	contact->city = areacodes_get_city(areacodes_plugin, contact->number);
}

/**
 * \brief Activate plugin
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	RouterManagerGlobalAreaCodesPlugin *areacodes_plugin = ROUTERMANAGER_GLOBAL_AREACODES_PLUGIN(plugin);
	gchar *areacodes = g_build_filename(get_directory(ROUTERMANAGER_PLUGINS), "areacodes_global", "globalareacodes.csv", NULL);
	gchar *data;
	gsize read;

	g_debug("AreaCodes: '%s'", areacodes);

	/* Load data file */
	data = file_load(areacodes, &read);
	if (!data || read < 1) {
		g_debug("Could not load areacodes: %s", areacodes);
		g_free(data);
		g_free(areacodes);

		return;
	}

	/* Make sure data is terminated */
	data[read - 1] = '\0';

	/* Parse data */
	areacodes_plugin->priv->table = csv_parse_global_areacodes_data(data);

	/* Free data */
	g_free(data);
	g_free(areacodes);

	/* Connect to "contact-process" signal using "after" as this should come last */
	areacodes_plugin->priv->signal_id = g_signal_connect_after(G_OBJECT(app_object), "contact-process", G_CALLBACK(global_areacodes_contact_process_cb), areacodes_plugin);
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerGlobalAreaCodesPlugin *areacodes_plugin = ROUTERMANAGER_GLOBAL_AREACODES_PLUGIN(plugin);

	/* If signal handler is connected: disconnect */
	if (g_signal_handler_is_connected(G_OBJECT(app_object), areacodes_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), areacodes_plugin->priv->signal_id);
	}

	/* Free hash table */
	if (areacodes_plugin->priv->table) {
		g_hash_table_destroy(areacodes_plugin->priv->table);
	}
}
