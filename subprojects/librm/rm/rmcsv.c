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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include <rm/rmcsv.h>
#include <rm/rmcallentry.h>
#include <rm/rmprofile.h>
#include <rm/rmfile.h>

//#include <rm/plugins/fritzbox/csv.h>

/**
 * SECTION:rmcsv
 * @title: RmCsv
 * @short_description: CSV handling functions
 *
 * CSV files are used for journals and address book plugins.
 */

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
		g_debug("%s(): Unknown CSV-Header = '%s'", __FUNCTION__, lines[index]);
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
