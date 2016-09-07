/**
 * The libroutermanager project
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include <libroutermanager/rmcsv.h>
#include <libroutermanager/rmcall.h>
#include <libroutermanager/rmprofile.h>
#include <libroutermanager/rmfile.h>
#include <libroutermanager/rmjournal.h>

#include <libroutermanager/plugins/fritzbox/csv.h>

/**
 * SECTION:rmjournal
 * @title: RmJournal
 * @short_description: Journal handling functions
 *
 * Journal functions (adding calls, sorting, loading, storing)
 */

/** This is our private header, not the one used by the router! */
#define RM_JOURNAL_HEADER "Typ;Datum;Name;Rufnummer;Nebenstelle;Eigene Rufnummer;Dauer"

/**
 * rm_journal_save_as:
 * @journal: journal list pointer
 * @filename: file name to store journal to
 *
 * Save journal to local storage.
 *
 * Returns: %TRUE on success, otherwise %FALSE
 */
gboolean rm_journal_save_as(GSList *journal, gchar *file_name)
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
 * rm_journal_save:
 * @journal: journal list pointer
 *
 * Save journal to local storage.
 *
 * Returns: %TRUE on success, otherwise %FALSE
 */
gboolean rm_journal_save(GSList *journal)
{
	RmProfile *profile = rm_profile_get_active();
	gchar *dir;
	gchar *file_name;
	gboolean ret;

	/* Build directory name and create it (if needed) */
	dir = g_build_filename(g_get_user_data_dir(), "routermanager", profile->name, NULL);
	g_mkdir_with_parents(dir, 0700);

	file_name = g_build_filename(dir, "journal.csv", NULL);

	ret = rm_journal_save_as(journal, file_name);

	g_free(dir);
	g_free(file_name);

	return ret;
}

/**
 * rm_journal_csv_parse_rm:
 * @ptr: pointer to journal
 * @split: splitted line
 *
 * Parse routermanager csv.
 *
 * Returns: pointer to journal with attached call line
 */
static inline gpointer rm_journal_csv_parse_rm(gpointer ptr, gchar **split)
{
	GSList *list = ptr;

	if (g_strv_length(split) == 7) {
		RmCall *call = rm_call_new(atoi(split[0]), split[1], split[2], split[3], split[4], split[5], split[6], NULL);

		list = rm_journal_add_call(list, call);
	}

	return list;
}

/**
 * rm_journal_csv_parse:
 * @list: journal list
 * @data: raw data to parse
 *
 * Parse journal data as csv.
 *
 * Returns: new journal call list
 */
static GSList *rm_journal_csv_parse(GSList *list, const gchar *data)
{
	list = rm_csv_parse_data(data, RM_JOURNAL_HEADER, rm_journal_csv_parse_rm, list);

	/* Return call list */
	return list;
}

/**
 * rm_journal_load:
 * @journal: list pointer to fill
 *
 * Load saved journal
 *
 * Returns: filled journal list
 */
GSList *rm_journal_load(GSList *journal)
{
	gchar *file_name;
	gchar *file_data;
	GSList *list = journal;
	RmProfile *profile = rm_profile_get_active();

	file_name = g_build_filename(g_get_user_data_dir(), "routermanager", profile->name, "journal.csv", NULL);
	file_data = rm_file_load(file_name, NULL);
	g_free(file_name);

	if (file_data) {
		list = rm_journal_csv_parse(journal, file_data);
		g_free(file_data);
	}

	return list;
}


/**
 * rm_journal_sort_by_date:
 * @a: a #RmCall
 * @b: a #RmCall
 *
 * Sort journal calls (compares two calls based on date/time).
 *
 * Returns: see strncmp
 */
gint rm_journal_sort_by_date(gconstpointer a, gconstpointer b)
{
	RmCall *call_a = (RmCall *) a;
	RmCall *call_b = (RmCall *) b;
	gchar *number_a = NULL;
	gchar *number_b = NULL;
	gchar part_time_a[7];
	gchar part_time_b[7];
	gint ret = 0;

	if (!call_a || !call_b) {
		return 0;
	}

	if (call_a) {
		number_a = call_a->date_time;
	}

	if (call_b) {
		number_b = call_b->date_time;
	}

	/* Compare year */
	ret = strncmp(number_a + 6, number_b + 6, 2);
	if (ret == 0) {
		/* Compare month */
		ret = strncmp(number_a + 3, number_b + 3, 2);
		if (ret == 0) {
			/* Compare day */
			ret = strncmp(number_a, number_b, 2);
			if (ret == 0) {
				/* Extract time */
				memset(part_time_a, 0, sizeof(part_time_a));
				g_strlcpy(part_time_a, number_a + 9, 6);

				/* Extract time */
				memset(part_time_b, 0, sizeof(part_time_b));
				g_strlcpy(part_time_b, number_b + 9, 6);

				ret = g_utf8_collate(part_time_a, part_time_b);
			}
		}
	}

	return -ret;
}

/**
 * rm_journal_add_call:
 * @journal: call list
 * @call: a #RmCall
 *
 * Add call to journal
 *
 * Returns: new call list with appended call structure
 */
GSList *rm_journal_add_call(GSList *journal, RmCall *call)
{
	GSList *list = journal;
	RmCall *journal_call;

	/* Search through list and find duplicates */
	for (list = journal; list != NULL; list = list->next) {
		journal_call = list->data;

		/* Easier compare method, we are just interested in the complete date_time, remote_number and type field */
		if (!strcmp(journal_call->date_time, call->date_time) && !strcmp(journal_call->remote->number, call->remote->number)) {
			if (journal_call->type == call->type) {
				/* Call with the same type already exists, return unchanged journal */
				return journal;
			}

			/* Found same call with different type (voice/fax): merge them */
			if (call->type == RM_CALL_TYPE_VOICE || call->type == RM_CALL_TYPE_FAX) {
				journal_call->type = call->type;
				journal_call->priv = call->priv;

				return journal;
			}
		}
	}

	/* Append call sorted to the list */
	list = g_slist_insert_sorted(journal, call, rm_journal_sort_by_date);

	/* Return new call list */
	return list;
}
