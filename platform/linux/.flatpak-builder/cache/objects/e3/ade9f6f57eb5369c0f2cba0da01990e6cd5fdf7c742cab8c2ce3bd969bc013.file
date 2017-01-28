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

#ifndef __RM_NUMBER_H
#define __RM_NUMBER_H

#include <rm/rmcontact.h>
#include <rm/rmprofile.h>

G_BEGIN_DECLS
/* current number format */
enum rm_number_format {
	RM_NUMBER_FORMAT_UNKNOWN,
	RM_NUMBER_FORMAT_LOCAL,
	RM_NUMBER_FORMAT_NATIONAL,
	RM_NUMBER_FORMAT_INTERNATIONAL,
	RM_NUMBER_FORMAT_INTERNATIONAL_PLUS
};
struct rm_call_by_call_entry {
	gchar *country_code;
	gchar *prefix;
	gint prefix_length;
};

gchar *rm_number_scramble(const gchar *number);
gchar *rm_number_full(const gchar *number, gboolean country_code_prefix);
gchar *rm_number_format(RmProfile *profile, const gchar *number, enum rm_number_format output_format);
gchar *rm_number_canonize(const gchar *number);

G_END_DECLS

#endif
