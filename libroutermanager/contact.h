/**
 * The libroutermanager project
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

struct contact {
	gchar *name;

	/* currently active number */
	gchar *number;
	gboolean lookup;

	gpointer image;
	gsize image_len;

	GSList *numbers;
	gchar *company;
	gchar *street;
	gchar *zip;
	gchar *city;
};

void contact_copy(struct contact *src, struct contact *dst);
struct contact *contact_dup(struct contact *src);

G_END_DECLS

#endif
