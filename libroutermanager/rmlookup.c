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

#include <libroutermanager/rmlookup.h>

/** Pointer to internal lookup function */
static RmLookup rm_lookup_active = NULL;

/** Lookup function list */
static GSList *rm_lookup_list = NULL;

/**
 * rm_lookup:
 * @number: number to lookup
 * @name: pointer to store name to
 * @address: pointer to store address to
 * @zip: pointer to store zip to
 * @city: pointer to store city to
 *
 * Lookup number and return name/address/zip/city
 *
 * Returns: %TRUE is lookup data has been found, otherwise %FALSE
 */
gboolean rm_lookup(gchar *number, gchar **name, gchar **address, gchar **zip, gchar **city)
{
	if (rm_lookup_active) {
		return rm_lookup_active(number, name, address, zip, city);
	}

	return FALSE;
}

/**
 * rm_lookup_register:
 * @lookup: a #RmLookup
 *
 * Register lookup routine.
 *
 * Returns: TRUE
 */
gboolean rm_lookup_register(RmLookup lookup)
{
	rm_lookup_list = g_slist_prepend(rm_lookup_list, lookup);
	rm_lookup_active = lookup;

	return TRUE;
}
