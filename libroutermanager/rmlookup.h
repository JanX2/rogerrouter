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

#ifndef LIBROUTERMANAGER_RMLOOKUP_H
#define LIBROUTERMANAGER_RMLOOKUP_H

G_BEGIN_DECLS

typedef struct {
	gchar *name;
	gboolean (*search)(gchar *number, gchar **name, gchar **address, gchar **zip, gchar **city);
} RmLookup;

RmLookup *rm_lookup_get(gchar *name);
gboolean rm_lookup_search(RmLookup *lookup, gchar *number, gchar **name, gchar **address, gchar **zip, gchar **city);
gboolean rm_lookup_register(RmLookup *lookup);
gboolean rm_lookup_unregister(RmLookup *lookup);

G_END_DECLS

#endif
