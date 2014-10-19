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
#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/logging.h>
#include <libroutermanager/network.h>
#include <libroutermanager/csv.h>
#include <libroutermanager/gstring.h>

#include "fritzbox.h"
#include "csv.h"
#include "firmware-common.h"
#include "firmware-04-74.h"

/**
 * \brief Login function (>= FRITZ!OS 4.74 && < 5.50)
 * \param profile profile information structure
 * \return error code
 */
gboolean fritzbox_login_04_74(struct profile *profile)
{
	SoupMessage *msg;
	gchar *response = NULL;
	gsize read;
	gchar *challenge = NULL;
	gchar *dots = NULL;
	gchar *str = NULL;
	gchar *md5_str = NULL;
	gchar *url;
	const gchar *data;
	gchar *writeaccess;

	if (profile->router_info->session_timer && g_timer_elapsed(profile->router_info->session_timer, NULL) < 9 * 60) {
		return TRUE;
	} else {
		if (!profile->router_info->session_timer) {
			profile->router_info->session_timer = g_timer_new();
			g_timer_start(profile->router_info->session_timer);
		} else {
			g_timer_reset(profile->router_info->session_timer);
		}
	}

	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "getpage", "../html/login_sid.xml",
	                            NULL);
	g_free(url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200 || !msg->response_body->length) {
		g_debug("Received status code: %d", msg->status_code);
		g_debug("Message length: %d", msg->response_body->length);
		g_object_unref(msg);

		g_timer_destroy(profile->router_info->session_timer);
		profile->router_info->session_timer = NULL;
		return FALSE;
	}
	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-04_74-login1.html", data, read);
	g_assert(data != NULL);

	/* <SID>X</SID> */
	profile->router_info->session_id = xml_extract_tag(data, "SID");

	/* <iswriteaccess>X</iswriteaccess> */
	writeaccess = xml_extract_tag(data, "iswriteaccess");
	if (writeaccess == NULL) {
		g_debug("writeaccess is NULL");
		g_object_unref(msg);

		g_timer_destroy(profile->router_info->session_timer);
		profile->router_info->session_timer = NULL;
		return FALSE;
	}

	/* <Challenge>X</Challenge> */
	challenge = xml_extract_tag(data, "Challenge");
	if (challenge == NULL) {
		g_debug("challenge is NULL");
		g_object_unref(msg);

		g_timer_destroy(profile->router_info->session_timer);
		profile->router_info->session_timer = NULL;
		return FALSE;
	}

	g_object_unref(msg);

	if (atoi(writeaccess) == 0) {
		/* Currently not logged in */
		g_debug("Currently not logged in");

		dots = make_dots(router_get_login_password(profile));
		str = g_strconcat(challenge, "-", dots, NULL);
		md5_str = md5(str);

		response = g_strconcat(challenge, "-", md5_str, NULL);

		g_free(md5_str);
		g_free(str);
		g_free(dots);

		url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
		msg = soup_form_request_new(SOUP_METHOD_POST, url,
		                            "login:command/response", response,
		                            "getpage", "../html/login_sid.xml",
		                            NULL);
		g_free(url);

		soup_session_send_message(soup_session_sync, msg);
		if (msg->status_code != 200) {
			g_debug("Received status code: %d", msg->status_code);
			g_object_unref(msg);

			g_timer_destroy(profile->router_info->session_timer);
			profile->router_info->session_timer = NULL;

			return FALSE;
		}
		data = msg->response_body->data;
		read = msg->response_body->length;

		log_save_data("fritzbox-04_74-login2.html", data, read);

		g_free(response);

		/* <iswriteaccess>X</iswriteaccess> */
		writeaccess = xml_extract_tag(data, "iswriteaccess");

		/* <Challenge>X</Challenge> */
		challenge = xml_extract_tag(data, "Challenge");

		if ((atoi(writeaccess) == 0) || strcmp(profile->router_info->session_id, "0000000000000000")) {
			g_debug("Login failure (%d should be non 0, %s should not be 0000000000000000)", atoi(writeaccess), profile->router_info->session_id);

			g_object_unref(msg);

			g_timer_destroy(profile->router_info->session_timer);
			profile->router_info->session_timer = NULL;

			return FALSE;
		}

		g_debug("Login successful");

		g_free(profile->router_info->session_id);
		profile->router_info->session_id = xml_extract_tag(data, "SID");

		g_object_unref(msg);
	} else {
		g_debug("Already logged in");
	}

	g_free(challenge);
	g_free(writeaccess);

	return TRUE;
}

/**
 * \brief Compare strings
 * \param a string a
 * \param b string b
 * \return return value of strcmp
 */
gint number_compare_04_74(gconstpointer a, gconstpointer b)
{
	return strcmp(a, b);
}

