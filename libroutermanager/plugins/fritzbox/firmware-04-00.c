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
#include "firmware-04-00.h"

/**
 * \brief Try to detect a FRITZ!Box router by simply access the start page
 * \param router_info router information structure
 * \return TRUE if FRITZ!Box is detected, otherwise FALSE on error
 */
gboolean fritzbox_present_04_00(struct router_info *router_info)
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
		g_warning("Could not load 04_00 present page (Error: %d)", msg->status_code);
		g_object_unref(msg);
		g_free(url);

		return ret;
	}

	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-04_00-present.html", data, read);
	g_assert(data != NULL);

	if (g_strcasestr(data, "fritz!box")) {
		ret = TRUE;

		router_info->name = g_strdup("FRITZ!Box");
		router_info->version = g_strdup(">= x.4.0");
		router_info->lang = g_strdup("de");
		router_info->annex = g_strdup("");

		/* This is a fritz!box router, but which version.... */
		router_info->box_id = 0;
		router_info->maj_ver_id = 4;
		router_info->min_ver_id = 0;
		router_info->serial = g_strdup("Type Login");
	} else {
		ret = FALSE;
	}

	g_object_unref(msg);
	g_free(url);

	return ret;
}

gboolean fritzbox_login_04_00(struct profile *profile)
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
		g_warning("Could not load 04_00 login page (Error: %d)", msg->status_code);
		g_object_unref(msg);
		g_free(url);

		return ret;
	}

	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-04_00-login1.html", data, read);
	g_assert(data != NULL);

	if (!strstr(data, "FRITZ!Box Anmeldung")) {
		ret = TRUE;
	}

	return ret;
}


/**
 * \brief Dial number using ClickToDial
 * \param profile profile information structure
 * \param port dial port
 * \param number remote number
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_dial_number_04_00(struct profile *profile, gint port, const gchar *number)
{
	SoupMessage *msg;
	gchar *port_str;
	gchar *scramble;
	gboolean ret = FALSE;

	/* Login to box */
	if (fritzbox_login(profile) == FALSE) {
		return FALSE;
	}

	/* Create POST message */
	gchar *url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	port_str = g_strdup_printf("%d", fritzbox_get_dialport(port));

	scramble = call_scramble_number(number);
	g_debug("Call number '%s' on port %s...", scramble, port_str);
	g_free(scramble);

	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "telcfg:settings/UseClickToDial", "1",
	                            "telcfg:settings/DialPort", port_str,
	                            "telcfg:command/Dial", number,
	                            "sid", profile->router_info->session_id,
	                            NULL);
	g_free(port_str);
	g_free(url);

	/* Send message */
	soup_session_send_message(soup_session_async, msg);
	if (msg->status_code == 200) {
		ret = TRUE;
	}
	fritzbox_logout(profile, FALSE);

	return ret;
}


/**
 * \brief Hangup call
 * \param profile profile information structure
 * \param port dial port
 * \param number remote number
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_hangup_04_00(struct profile *profile, gint port, const gchar *number)
{
	SoupMessage *msg;
	gchar *port_str;

	/* Login to box */
	if (fritzbox_login(profile) == FALSE) {
		return FALSE;
	}

	/* Create POST message */
	gchar *url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	port_str = g_strdup_printf("%d", fritzbox_get_dialport(port));

	g_debug("Hangup on port %s...", port_str);

	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "telcfg:settings/UseClickToDial", "1",
	                            "telcfg:settings/DialPort", port_str,
	                            "telcfg:command/Hangup", number,
	                            "sid", profile->router_info->session_id,
	                            NULL);
	g_free(port_str);
	g_free(url);

	/* Send message */
	soup_session_send_message(soup_session_async, msg);
	fritzbox_logout(profile, FALSE);

	return TRUE;
}

