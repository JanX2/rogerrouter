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

/**
 * TODO:
 *  - Asynchronous lookup
 */

#include <string.h>

#include <glib.h>

#include <rm/rmlookup.h>

/**
 * SECTION:rmlookup
 * @title: RmLookup
 * @short_description: Reverse lookup
 * @stability: Stable
 *
 * Reverse lookup of telephone numbers using online services
 */

/** Lookup function list */
static GSList *rm_lookup_plugins = NULL;

/**
 * rm_lookup_get:
 * @name: name of address book to lookup
 *
 * Find address book as requested by name.
 *
 * Returns: a #RmLookup, or %NULL on error
 */
RmLookup *rm_lookup_get(gchar *name)
{
	GSList *list;

	if (!name) {
		g_warning("%s(): requested invalid name", __FUNCTION__);
		return NULL;
	}

	for (list = rm_lookup_plugins; list != NULL; list = list->next) {
		RmLookup *lookup = list->data;

		if (lookup && lookup->name && !strcmp(lookup->name, name)) {
			return lookup;
		}
	}

	return NULL;
}

/**
 * rm_lookup_search:
 * @number: number to lookup
 * @contact: a #RmContact to store data to
 *
 * Lookup number and return name/address/zip/city
 *
 * Returns: %TRUE is lookup data has been found, otherwise %FALSE
 */
gboolean rm_lookup_search(gchar *number, RmContact *contact)
{
	GSList *list;

	for (list = rm_lookup_plugins; list != NULL; list = list->next) {
		RmLookup *lookup = list->data;

		if (lookup->search(number, contact)) {
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * rm_lookup_register:
 * @lookup: a #RmLookup
 *
 * Register lookup routine.
 *
 * Returns: %TRUE
 */
gboolean rm_lookup_register(RmLookup *lookup)
{
	rm_lookup_plugins = g_slist_prepend(rm_lookup_plugins, lookup);

	return TRUE;
}

/**
 * rm_lookup_unregister:
 * @lookup: a #RmLookup
 *
 * Unregister lookup routine.
 *
 * Returns: %TRUE
 */
gboolean rm_lookup_unregister(RmLookup *lookup)
{
	rm_lookup_plugins = g_slist_remove(rm_lookup_plugins, lookup);

	return TRUE;
}
