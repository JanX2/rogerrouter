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

#include <string.h>
#include <stdio.h>

#include <glib.h>

#include <rm/rmcallentry.h>
#include <rm/rmmain.h>
#include <rm/rmrouter.h>
#include <rm/rmobject.h>
#include <rm/rmfile.h>
#include <rm/rmplugins.h>
#include <rm/rmstring.h>
#include <rm/rmnumber.h>

#include "csv.h"

typedef struct {
	guint signal_id;
	GHashTable *table;
} RmGlobalAreaCodesPlugin;

/**
 * areacodes_get_area_code:
 * @areacodes_plugin: a #RmGlobalAreaCodesPlugin
 * @full_number: full phone number
 *
 * Get area code pointer based on full_number
 *
 * Returns: a #RmAreaCode
 */
RmAreaCode *areacodes_get_area_code(RmGlobalAreaCodesPlugin *areacodes_plugin, const gchar *full_number)
{
	RmAreaCode *areacode;
	gchar sub_string[6];
	gint index;
	gint len = strlen(full_number);

	index = len > 6 ? 6 : len;

	while (index-- > 0) {
		memset(sub_string, 0, sizeof(sub_string));
		strncpy(sub_string, full_number + 2, index);

		areacode = g_hash_table_lookup(areacodes_plugin->table, sub_string);
		if (areacode) {
			return areacode;
		}
	}

	return NULL;
}

/**
 * areacodes_get_city:
 * @areacodes_plugin: a #RmGlobalAreaCodesPlugin
 * @number: remote caller number
 *
 * Lookup city name by number
 *
 * Returns: city name or empty string
 */
static gchar *areacodes_get_city(RmGlobalAreaCodesPlugin *areacodes_plugin, gchar *number)
{
	RmAreaCode *areacode = NULL;
	gchar *full_number;
	gchar *ret = NULL;
	gint len;
	gchar sub_string[6];
	gchar *local_number;

	if (!areacodes_plugin->table) {
		return g_strdup("");
	}

	full_number = rm_number_full(number, TRUE);

	/* Find area code */
	areacode = areacodes_get_area_code(areacodes_plugin, full_number);

	if (!areacode) {
		g_free(full_number);
		return g_strdup("");
	}

	local_number = g_strdup_printf("0%s", full_number + 2 + areacode->skip);
	g_free(full_number);

	len = strlen(local_number);

	/* Check for a 3 char match */
	if (len >= 3 && !ret) {
		memset(sub_string, 0, sizeof(sub_string));
		strncpy(sub_string, local_number, 3);

		ret = g_hash_table_lookup(areacode->table, sub_string);
	}

	/* Check for a 4 char match */
	if (len >= 4 && !ret) {
		memset(sub_string, 0, sizeof(sub_string));
		strncpy(sub_string, local_number, 4);

		ret = g_hash_table_lookup(areacode->table, sub_string);
	}

	/* Check for a 5 char match */
	if (len >= 5 && !ret) {
		memset(sub_string, 0, sizeof(sub_string));
		strncpy(sub_string, local_number, 5);

		ret = g_hash_table_lookup(areacode->table, sub_string);
	}

	g_free(local_number);

	if (!ret) {
		return g_strdup("");
	}

	return rm_convert_utf8(ret, -1);
}

/**
 * areacodes_contact_process_cb:
 * @obj: a #RmObject
 * @contact: a #RmContact
 * @user_data: pointer to areacodes plugin structure
 *
 * Contact process callback (searches for areacodes and set city name)
 */
static void areacodes_contact_process_cb(RmObject *obj, RmContact *contact, gpointer user_data)
{
	RmGlobalAreaCodesPlugin *areacodes_plugin = user_data;

	if (RM_EMPTY_STRING(contact->number)) {
		return;
	}

	/* Free old city name */
	g_free(contact->city);

	/* Set new city name */
	contact->city = areacodes_get_city(areacodes_plugin, contact->number);
}

/**
 * impl_activate:
 * @plugin: a #PeasActivatable
 *
 * Activate plugin
 */
static gboolean areacodes_plugin_init(RmPlugin *plugin)
{
	RmGlobalAreaCodesPlugin *areacodes_plugin = g_slice_alloc0(sizeof(RmGlobalAreaCodesPlugin));
	gchar *areacodes = g_build_filename(rm_get_directory(RM_PLUGINS), "areacodes_global", "globalareacodes.csv", NULL);
	gchar *data;
	gsize read;

	g_debug("AreaCodes: '%s'", areacodes);

	plugin->priv = areacodes_plugin;

	/* Load data file */
	data = rm_file_load(areacodes, &read);
	if (!data || read < 1) {
		g_debug("Could not load areacodes: %s", areacodes);
		g_free(data);
		g_free(areacodes);

		return FALSE;
	}

	/* Make sure data is terminated */
	data[read - 1] = '\0';

	/* Parse data */
	areacodes_plugin->table = csv_parse_global_areacodes_data(data);

	/* Free data */
	g_free(data);
	g_free(areacodes);

	/* Connect to "contact-process" signal using "after" as this should come last */
	areacodes_plugin->signal_id = g_signal_connect_after(G_OBJECT(rm_object), "contact-process", G_CALLBACK(areacodes_contact_process_cb), areacodes_plugin);

	return TRUE;
}

/**
 * impl_deactivate:
 * @plugin: a #PeasActivatable
 *
 * Deactivate plugin
 */
static gboolean areacodes_plugin_shutdown(RmPlugin *plugin)
{
	RmGlobalAreaCodesPlugin *areacodes_plugin = plugin->priv;

	/* If signal handler is connected: disconnect */
	if (g_signal_handler_is_connected(G_OBJECT(rm_object), areacodes_plugin->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(rm_object), areacodes_plugin->signal_id);
	}

	/* Free hash table */
	if (areacodes_plugin->table) {
		g_hash_table_destroy(areacodes_plugin->table);
	}

	return TRUE;
}

PLUGIN(areacodes);
