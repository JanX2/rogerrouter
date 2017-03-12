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

#include <rmconfig.h>

#include <string.h>
#include <glib.h>

#include <rm/rmfax.h>
#include <rm/rmstring.h>

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

/**
 * rm_fax_get_plugins:
 *
 * Return list of fax plugins
 *
 * Returns: a list of fax plugins
 */
GSList *rm_fax_get_plugins(void)
{
	return rm_fax_plugins;
}

/**
 * rm_fax_get_name:
 * @fax: a #RmFax
 *
 * Return name of fax device
 *
 * Returns: fax name
 */
gchar *rm_fax_get_name(RmFax *fax)
{
	return g_strdup(fax ? fax->name : "");
}

/**
 * rm_fax_get_status:
 * @fax: a #RmFax
 * @connection: a #RmConnection
 * @fax_status: a #RmFaxStatus
 *
 * Retrieve fax status in @fax_status for given fax device @fax with connection @connection.
 *
 * Returns: %TRUE if status has been received, %FALSE on error
 */
gboolean rm_fax_get_status(RmFax *fax, RmConnection *connection, RmFaxStatus *fax_status)
{
	return fax ? fax->get_status(connection, fax_status) : FALSE;
}

/**
 * rm_fax_send:
 * @fax: a #RmFax device
 * @file: file name to transfer
 * @target: target phone number
 * @anonymous: flag indicating if we should create an anonymous connection
 *
 * Starts fax file transfer.
 *
 * Returns: a #RmConnection for the fax transfer
 */
RmConnection *rm_fax_send(RmFax *fax, gchar *file, const gchar *target, gboolean anonymous)
{
	return fax ? fax->send(file, target, anonymous) : NULL;
}

/**
 * rm_fax_get:
 * @name: Name of fax device
 *
 * Retrieve fax device by name.
 *
 * Returns: a #RmFax device or %NULL on error
 */
RmFax *rm_fax_get(gchar *name)
{
	GSList *list;

	for (list = rm_fax_plugins; list != NULL; list = list->next) {
		RmFax *fax = list->data;

		if (fax && fax->name && name && !strcmp(fax->name, name)) {
			return fax;
		}
	}

	/* Temporary workaround */
	if (rm_fax_plugins) {
		return rm_fax_plugins->data;
	}

	return NULL;
}

/**
 * rm_fax_hangup:
 * @fax: a #RmFax device
 * @connection: a #RmConnection
 *
 * Hangup fax @connection on device @fax.
 */
void rm_fax_hangup(RmFax *fax, RmConnection *connection)
{
	if (fax) {
		fax->hangup(connection);
	}
}

