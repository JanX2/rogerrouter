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

/**
 * TODO:
 *  - Asynchronous lookup
 */

#include <glib.h>

#include <libroutermanager/lookup.h>

/** Pointer to internal lookup function */
static lookup_func lookup_active = NULL;

/** Lookup function list */
static GSList *lookup_list = NULL;

/**
 * \brief Lookup number and return name/address/zip/city
 * \param number number to lookup
 * \param name pointer to store name to
 * \param address pointer to store address to
 * \param zip pointer to store zip to
 * \param city pointer to store city to
 * \return TRUE is lookup data has been found, otherwise FALSE
 */
gboolean routermanager_lookup(gchar *number, gchar **name, gchar **address, gchar **zip, gchar **city)
{
	if (lookup_active) {
		return lookup_active(number, name, address, zip, city);
	}

	return FALSE;
}

/**
 * \brief Register lookup routine
 * \return TRUE
 */
gboolean routermanager_lookup_register(lookup_func lookup)
{
	lookup_list = g_slist_prepend(lookup_list, lookup);
	lookup_active = lookup;

	return TRUE;
}
