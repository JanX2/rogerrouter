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
	gsize read;

	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_info->host);
	msg = soup_message_new(SOUP_METHOD_GET, url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_warning("Could not plain present page (Error: %d)", msg->status_code);
		g_object_unref(msg);
		g_free(url);

		return ret;
	}

	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-plain-present.html", data, read);
	g_assert(data != NULL);

	if (g_strcasestr(data, "fritz!box")) {
		ret = TRUE;

		g_debug("Found old fritzbox router...");
		router_info->name = g_strdup("FRITZ!Box");
		router_info->version = g_strdup(">= x.4.y");
		router_info->lang = g_strdup("de");
		router_info->serial = g_strdup("OLD");
		router_info->annex = g_strdup("");

		/* This is a fritz!box router, but which version.... */
		//if (g_strcasestr(data, "login:command/password")) {
			/* Force version to 4.0: Seems to have the plain old login structure */
		//	router_info->box_id = 0;
		//	router_info->maj_ver_id = 4;
		//	router_info->min_ver_id = 0;
		//} else {
			/* Force version to 4.74: Seems to have the new login structure */
			router_info->box_id = 0;
			router_info->maj_ver_id = 4;
			router_info->min_ver_id = 74;
		//}
		g_debug("Version: %d.%d.%d", router_info->box_id, router_info->maj_ver_id, router_info->min_ver_id);
	} else {
		ret = FALSE;
	}

	g_object_unref(msg);
	g_free(url);

	return ret;
}

gboolean fritzbox_login_plain(struct profile *profile)
{
	SoupMessage *msg;
	const gchar *data;
	gchar *url;
	gboolean ret = FALSE;
	gsize read;
	gchar *password;

	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));

	password = router_get_login_password(profile);
	msg = soup_form_request_new(SOUP_METHOD_POST, url,
				    "login:command/password", password,
				    "var:loginDone", "1",
				    NULL);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_warning("Could not plain login page (Error: %d)", msg->status_code);
		g_object_unref(msg);
		g_free(url);

		return ret;
	}

	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-plain-login1.html", data, read);
	g_assert(data != NULL);

	if (!strstr(data, "FRITZ!Box Anmeldung")) {
		ret = TRUE;
	}

	return ret;
}

gboolean fritzbox_get_settings_plain(struct profile *profile)
{
	if (fritzbox_login(profile) == FALSE) {
		return FALSE;
	}

	fritzbox_logout(profile, FALSE);

	return FALSE;
}