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

#ifndef LIBROUTERMANAGER_CALL_H
#define LIBROUTERMANAGER_CALL_H

#include <libroutermanager/contact.h>
#include <libroutermanager/profile.h>

G_BEGIN_DECLS

/** Supported call types */
enum {
	CALL_TYPE_ALL,
	CALL_TYPE_INCOMING,
	CALL_TYPE_MISSED,
	CALL_TYPE_OUTGOING,
	CALL_TYPE_VOICE,
	CALL_TYPE_FAX,
};

/* current number format */
enum number_format {
	NUMBER_FORMAT_UNKNOWN,
	NUMBER_FORMAT_LOCAL,
	NUMBER_FORMAT_NATIONAL,
	NUMBER_FORMAT_INTERNATIONAL,
	NUMBER_FORMAT_INTERNATIONAL_PLUS
};

struct call {
	gint type;
	gchar *date_time;
	gchar *duration;

	struct contact *remote;
	struct contact *local;

	/* Private (e.g. original filename) */
	gchar *priv;
};

struct call_by_call_entry {
	gchar *country_code;
	gchar *prefix;
	gint prefix_length;
};

GSList *call_add(GSList *journal, gint type, const gchar *date_time, const gchar *remote_name, const gchar *remote_number, const gchar *local_name, const gchar *local_number, const gchar *duration, gpointer priv);
void call_free(gpointer data);
gchar *call_scramble_number(const gchar *number);
gchar *call_full_number(const gchar *number, gboolean country_code_prefix);
gint call_sort_by_date(gconstpointer a, gconstpointer b);
gchar *call_format_number(struct profile *profile, const gchar *number, enum number_format output_format);
gchar *call_canonize_number(const gchar *number);

G_END_DECLS

#endif
