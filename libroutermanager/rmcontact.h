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

#ifndef LIBROUTERMANAGER_RMCONTACT_H
#define LIBROUTERMANAGER_RMCONTACT_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct contact_address {
	gint type;
	gchar *street;
	gchar *zip;
	gchar *city;
} RmContactAddress;

typedef struct contact {
	/* Name */
	gchar *name;
	/* Picture */
	gpointer image;
	/* Picture len */
	gsize image_len;
	/* Picture URI for online services */
	gchar *image_uri;

#if 1
	/* currently active number */
	gchar *number;
	gchar *company;
	gchar *street;
	gchar *zip;
	gchar *city;
	gboolean lookup;
#endif

	/* Phone numbers */
	GSList *numbers;
	/* Addresses */
	GSList *addresses;

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