/**
 * \brief Extract phone number from fw 04.74
 * \param number_list pointer to number_list
 * \param data incoming page data
 * \param msn_str string we want to lookup
 * \return TRUE on success, otherwise FALSE
 */
gboolean extract_number_04_74(GSList **number_list, const gchar *data, gchar *msn_str)
{
	gchar *fon;

	fon = xml_extract_input_value(data, msn_str);
	if (!EMPTY_STRING(fon) && isdigit(fon[0])) {
		if (!g_slist_find_custom(*number_list, fon, number_compare_04_74)) {
			*number_list = g_slist_prepend(*number_list, fon);
		} else {
			g_free(fon);
		}

		return TRUE;
	}

	g_free(fon);
	return FALSE;
}

/**
 * \brief strndup phone number from fw 04.74
 * \param number_list pointer to number_list
 * \param data incoming page data
 * \param len len of string to copy
 * \return TRUE on success, otherwise FALSE
 */
gboolean copy_number_04_74(GSList **number_list, const gchar *data, gsize len)
{
	gchar *fon;

	fon = g_strndup(data, len);
	if (!EMPTY_STRING(fon) && isdigit(fon[0])) {
		if (!g_slist_find_custom(*number_list, fon, number_compare_04_74)) {
			*number_list = g_slist_prepend(*number_list, fon);
		} else {
			g_free(fon);
		}

		return TRUE;
	}

	g_free(fon);
	return FALSE;
}

/**
 * \brief Read MSNs of data
 * \param profile profile information structure
 * \param data data to parse for MSNs
 */
void fritzbox_extract_numbers_04_74(struct profile *profile, const gchar *data)
{
	gint index;
	gint type = -1;
	gint port;
	GSList *number_list = NULL;
	GSList *list;
	gchar **numbers;
	gint counter = 0;
	gchar *skip = NULL;
	gchar *start;
	gchar *end;

	/* First read old entries */
	skip = strstr(data, "readFonNumbers()");

	if (skip != NULL) {
		/* POTS */
		skip = strstr(skip, "nrs.pots");
		if (skip != NULL) {
			start = strchr(skip, '"');
			end = strchr(start + 1, '"');
			if (end - start - 1 > 0) {
				copy_number_04_74(&number_list, start + 1, end - start - 1);
			}
		} else {
			skip = (gchar*)data;
		}

		/* MSN */
		for (index = 0; index < 10; index++) {
			skip = strstr(skip, "nrs.msn.push");
			if (skip != NULL) {
				start = strchr(skip, '"');
				end = strchr(start + 1, '"');
				if (end - start - 1 > 0) {
					copy_number_04_74(&number_list, start + 1, end - start - 1);
				}
				skip = end;
			}
		}

		/* SIP */
		for (index = 0; index < 19; index++) {
			skip = strstr(skip, "nrs.sip.push");
			if (skip != NULL) {
				start = strchr(skip, '"');
				end = strchr(start + 1, '"');
				if (end - start - 1 > 0) {
					copy_number_04_74(&number_list, start + 1, end - start - 1);
				}
				skip = end;
			}
		}
	}

	/* Now read the new entries */
	/* POTS first! */
	if (extract_number_04_74(&number_list, data, "telcfg:settings/MSN/POTS")) {
		type = 3;
	}

	/* TAM */
	for (index = 0; index < 10; index++) {
		gchar *msn_str = g_strdup_printf("tam:settings/MSN%d", index);

		extract_number_04_74(&number_list, data, msn_str);

		g_free(msn_str);
	}

	/* FAX */
	for (index = 0; index < 10; index++) {
		gchar *msn_str = g_strdup_printf("telcfg:settings/FaxMSN%d", index);

		extract_number_04_74(&number_list, data, msn_str);

		g_free(msn_str);
	}

	/* PortX-MSN */
	for (port = 0; port < 3; port++) {
		for (index = 0; index < 10; index++) {
			gchar *msn_str = g_strdup_printf("telcfg:settings/MSN/Port%d/MSN%d", port, index);

			if (extract_number_04_74(&number_list, data, msn_str)) {
				if (type == -1) {
					type = 0;
				}
			}
			g_free(msn_str);
		}
	}

	/* MSN */
	for (index = 0; index < 10; index++) {
		gchar *msn_str = g_strdup_printf("telcfg:settings/MSN/MSN%d", index);

		if (extract_number_04_74(&number_list, data, msn_str)) {
			if (type == -1) {
				type = 0;
			}
		}
		g_free(msn_str);
	}

	/* SIP */
	for (index = 0; index < 19; index++) {
		gchar *msn_str = g_strdup_printf("telcfg:settings/SIP%d/MSN", index);

		if (extract_number_04_74(&number_list, data, msn_str)) {
			if (type == -1) {
				type = 4;
			}
		}

		g_free(msn_str);
	}

	/* VoipExtensionX/NumberY */
	for (port = 0; port < 10; port++) {
		for (index = 0; index < 10; index++) {
			gchar *msn_str = g_strdup_printf("telcfg:settings/VoipExtension%d/Number%d", port, index);

			if (extract_number_04_74(&number_list, data, msn_str)) {
				if (type == -1) {
					type = 4;
				}
			}
			g_free(msn_str);
		}
	}

	numbers = g_malloc(sizeof(gchar *) * (g_slist_length(number_list) + 1));
	for (list = number_list; list; list = list->next) {
		g_debug("Adding MSN '%s'", call_scramble_number(list->data));
		numbers[counter++] = g_strdup(list->data);
	}
	numbers[counter] = NULL;

	g_settings_set_strv(profile->settings, "numbers", (const gchar * const *)numbers);

	if (type != -1) {
		g_debug("Setting controllers to %d", type);
		g_settings_set_int(profile->settings, "fax-controller", type);
		g_settings_set_int(profile->settings, "phone-controller", type);
	}
}

