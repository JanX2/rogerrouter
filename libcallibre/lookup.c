/**
 * The libcallibre project
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

/**
 * TODO:
 *  - Asynchronous lookup
 *  - Support multiple lookup functions
 */

#include <glib.h>

#include <libcallibre/lookup.h>

/** Pointer to internal lookup function */
static lookup_func *internal_lookup = NULL;

/**
 * \brief Lookup number and return name/address/zip/city
 * \param number number to lookup
 * \param name pointer to store name to
 * \param address pointer to store address to
 * \param zip pointer to store zip to
 * \param city pointer to store city to
 * \return TRUE is lookup data has been found, otherwise FALSE
 */
gboolean callibre_lookup(gchar *number, gchar **name, gchar **address, gchar **zip, gchar **city)
{
	if (internal_lookup) {
		return internal_lookup(number, name, address, zip, city);
	}

	return FALSE;
}

/**
 * \brief Register lookup routine
 * \return TRUE
 */
gboolean callibre_lookup_register(lookup_func lookup)
{
	internal_lookup = lookup;

	return TRUE;
}
