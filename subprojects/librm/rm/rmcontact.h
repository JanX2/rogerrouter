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

#ifndef __RM_CONTACT_H_
#define __RM_CONTACT_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	gint type;
	gchar *street;
	gchar *zip;
	gchar *city;
	gboolean lookup;
} RmContactAddress;

typedef struct {
	/* Name */
	gchar *name;
	/* Picture */
	gpointer image;
	/* Picture len */
	gsize image_len;
	/* Picture URI for online services */
	gchar *image_uri;
	/* Company */
	gchar *company;

	/* Phone numbers */
	GSList *numbers;
	/* Addresses */
	GSList *addresses;

	/* currently active number */
	gchar *number;

	/* Identified data based on active number */
	gchar *street;
	gchar *zip;
	gchar *city;
	gboolean lookup;

	/* Private data */
	gpointer priv;
} RmContact;

void rm_contact_copy(RmContact *src, RmContact *dst);
RmContact *rm_contact_dup(RmContact *src);
gint rm_contact_name_compare(gconstpointer a, gconstpointer b);
RmContact *rm_contact_find_by_number(gchar *number);
void rm_contact_free(RmContact *contact);

G_END_DECLS

#endif