/**
 * \brief Get settings (std)
 * \param profile profile information structure
 * \return error code
 */
gboolean fritzbox_get_settings_04_74(struct profile *profile)
{
	SoupMessage *msg;
	const gchar *data;
	gint index;
	gsize read;
	gchar *url;
	gchar *volume = NULL;

	if (fritzbox_login(profile) == FALSE) {
		return FALSE;
	}

	gchar *request = g_strconcat("../html/",
	                             profile->router_info->lang,
	                             "/menus/menu2.html", NULL);

	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
	                            "getpage", request,
	                            "var:lang", profile->router_info->lang,
	                            "var:pagename", "fondevices",
	                            "var:menu", "home",
	                            "sid", profile->router_info->session_id,
	                            NULL);
	g_free(url);
	g_free(request);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}
	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-04_74-get-settings-1.html", data, read);
	g_assert(data != NULL);

	fritzbox_extract_numbers_04_74(profile, data);

	for (index = 0; index < NUM_PHONE_PORTS; index++) {
		gchar *value;

		value = xml_extract_input_value(data, fritzbox_phone_ports[index].name);
		if (value != NULL && strlen(value) > 0) {
			g_debug("port %d: '%s'", index, value);
			g_settings_set_string(profile->settings, router_phone_ports[index].name, value);
		}
		g_free(value);
	}
	g_object_unref(msg);

	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
	                            "getpage", "../html/de/menus/menu2.html",
	                            "var:lang", profile->router_info->lang,
	                            "var:pagename", "sipoptionen",
	                            "var:menu", "fon",
	                            "sid", profile->router_info->session_id,
	                            NULL);
	g_free(url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}
	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-04_74-get-settings-2.html", data, read);
	g_assert(data != NULL);

	gchar *value;

	value = xml_extract_list_value(data, "telcfg:settings/Location/LKZ");
	if (value != NULL && strlen(value) > 0) {
		g_debug("lkz: '%s'", value);
	}
	g_settings_set_string(profile->settings, "country-code", value);
	g_free(value);

	value = xml_extract_list_value(data, "telcfg:settings/Location/LKZPrefix");
	if (value != NULL && strlen(value) > 0) {
		g_debug("lkz prefix: '%s'", value);
	}
	g_settings_set_string(profile->settings, "international-call-prefix", value);
	g_free(value);

	value = xml_extract_list_value(data, "telcfg:settings/Location/OKZ");
	if (value != NULL && strlen(value) > 0) {
		g_debug("okz: '%s'", value);
	}
	g_settings_set_string(profile->settings, "area-code", value);
	g_free(value);

	value = xml_extract_list_value(data, "telcfg:settings/Location/OKZPrefix");
	if (value != NULL && strlen(value) > 0) {
		g_debug("okz prefix: '%s'", value);
	}
	g_settings_set_string(profile->settings, "national-call-prefix", value);
	g_free(value);

	g_object_unref(msg);

	/* Extract Fax information */
	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "getpage", "../html/de/menus/menu2.html",
	                            "var:lang", profile->router_info->lang,
	                            "var:pagename", "fon1fxi",
	                            "var:menu", "fon",
	                            "sid", profile->router_info->session_id,
	                            NULL);
	g_free(url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}
	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-04_74-get-settings-fax.html", data, read);
	g_assert(data != NULL);

	gchar *header = xml_extract_input_value(data, "telcfg:settings/FaxKennung");
	g_debug("Fax-Header: '%s'", header);
	g_settings_set_string(profile->settings, "fax-header", header);
	g_free(header);

	gchar *fax_msn = xml_extract_input_value(data, "telcfg:settings/FaxMSN0");
	if (fax_msn) {
		gchar *formated_number = call_format_number(profile, fax_msn, NUMBER_FORMAT_INTERNATIONAL_PLUS);

		g_debug("Fax number: '%s'", call_scramble_number(fax_msn));
		g_settings_set_string(profile->settings, "fax-number", fax_msn);

		g_settings_set_string(profile->settings, "fax-ident", formated_number);
		g_free(formated_number);
	}
	g_free(fax_msn);

	gchar *active = xml_extract_input_value(data, "telcfg:settings/FaxMailActive");

	if (active && ((atoi(&active[0]) == 2) || (atoi(&active[0]) == 3))) {
		volume = xml_extract_input_value(data, "ctlusb:settings/storage-part0");

		if (volume) {
			g_debug("Fax-Storage-Volume: '%s'", volume);
			g_settings_set_string(profile->settings, "fax-volume", volume);
		} else {
			g_settings_set_string(profile->settings, "fax-volume", "");
		}

		g_free(active);
	} else {
		g_settings_set_string(profile->settings, "fax-volume", "");
	}

	g_object_unref(msg);

	/* Extract default dial port */
	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "getpage", "../html/de/menus/menu2.html",
	                            "var:lang", profile->router_info->lang,
	                            "var:pagename", "dial",
	                            "var:menu", "fon",
	                            "sid", profile->router_info->session_id,
	                            NULL);
	g_free(url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}
	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-04_74-get-settings-4.html", data, read);
	g_assert(data != NULL);

	gchar *dialport = xml_extract_input_value(data, "telcfg:settings/DialPort");
	if (dialport) {
		gint port = atoi(dialport);
		gint phone_port = fritzbox_find_phone_port(port);
		gchar tmp[10];
		g_debug("Dial port: %s, phone_port: %d", dialport, phone_port);
		snprintf(tmp, sizeof(tmp), "%d", phone_port);
		g_settings_set_string(profile->settings, "port", tmp);
	}
	g_free(dialport);

	/* Always use tam-stick */
	g_settings_set_int(profile->settings, "tam-stick", 1);

	g_object_unref(msg);

	fritzbox_logout(profile, FALSE);

	return TRUE;
}

