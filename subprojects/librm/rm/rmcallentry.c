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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include <rm/rmcallentry.h>
#include <rm/rmstring.h>
#include <rm/rmprofile.h>
#include <rm/rmrouter.h>

/**
 * SECTION:rmcallentry
 * @title: RmCallEntry
 * @short_description: Call entry tracking functions
 *
 * Call entry keeps track of all call entries.
 */

/**
 * rm_call_entry_new:
 * @type: call entry type
 * @date_time: date and time of call
 * @remote_name: remote caller name
 * @remote_number: remote caller number
 * @local_name: local caller name
 * @local_number: local caller number
 * @duration: call duration
 * @priv: private data
 *
 * Creates a new #RmCallEntry
 *
 * Returns: new #RmCallEntry
 */
RmCallEntry *rm_call_entry_new(enum rm_call_entry_types type, const gchar *date_time, const gchar *remote_name, const gchar *remote_number, const gchar *local_name, const gchar *local_number, const gchar *duration, gpointer priv)
{
	RmCallEntry *call_entry;

	/* Create new call entry structure */
	call_entry = g_slice_new0(RmCallEntry);

	/* Set entries */
	call_entry->type = type;
	call_entry->date_time = date_time ? g_strdup(date_time) : g_strdup("");
	call_entry->remote = g_slice_new0(RmContact);
	call_entry->remote->image = NULL;
	call_entry->remote->name = remote_name ? rm_convert_utf8(remote_name, -1) : g_strdup("");
	call_entry->remote->number = remote_number ? g_strdup(remote_number) : g_strdup("");
	call_entry->local = g_slice_new0(RmContact);
	call_entry->local->name = local_name ? rm_convert_utf8(local_name, -1) : g_strdup("");
	call_entry->local->number = local_number ? g_strdup(local_number) : g_strdup("");
	call_entry->duration = duration ? g_strdup(duration) : g_strdup("");

	/* Extended */
	call_entry->remote->company = g_strdup("");
	call_entry->remote->city = g_strdup("");
	call_entry->priv = priv;

	//g_debug("%s(): %d / %s / %s / %s", __FUNCTION__, call_entry->type, call_entry->date_time, call_entry->remote->number, call_entry->local->number);

	return call_entry;
}

/**
 * rm_call_entry_free:
 * @data: pointer to call entry structure
 *
 * Free call entry structure.
 */
void rm_call_entry_free(gpointer data)
{
	RmCallEntry *call_entry = data;

	/* Free entries */
	g_free(call_entry->date_time);
	g_free(call_entry->remote->name);
	g_free(call_entry->remote->number);
	g_free(call_entry->local->name);
	g_free(call_entry->local->number);
	g_free(call_entry->duration);

	/* Extended */
	g_free(call_entry->remote->company);
	g_free(call_entry->remote->city);

	/* Free structure */
	g_slice_free(RmContact, call_entry->remote);
	g_slice_free(RmContact, call_entry->local);
	g_slice_free(RmCallEntry, call_entry);
}
