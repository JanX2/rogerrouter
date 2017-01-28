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

#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include <rm/rmcsv.h>
#include <csv.h>

#define CSV_AREACODES "\"Country\",\"Country Code\",\"Area\",\"Area Code\""

/**
 * csv_trim:
 * @str_in: input string
 *
 * Trim csv string
 *
 * Returns: trimmed string
 */
static gchar *csv_trim(const gchar *str_in)
{
	gchar *str_out;
	gint len = strlen(str_in) - 1;

	if (len <= 0) {
		return g_strdup("");
	}

	str_out = g_malloc0(len);
	strncpy(str_out, str_in + 1, len - 1);

	return str_out;
}

/**
 * csv_parse_global_areacodes:
 * @ptr: hashtable pointer
 * @split: splitted line
 *
 * Parse areacodes line
 *
 * Returns: hashtable pointer
 */
static gpointer csv_parse_global_areacodes(gpointer ptr, gchar **split)
{
	GHashTable *global_table = ptr;

	/* If we have 4 fields add it to table */
	if (g_strv_length(split) == 4) {
		RmAreaCode *areacode;

		areacode = g_hash_table_lookup(global_table, split[1]);
		if (!areacode) {
			areacode = g_slice_new(RmAreaCode);
			areacode->country = csv_trim(split[0]);
			areacode->skip = strlen(split[1]);

			areacode->table = g_hash_table_new(g_str_hash, g_str_equal);
			g_hash_table_insert(global_table, g_strdup(split[1]), areacode);
		}

		if (strlen(split[2]) && strlen(split[3])) {
			g_hash_table_insert(areacode->table, csv_trim(split[3]), csv_trim(split[2]));
		}
	}

	return ptr;
}

/**
 * csv_data_destroy:
 * @data: data pointer
 *
 * Free key data of hash table
 */
static void csv_data_destroy(gpointer data)
{
	g_free(data);
}

/**
 * csv_areacode_destroy:
 * @data: data pointer
 *
 * Free value data of hash table
 */
static void csv_areacode_destroy(gpointer data)
{
	RmAreaCode *areacode = data;

	g_free(areacode->country);

	if (areacode->table) {
		g_hash_table_destroy(areacode->table);
	}

	g_slice_free(RmAreaCode, data);
}

/**
 * csv_parse_global_areacodes_data:
 * @data: input data from areacodes file
 *
 * Parse Areacodes data
 *
 * Returns: hashtable pointer
 */
GHashTable *csv_parse_global_areacodes_data(gchar *data)
{
	GHashTable *global_table = NULL;

	global_table = g_hash_table_new_full(g_str_hash, g_str_equal, csv_data_destroy, csv_areacode_destroy);

	rm_csv_parse_data(data, CSV_AREACODES, csv_parse_global_areacodes, global_table);

	return global_table;
}
