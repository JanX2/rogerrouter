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

/**
 * \TODO: Split general csv function and routermanager journal functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include <libroutermanager/rmcsv.h>
#include <libroutermanager/rmcall.h>
#include <libroutermanager/rmprofile.h>
#include <libroutermanager/rmfile.h>

#include <libroutermanager/plugins/fritzbox/csv.h>

/** This is our private header, not the one used by the router! */
#define RM_JOURNAL_HEADER "Typ;Datum;Name;Rufnummer;Nebenstelle;Eigene Rufnummer;Dauer"

/**
 * rm_csv_parse_data:
 * @data: raw data to parse
 * @header expected: header line
 * @csv_parse_line: a function pointer
 * @ptr: user pointer
 *
 * Parse data as csv.
 *
 * Returns: user pointer
 */
gpointer rm_csv_parse_data(const gchar *data, const gchar *header, rm_csv_parse_line_func csv_parse_line, gpointer ptr)
{
	gint index = 0;
	gchar sep[2];
	gchar **lines = NULL;
	gchar *pos;
	gpointer data_ptr = ptr;

	/* Safety check */
	g_assert(data != NULL);

	/* Split data to lines */
	lines = g_strsplit(data, "\n", -1);

	/* Check for separator */
	pos = g_strstr_len(lines[index], -1, "sep=");
	if (pos) {
		sep[0] = pos[4];
		index++;
	} else {
		sep[0] = ',';
	}
	sep[1] = '\0';

	/* Check header */
	if (strncmp(lines[index], header, strlen(header))) {
		g_debug("Unknown CSV-Header: '%s'", lines[index]);
		data_ptr = NULL;
		goto end;
	}

	/* Parse each line, split it and use parse function */
	while (lines[++index] != NULL) {
		gchar **split = g_strsplit(lines[index], sep, -1);

		data_ptr = csv_parse_line(data_ptr, split);

		g_strfreev(split);
	}

end:
	g_strfreev(lines);

	/* Return ptr */
	return data_ptr;
}

/**
 * rm_csv_save_journal_as:
 * @journal: journal list pointer
 * @filename: file name to store journal to
 *
 * Save journal to local storage.
 *
 * Returns: %TRUE on success, otherwise %FALSE
 */
gboolean rm_csv_save_journal_as(GSList *journal, gchar *file_name)
{
	GSList *list;
	RmCall *call;
	FILE *file;

	/* Open output file */
	file = fopen(file_name, "wb+");

	if (!file) {
		g_debug("Could not open journal output file %s", file_name);
		return FALSE;
	}

	fprintf(file, "sep=;\n");
	fprintf(file, RM_JOURNAL_HEADER);
	fprintf(file, "\n");

	for (list = journal; list; list = list->next) {
		call = list->data;

		if (call->type != RM_CALL_TYPE_INCOMING && call->type != RM_CALL_TYPE_OUTGOING && call->type != RM_CALL_TYPE_MISSED && call->type != RM_CALL_TYPE_BLOCKED) {
			continue;
		}

		gchar *name = g_convert(call->remote->name, -1, "iso-8859-1", "UTF-8", NULL, NULL, NULL);
		fprintf(file, "%d;%s;%s;%s;%s;%s;%s\n",
		        call->type,
		        call->date_time,
		        name,
		        call->remote->number,
		        call->local->name,
		        call->local->number,
		        call->duration);
		g_free(name);
	}

	fclose(file);

	return TRUE;
}

/**
 * rm_csv_save_journal:
 * @journal: journal list pointer
 *
 * Save journal to local storage.
 *
 * Returns: %TRUE on success, otherwise %FALSE
 */
gboolean rm_csv_save_journal(GSList *journal)
{
	RmProfile *profile = rm_profile_get_active();
	gchar *dir;
	gchar *file_name;
	gboolean ret;

	/* Build directory name and create it (if needed) */
	dir = g_build_filename(g_get_user_data_dir(), "routermanager", profile->name, NULL);
	g_mkdir_with_parents(dir, 0700);

	file_name = g_build_filename(dir, "journal.csv", NULL);

	ret = rm_csv_save_journal_as(journal, file_name);

	g_free(dir);
	g_free(file_name);

	return ret;
}

/**
 * rm_csv_parse_rm:
 * @ptr: pointer to journal
 * @split: splitted line
 *
 * Parse routermanager csv.
 *
 * Returns: pointer to journal with attached call line
 */
static inline gpointer rm_csv_parse_rm(gpointer ptr, gchar **split)
{
	GSList *list = ptr;

	if (g_strv_length(split) == 7) {
		list = rm_call_add(list, atoi(split[0]), split[1], split[2], split[3], split[4], split[5], split[6], NULL);
	}

	return list;
}

/**
 * rm_csv_parse_journal_data:
 * @list: journal list
 * @data: raw data to parse
 *
 * Parse journal data as csv.
 *
 * Returns: new journal call list
 */
GSList *rm_csv_parse_journal_data(GSList *list, const gchar *data)
{
	list = rm_csv_parse_data(data, RM_JOURNAL_HEADER, rm_csv_parse_rm, list);

	/* Return call list */
	return list;
}

/**
 * rm_csv_load_journal:
 * @journal: list pointer to fill
 *
 * Load saved journal
 *
 * Returns: filled journal list
 */
GSList *rm_csv_load_journal(GSList *journal)
{
	gchar *file_name;
	gchar *file_data;
	GSList *list = journal;
	RmProfile *profile = rm_profile_get_active();

	file_name = g_build_filename(g_get_user_data_dir(), "routermanager", profile->name, "journal.csv", NULL);
	file_data = rm_file_load(file_name, NULL);
	g_free(file_name);

	if (file_data) {
		list = rm_csv_parse_journal_data(journal, file_data);
		g_free(file_data);
	}

	return list;
}
