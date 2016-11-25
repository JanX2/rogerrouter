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

#include <config.h>

#include <string.h>
#include <glib.h>

#include <libroutermanager/rmfax.h>
#include <libroutermanager/rmstring.h>

/**
 * SECTION:rmfax
 * @title: RmFax
 * @short_description: Fax device implementation
 *
 * Wrapper for fax device functions.
 */

/** Internal fax list */
static GSList *rm_fax_plugins = NULL;

/**
 * rm_fax_register:
 * @fax: a #RmFax
 *
 * Register fax plugin.
 */
void rm_fax_register(RmFax *fax)
{
	g_debug("%s(): Registering %s", __FUNCTION__, fax->name);
	rm_fax_plugins = g_slist_prepend(rm_fax_plugins, fax);
}

GSList *rm_fax_get_plugins(void)
{
	return rm_fax_plugins;
}

gchar *rm_fax_get_name(RmFax *fax)
{
	return g_strdup(fax ? fax->name : "");
}

gboolean rm_fax_get_status(RmFax *fax, RmConnection *connection, RmFaxStatus *fax_status)
{
	return fax ? fax->get_status(connection, fax_status) : FALSE;
}

RmConnection *rm_fax_send(RmFax *fax, gchar *file, const gchar *target, gboolean anonymous)
{
	return fax ? fax->send(file, target, anonymous) : NULL;
}

RmFax *rm_fax_get(gchar *name)
{
	GSList *list;

	for (list = rm_fax_plugins; list != NULL; list = list->next) {
		RmFax *fax = list->data;

		if (fax && fax->name && name && !strcmp(fax->name, name)) {
			return fax;
		}
	}

	return NULL;
}

void rm_fax_hangup(RmFax *fax, RmConnection *connection)
{
	if (fax) {
		fax->hangup(connection);
	}
}
