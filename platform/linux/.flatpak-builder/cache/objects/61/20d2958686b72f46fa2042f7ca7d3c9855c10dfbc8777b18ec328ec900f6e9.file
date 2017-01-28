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

#ifndef LIBROUTERMANAGER_CONTACT_H
#define LIBROUTERMANAGER_CONTACT_H

#include <glib.h>

G_BEGIN_DECLS

struct contact_address {
	gint type;
	gchar *street;
	gchar *zip;
	gchar *city;
};

struct contact {
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
};

void contact_copy(struct contact *src, struct contact *dst);
struct contact *contact_dup(struct contact *src);
gint contact_name_compare(gconstpointer a, gconstpointer b);
struct contact *contact_find_by_number(gchar *number);
void contact_free(struct contact *contact);

G_END_DECLS

#endif
