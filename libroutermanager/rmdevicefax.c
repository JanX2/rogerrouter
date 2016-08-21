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

#include <libroutermanager/rmdevicefax.h>
#include <libroutermanager/rmstring.h>

/** Internal fax list */
static GSList *rm_fax_plugins = NULL;

/**
 * rm_device_fax_register:
 * @fax: a #RmDeviceFax
 *
 * Register fax plugin.
 */
void rm_device_fax_register(RmDeviceFax *fax)
{
	g_debug("%s(): Registering %s", __FUNCTION__, fax->name);
	rm_fax_plugins = g_slist_prepend(rm_fax_plugins, fax);
}

