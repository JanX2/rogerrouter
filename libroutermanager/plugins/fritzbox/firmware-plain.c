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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/file.h>
#include <libroutermanager/logging.h>
#include <libroutermanager/network.h>
#include <libroutermanager/csv.h>
#include <libroutermanager/ftp.h>
#include <libroutermanager/call.h>
#include <libroutermanager/gstring.h>

#include "fritzbox.h"
#include "csv.h"
#include "firmware-common.h"
#include "firmware-plain.h"

/**
 * \brief Try to detect a FRITZ!Box router by simply access the start page
 * \param router_info router information structure
 * \return TRUE if FRITZ!Box is detected, otherwise FALSE on error
 */
gboolean fritzbox_present_plain(struct router_info *router_info)
{
	SoupMessage *msg;
	const gchar *data;
	gchar *url;
	gboolean ret = FALSE;

	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_info->host);
	msg = soup_message_new(SOUP_METHOD_GET, url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_warning("Could not login page (Error: %d)", msg->status_code);
		g_object_unref(msg);
		g_free(url);

		return ret;
	}

	data = msg->response_body->data;

	if (g_strcasestr(data, "fritz!box")) {
		ret = TRUE;

		router_info->name = g_strdup("FRITZ!Box");
		router_info->version = g_strdup(">= x.4.74");
		router_info->lang = g_strdup("de");
		router_info->serial = g_strdup("OLD");
		router_info->annex = g_strdup("");

		/* Force version to 4.74 as this is the minimum supported version */
		router_info->box_id = 0;
		router_info->maj_ver_id = 4;
		router_info->min_ver_id = 74;

	} else {
		ret = FALSE;
	}

	g_object_unref(msg);
	g_free(url);

	return ret;
}
