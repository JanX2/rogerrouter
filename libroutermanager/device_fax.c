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

#include <libroutermanager/device_fax.h>
#include <libroutermanager/gstring.h>

/** Internal fax list */
static GSList *fax_plugins = NULL;

/**
 * \brief Register phone plugin
 * \param phone phone plugin
 */
void rm_fax_register(struct device_fax *fax)
{
	g_debug("%s(): Registering %s", __FUNCTION__, fax->name);
	fax_plugins = g_slist_prepend(fax_plugins, fax);
}

