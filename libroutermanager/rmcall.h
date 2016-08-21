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

#ifndef LIBROUTERMANAGER_RMCALL_H
#define LIBROUTERMANAGER_RMCALL_H

#include <libroutermanager/rmcontact.h>
#include <libroutermanager/rmprofile.h>

G_BEGIN_DECLS

/** Supported call types */
enum rm_call_types {
	RM_CALL_TYPE_ALL,
	RM_CALL_TYPE_INCOMING,
	RM_CALL_TYPE_MISSED,
	RM_CALL_TYPE_OUTGOING,
	RM_CALL_TYPE_VOICE,
	RM_CALL_TYPE_FAX,
	RM_CALL_TYPE_FAX_REPORT,
	RM_CALL_TYPE_RECORD,
	RM_CALL_TYPE_BLOCKED
};

/* current number format */
enum rm_number_format {
	RM_NUMBER_FORMAT_UNKNOWN,
	RM_NUMBER_FORMAT_LOCAL,
	RM_NUMBER_FORMAT_NATIONAL,
	RM_NUMBER_FORMAT_INTERNATIONAL,
	RM_NUMBER_FORMAT_INTERNATIONAL_PLUS
};

typedef struct rm_call {
	enum rm_call_types type;
	gchar *date_time;
	gchar *duration;

	RmContact *remote;
	RmContact *local;

	/* Private (e.g. original filename) */
	gchar *priv;
} RmCall;

struct rm_call_by_call_entry {
	gchar *country_code;
	gchar *prefix;
	gint prefix_length;
};

GSList *rm_call_add(GSList *journal, enum rm_call_types type, const gchar *date_time, const gchar *remote_name, const gchar *remote_number, const gchar *local_name, const gchar *local_number, const gchar *duration, gpointer priv);
void rm_call_free(gpointer data);
gchar *rm_call_scramble_number(const gchar *number);
gchar *rm_call_full_number(const gchar *number, gboolean country_code_prefix);
gint rm_call_sort_by_date(gconstpointer a, gconstpointer b);
gchar *rm_call_format_number(struct profile *profile, const gchar *number, enum rm_number_format output_format);
gchar *rm_call_canonize_number(const gchar *number);

G_END_DECLS

#endif