/**
 * \brief Journal callback function (parse data and emit "journal-process"/"journal-loaded" signals, logout)
 * \param session soup session
 * \param msg soup message
 * \param user_data poiner to profile structure
 */
void fritzbox_journal_04_74_cb(SoupSession *session, SoupMessage *msg, gpointer user_data)
{
	GSList *journal = NULL;
	struct profile *profile = user_data;

	/* Parse journal */
	journal = csv_parse_fritzbox_journal_data(journal, msg->response_body->data);

	/* Load and add faxbox */
	journal = fritzbox_load_faxbox(journal);

	/* Load and add voicebox */
	journal = fritzbox_load_voicebox(journal);

	router_process_journal(journal);

	/* Logout */
	fritzbox_logout(profile, FALSE);
}

/**
 * \brief Load journal function for FRITZ!OS >= 4.74 && < 5.50
 * \param profile profile info structure
 * \param data_ptr data pointer to optional store journal to
 * \return error code
 */
gboolean fritzbox_load_journal_04_74(struct profile *profile, gchar **data_ptr)
{
	SoupMessage *msg;
	gchar *url;

	/* Login to box */
	if (fritzbox_login(profile) == FALSE) {
		g_debug("Login failed");
		return FALSE;
	}

	/* Create POST request */
	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "getpage", "../html/de/menus/menu2.html",
	                            "var:lang", profile->router_info->lang,
	                            "var:pagename", "foncalls",
	                            "var:menu", "fon",
	                            "sid", profile->router_info->session_id,
	                            NULL);
	g_free(url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}
	g_object_unref(msg);

	/* Create POST request */
	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "getpage", "../html/de/FRITZ!Box_Anrufliste.csv",
	                            "sid", profile->router_info->session_id,
	                            NULL);
	g_free(url);

	/* Queue message to session */
	soup_session_queue_message(soup_session_async, msg, fritzbox_journal_04_74_cb, profile);

	return TRUE;
}

/**
 * \brief Clear journal
 * \param profile profile pointer
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_clear_journal_04_74(struct profile *profile)
{
	SoupMessage *msg;
	gchar *url;

	/* Login to box */
	if (fritzbox_login(profile) == FALSE) {
		return FALSE;
	}

	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "sid", profile->router_info->session_id,
	                            "getpage", "../html/de/menus/menu2.html",
	                            "var:pagename", "foncalls",
	                            "var:menu", "fon",
	                            "telcfg:settings/ClearJournal", "",
	                            NULL);
	g_free(url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}

	g_debug("Done");

	g_object_unref(msg);

	fritzbox_logout(profile, FALSE);

	return TRUE;
}
