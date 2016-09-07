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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include <libroutermanager/rmcall.h>
#include <libroutermanager/rmstring.h>
#include <libroutermanager/rmprofile.h>
#include <libroutermanager/router.h>

/**
 * SECTION:rmcall
 * @title: RmCall
 * @short_description: Call tracking functions
 *
 * Call keeps track of ongoing active calls.
 */

/**
 * rm_call_new:
 * @type: call type
 * @date_time: date and time of call
 * @remote_name: remote caller name
 * @remote_number: remote caller number
 * @local_name: local caller name
 * @local_number: local caller number
 * @duration: call duration
 * @priv: private data
 *
 * Creates a new #RmCall
 *
 * Returns: new #RmCall
 */
RmCall *rm_call_new(enum rm_call_types type, const gchar *date_time, const gchar *remote_name, const gchar *remote_number, const gchar *local_name, const gchar *local_number, const gchar *duration, gpointer priv)
{
	RmCall *call;

	/* Create new call structure */
	call = g_slice_new0(RmCall);

	/* Set entries */
	call->type = type;
	call->date_time = date_time ? g_strdup(date_time) : g_strdup("");
	call->remote = g_slice_new0(RmContact);
	call->remote->image = NULL;
	call->remote->name = remote_name ? rm_convert_utf8(remote_name, -1) : g_strdup("");
	call->remote->number = remote_number ? g_strdup(remote_number) : g_strdup("");
	call->local = g_slice_new0(RmContact);
	call->local->name = local_name ? rm_convert_utf8(local_name, -1) : g_strdup("");
	call->local->number = local_number ? g_strdup(local_number) : g_strdup("");
	call->duration = duration ? g_strdup(duration) : g_strdup("");

	/* Extended */
	call->remote->company = g_strdup("");
	call->remote->city = g_strdup("");
	call->priv = priv;

	return call;
}

/**
 * rm_call_free:
 * @data: pointer to call structure
 *
 * Free call structure.
 */
void rm_call_free(gpointer data)
{
	RmCall *call = data;

	/* Free entries */
	g_free(call->date_time);
	g_free(call->remote->name);
	g_free(call->remote->number);
	g_free(call->local->name);
	g_free(call->local->number);
	g_free(call->duration);

	/* Extended */
	g_free(call->remote->company);
	g_free(call->remote->city);

	/* Free structure */
	g_slice_free(RmContact, call->remote);
	g_slice_free(RmContact, call->local);
	g_slice_free(RmCall, call);
}
