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

#include <libroutermanager/csv.h>
#include <libroutermanager/call.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/file.h>

#include <libroutermanager/plugins/fritzbox/csv.h>

/** This is our private header, not the one used by the router! */
#define ROUTERMANAGER_JOURNAL_HEADER "Typ;Datum;Name;Rufnummer;Nebenstelle;Eigene Rufnummer;Dauer"

/**
 * \brief Parse data as csv
 * \param data raw data to parse
 * \param header expected header line
 * \param csv_parse_line function pointer
 * \param ptr user pointer
 * \return user pointer
 */
gpointer csv_parse_data(const gchar *data, const gchar *header, csv_parse_line_func csv_parse_line, gpointer ptr)
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
 * \brief Save journal to local storage
 * \param journal journal list pointer
 * \param filename file name to store journal to
 * \return TRUE on success, otherwise FALSE
 */
gboolean csv_save_journal_as(GSList *journal, gchar *file_name)
{
	GSList *list;
	struct call *call;
	FILE *file;

	/* Open output file */
	file = fopen(file_name, "wb+");

	if (!file) {
		g_debug("Could not open journal output file %s", file_name);
		return FALSE;
	}

	fprintf(file, "sep=;\n");
	fprintf(file, ROUTERMANAGER_JOURNAL_HEADER);
	fprintf(file, "\n");

	for (list = journal; list; list = list->next) {
		call = list->data;

		if (call->type != CALL_TYPE_INCOMING && call->type != CALL_TYPE_OUTGOING && call->type != CALL_TYPE_MISSED && call->type != CALL_TYPE_BLOCKED) {
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
 * \brief Save journal to local storage
 * \param journal journal list pointer
 * \return TRUE on success, otherwise FALSE
 */
gboolean csv_save_journal(GSList *journal)
{
	struct profile *profile = profile_get_active();
	gchar *dir;
	gchar *file_name;
	gboolean ret;

	/* Build directory name and create it (if needed) */
	dir = g_build_filename(g_get_user_data_dir(), "routermanager", profile->name, NULL);
	g_mkdir_with_parents(dir, 0700);

	file_name = g_build_filename(dir, "journal.csv", NULL);

	ret = csv_save_journal_as(journal, file_name);

	g_free(dir);
	g_free(file_name);

	return ret;
}

/**
 * \brief Parse routermanager
 * \param ptr pointer to journal
 * \param split splitted line
 * \return pointer to journal with attached call line
 */
static inline gpointer csv_parse_routermanager(gpointer ptr, gchar **split)
{
	GSList *list = ptr;

	if (g_strv_length(split) == 7) {
		list = call_add(list, atoi(split[0]), split[1], split[2], split[3], split[4], split[5], split[6], NULL);
	}

	return list;
}

/**
 * \brief Parse journal data as csv
 * \param data raw data to parse
 * \return call list
 */
GSList *csv_parse_journal_data(GSList *list, const gchar *data)
{
	list = csv_parse_data(data, ROUTERMANAGER_JOURNAL_HEADER, csv_parse_routermanager, list);

	/* Return call list */
	return list;
}

/**
 * \brief Load saved journal
 * \param journal list pointer to fill
 * \return filled journal list
 */
GSList *csv_load_journal(GSList *journal)
{
	gchar *file_name;
	gchar *file_data;
	GSList *list = journal;
	struct profile *profile = profile_get_active();

	file_name = g_build_filename(g_get_user_data_dir(), "routermanager", profile->name, "journal.csv", NULL);
	file_data = file_load(file_name, NULL);
	g_free(file_name);

	if (file_data) {
		list = csv_parse_journal_data(journal, file_data);
		g_free(file_data);
	}

	return list;
}
