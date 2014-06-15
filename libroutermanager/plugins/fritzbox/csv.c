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
#include <stdio.h>

#include <glib.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/call.h>
#include <libroutermanager/csv.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/logging.h>

#include "csv.h"
#include "firmware-common.h"

/**
 * \brief Parse FRITZ!Box "Anruferliste"
 * \param ptr pointer to journal
 * \param split splitted line
 * \return pointer to journal with attached call line
 */
static inline gpointer csv_parse_fritzbox(gpointer ptr, gchar **split)
{
	GSList *list = ptr;

	if (g_strv_length(split) == 7) {
		gint call_type = 0;

		switch (atoi(split[0])) {
		case 1:
			call_type = CALL_TYPE_INCOMING;
			break;
		case 2:
			call_type = CALL_TYPE_MISSED;
			break;
		case 3:
		case 4:
			call_type = CALL_TYPE_OUTGOING;
			break;
		}

		list = call_add(list, call_type, split[1], split[2], split[3], split[4], split[5], split[6], NULL);
	}

	return list;
}

/**
 * \brief Parse journal data as csv
 * \param data raw data to parse
 * \return call list
 */
GSList *csv_parse_fritzbox_journal_data(GSList *list, const gchar *data)
{
	GSList *new_list = NULL;

	new_list = csv_parse_data(data, CSV_FRITZBOX_JOURNAL_DE, csv_parse_fritzbox, list);
	if (!new_list) {
		new_list = csv_parse_data(data, CSV_FRITZBOX_JOURNAL_EN, csv_parse_fritzbox, list);
		if (!new_list) {
			new_list = csv_parse_data(data, CSV_FRITZBOX_JOURNAL_EN2, csv_parse_fritzbox, list);
		}
	}

	if (!new_list) {
		log_save_data("journal.csv", data, strlen(data));
	}

	/* Return call list */
	return new_list;
}
