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
#include "firmware-05-50.h"

/**
 * \brief Login function (>= FRITZ!OS 5.50)
 * \param profile profile information structure
 * \return error code
 */
gboolean fritzbox_login_05_50(struct profile *profile)
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

	url = g_strdup_printf("http://%s/login_sid.lua", router_get_host(profile));
	msg = soup_message_new(SOUP_METHOD_GET, url);
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

	log_save_data("fritzbox-05_50-login_1.html", data, read);
	g_assert(data != NULL);

	/* <SID>session_id</SID> */
	profile->router_info->session_id = xml_extract_tag(data, "SID");

	if (!strcmp(profile->router_info->session_id, "0000000000000000")) {
		gchar *user = router_get_login_user(profile);
		gchar *password = router_get_login_password(profile);

		challenge = xml_extract_tag(data, "Challenge");
		g_object_unref(msg);

		dots = make_dots(password);
		g_free(password);
		str = g_strconcat(challenge, "-", dots, NULL);
		md5_str = md5(str);

		response = g_strconcat(challenge, "-", md5_str, NULL);

		g_free(md5_str);
		g_free(str);
		g_free(dots);
		g_free(challenge);

		url = g_strdup_printf("http://%s/login_sid.lua", router_get_host(profile));
		msg = soup_form_request_new(SOUP_METHOD_POST, url,
		                            "username", user,
		                            "response", response,
		                            NULL);
		g_free(url);

		soup_session_send_message(soup_session_sync, msg);
		g_free(user);
		if (msg->status_code != 200) {
			g_debug("Received status code: %d", msg->status_code);
			g_object_unref(msg);

			g_timer_destroy(profile->router_info->session_timer);
			profile->router_info->session_timer = NULL;

			return FALSE;
		}
		data = msg->response_body->data;
		read = msg->response_body->length;

		log_save_data("fritzbox-05_50-login_2.html", data, read);

		g_free(response);

		profile->router_info->session_id = xml_extract_tag(data, "SID");
	}

	g_object_unref(msg);

	return !!strcmp(profile->router_info->session_id, "0000000000000000");
}

/**
 * \brief Number compare function
 * \param a string a
 * \param b string b
 * \return return value of strcmp
 */
gint number_compare(gconstpointer a, gconstpointer b)
{
	return strcmp(a, b);
}

/**
 * \brief Extract number from fw 05.50
 * \param number_list pointer to number list
 * \param data incoming page data
 * \param msn_str msn string to lookup
 * \return TRUE on success, otherwise FALSE
 */
gboolean extract_number_05_50(GSList **number_list, const gchar *data, gchar *msn_str)
{
	gchar *fon;

	fon = xml_extract_list_value(data, msn_str);
	if (!EMPTY_STRING(fon) && isdigit(fon[0])) {
		if (!g_slist_find_custom(*number_list, fon, number_compare)) {
			if (strlen(fon) > 2) {
				*number_list = g_slist_prepend(*number_list, fon);
			}
		} else {
			g_free(fon);
		}

		return TRUE;
	}

	g_free(fon);
	return FALSE;
}

/**
 * \brief Extract phone numbers from webpage data
 * \param profile profile structure
 * \param data webpage data
 */
static void fritzbox_detect_controller_05_50(struct profile *profile, const gchar *data)
{
	gint index;
	gint type = -1;
	gint port;
	GSList *number_list = NULL;

	/* POTS first! */
	if (extract_number_05_50(&number_list, data, "telcfg:settings/MSN/POTS")) {
		type = 3;
		goto set;
	}

	/* PortX-MSN */
	for (port = 0; port < 3; port++) {
		for (index = 0; index < 10; index++) {
			gchar *msn_str = g_strdup_printf("telcfg:settings/MSN/Port%d/MSN%d", port, index);

			if (extract_number_05_50(&number_list, data, msn_str)) {
				if (type == -1) {
					type = 0;
					g_free(msn_str);
					goto set;
				}
			}
			g_free(msn_str);
		}
	}

	/* NTHotDialList */
	for (index = 0; index < 10; index++) {
		gchar *msn_str = g_strdup_printf("telcfg:settings/NTHotDialList/Number%d", index);

		if (!msn_str) {
			continue;
		}

		if (extract_number_05_50(&number_list, data, msn_str)) {
			if (type == -1) {
				type = 0;
				g_free(msn_str);
				goto set;
			}
		}
		g_free(msn_str);
	}

	/* SIP */
	for (index = 0; index < 19; index++) {
		gchar *msn_str = g_strdup_printf("telcfg:settings/SIP%d/MSN", index);

		if (extract_number_05_50(&number_list, data, msn_str)) {
			if (type == -1) {
				type = 4;
				g_free(msn_str);
				goto set;
			}
		}

		g_free(msn_str);
	}

	return;

set:
	g_debug("Setting controllers to %d", type);
	g_settings_set_int(profile->settings, "fax-controller", type);
	g_settings_set_int(profile->settings, "phone-controller", type);
}

/**
 * \brief Extract DECT numbers of fw 05.50
 * \param profile profile pointer
 * \param data incoming page data
 */
static void fritzbox_extract_dect_05_50(struct profile *profile, const gchar *data)
{
	const gchar *start = data;
	gchar *pos;
	gchar *end;
	gint size;
	gchar *fon;
	gint count = 1;
	gchar name_dect[11];

	do {
		pos = g_strstr_len(start, -1, "<td>DECT</td>");
		if (!pos) {
			break;
		}

		/* Set new start position */
		start = pos + 1;

		/* Extract previous <td>XXXX</td>, this is the phone name */
		end = g_strrstr_len(data, pos - data - 1, "\">");
		if (!end) {
			continue;
		}

		size = pos - end - 7;
		if (size <= 0) {
			continue;
		}

		memset(name_dect, 0, sizeof(name_dect));
		g_snprintf(name_dect, sizeof(name_dect), "name-dect%d", count);
		fon = g_slice_alloc0(size);
		g_strlcpy(fon, end + 2, size);
		g_debug("fon: '%s'", fon);
		g_settings_set_string(profile->settings, name_dect, fon);
		g_slice_free1(size, fon);
		count++;
	} while (count < 7);
}

gboolean fritzbox_get_fax_information_05_50(struct profile *profile)
{
	SoupMessage *msg;
	const gchar *data;
	gsize read;
	gchar *url;
	gchar *scramble;

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

	log_save_data("fritzbox-05_50-get-settings-fax.html", data, read);

	g_assert(data != NULL);

	gchar *header = xml_extract_input_value(data, "telcfg:settings/FaxKennung");
	if (header) {
		scramble = call_scramble_number(header);
		g_debug("Fax-Header: '%s'", scramble);
		g_free(scramble);
		g_settings_set_string(profile->settings, "fax-header", header);
		g_free(header);
	}

	gchar *fax_msn = xml_extract_input_value(data, "telcfg:settings/FaxMSN0");
	if (fax_msn) {
		if (!strcmp(fax_msn, "POTS")) {
			gchar **numbers = g_settings_get_strv(profile->settings, "numbers");
			g_free(fax_msn);
			fax_msn = g_strdup(numbers[0]);
		}
		gchar *formated_number;

		formated_number = call_format_number(profile, fax_msn, NUMBER_FORMAT_INTERNATIONAL_PLUS);

		scramble = call_scramble_number(fax_msn);
		g_debug("Fax number: '%s'", scramble);
		g_free(scramble);

		g_settings_set_string(profile->settings, "fax-number", fax_msn);

		g_settings_set_string(profile->settings, "fax-ident", formated_number);
		g_free(formated_number);
	}
	g_free(fax_msn);

	g_settings_set_string(profile->settings, "fax-volume", "");
	gchar *active = xml_extract_input_value(data, "telcfg:settings/FaxMailActive");
	if (active) {
		gint fax_mail_active = atoi(&active[0]);

		if ((fax_mail_active == 2 || fax_mail_active == 3)) {
			gchar *volume = xml_extract_input_value(data, "ctlusb:settings/storage-part0");

			if (volume) {
				g_debug("Fax-Storage-Volume: '%s'", volume);
				g_settings_set_string(profile->settings, "fax-volume", volume);
			} else {
				g_settings_set_string(profile->settings, "fax-volume", "");
			}

			g_free(active);
		}
	}

	g_object_unref(msg);

	return TRUE;
}

gboolean fritzbox_get_fax_information_06_00(struct profile *profile)
{
	SoupMessage *msg;
	const gchar *data;
	gsize read;
	gchar *url;
	gchar *scramble;

	url = g_strdup_printf("http://%s/fon_devices/fax_send.lua", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
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

	log_save_data("fritzbox-06_00-get-settings-fax.html", data, read);

	g_assert(data != NULL);

	gchar *header = xml_extract_list_value(data, "telcfg:settings/FaxKennung");
	if (header) {
		scramble = call_scramble_number(header);
		g_debug("Fax-Header: '%s'", scramble);
		g_free(scramble);
		g_settings_set_string(profile->settings, "fax-header", header);
		g_free(header);
	}

	gchar *fax_msn = xml_extract_list_value(data, "telcfg:settings/FaxMSN0");
	if (fax_msn) {
		if (!strcmp(fax_msn, "POTS")) {
			gchar **numbers = g_settings_get_strv(profile->settings, "numbers");
			g_free(fax_msn);
			fax_msn = g_strdup(numbers[0]);
		}
		gchar *formated_number;

		formated_number = call_format_number(profile, fax_msn, NUMBER_FORMAT_INTERNATIONAL_PLUS);

		scramble = call_scramble_number(fax_msn);
		g_debug("Fax number: '%s'", scramble);
		g_free(scramble);

		g_settings_set_string(profile->settings, "fax-number", fax_msn);

		g_settings_set_string(profile->settings, "fax-ident", formated_number);
		g_free(formated_number);
	}
	g_free(fax_msn);

	g_settings_set_string(profile->settings, "fax-volume", "");
	gchar *mail_active = xml_extract_list_value(data, "telcfg:settings/FaxMailActive");
	if (mail_active) {
		gint fax_mail_active = atoi(&mail_active[0]);

		if (fax_mail_active == 3) {
			gchar *volume;
			g_object_unref(msg);


			url = g_strdup_printf("http://%s/usb/show_usb_devices.lua", router_get_host(profile));
			msg = soup_form_request_new(SOUP_METHOD_GET, url,
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

			log_save_data("fritzbox-06_00-show-usb-devices.html", data, read);

			g_assert(data != NULL);

			volume = xml_extract_list_value(data, "name");

			if (volume) {
				g_debug("Fax-Storage-Volume: '%s'", volume);
				g_settings_set_string(profile->settings, "fax-volume", volume);
			}

			g_free(mail_active);
		}
	}

	g_object_unref(msg);

	return TRUE;
}

/**
 * \brief Get settings via lua-scripts (phone numbers/names, default controller, tam setting, fax volume/settings, prefixes, default dial port)
 * \param profile profile information structure
 * \return error code
 */
gboolean fritzbox_get_settings_05_50(struct profile *profile)
{
	SoupMessage *msg;
	const gchar *data;
	gint index;
	gsize read;
	gchar *url;

	g_debug("Get settings");

	/* Login */
	if (fritzbox_login(profile) == FALSE) {
		return FALSE;
	}

	/* Extract phone numbers */
	url = g_strdup_printf("http://%s/fon_num/fon_num_list.lua", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
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

	log_save_data("fritzbox-05_50-get-settings-0.html", data, read);
	g_assert(data != NULL);

	gchar **numbers = xml_extract_tags(data, "td title=\"[^\"]*\"", "td");

	if (g_strv_length(numbers)) {
		gchar **profile_numbers = strv_remove_duplicates(numbers);
		gint idx;

		if (g_strv_length(profile_numbers)) {
			for (idx = 0; idx < g_strv_length(profile_numbers); idx++) {
				gchar *scramble = call_scramble_number(profile_numbers[idx]);
				g_debug("Adding MSN '%s'", scramble);
				g_free(scramble);
			}
			g_settings_set_strv(profile->settings, "numbers", (const gchar * const *)profile_numbers);
		}
		g_strfreev(numbers);

	}
	g_object_unref(msg);

	/* Extract phone names, default controller */
	url = g_strdup_printf("http://%s/fon_devices/fondevices_list.lua", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
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

	log_save_data("fritzbox-05_50-get-settings-1.html", data, read);
	g_assert(data != NULL);

	/* Try to detect controller */
	fritzbox_detect_controller_05_50(profile, data);

	/* Extract phone names */
	for (index = 0; index < PORT_MAX; index++) {
		gchar *value;

		value = xml_extract_list_value(data, fritzbox_phone_ports[index].name);
		if (value) {
			if (!EMPTY_STRING(value)) {
				g_debug("Port %d: '%s'", index, value);
			}
			g_settings_set_string(profile->settings, router_phone_ports[index].name, value);
			g_free(value);
		}
	}

	/* FRITZ!OS 5.50 has broken the layout of DECT, therefore we must scan again for DECT */
	fritzbox_extract_dect_05_50(profile, data);

	/* Check if TAM is using USB-Stick */
	gchar *stick = xml_extract_input_value(data, "tam:settings/UseStick");
	if (stick && atoi(&stick[0])) {
		g_settings_set_int(profile->settings, "tam-stick", atoi(stick));
	} else {
		g_settings_set_int(profile->settings, "tam-stick", 0);
	}
	g_free(stick);

	g_object_unref(msg);

	/* Extract city/country/area prefix */
	url = g_strdup_printf("http://%s/fon_num/sip_option.lua", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
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

	log_save_data("fritzbox-05_50-get-settings-2.html", data, read);
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
	if (FIRMWARE_IS(6, 0)) {
		fritzbox_get_fax_information_06_00(profile);
	} else {
		fritzbox_get_fax_information_05_50(profile);
	}

	/* Extract default dial port */
	url = g_strdup_printf("http://%s/fon_num/dial_foncalls.lua", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
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

	log_save_data("fritzbox-05_50-get-settings-3.html", data, read);
	g_assert(data != NULL);

	gchar *dialport = xml_extract_list_value(data, "telcfg:settings/DialPort");
	if (dialport) {
		gint port = atoi(dialport);
		gint phone_port = fritzbox_find_phone_port(port);
		g_debug("Dial port: %s, phone_port: %d", dialport, phone_port);
		router_set_phone_port(profile, phone_port);
	}
	g_free(dialport);

	g_object_unref(msg);

	/* The end - exit */
	fritzbox_logout(profile, FALSE);

	return TRUE;
}

/**
 * \brief Journal callback function (parse data and emit "journal-process"/"journal-loaded" signals, logout)
 * \param session soup session
 * \param msg soup message
 * \param user_data poiner to profile structure
 */
void fritzbox_journal_05_50_cb(SoupSession *session, SoupMessage *msg, gpointer user_data)
{
	GSList *journal = NULL;
	struct profile *profile = user_data;

	/* Parse online journal */
	journal = csv_parse_fritzbox_journal_data(journal, msg->response_body->data);

	/* Load and add faxbox */
	journal = fritzbox_load_faxbox(journal);

	/* Load and add voicebox */
	journal = fritzbox_load_voicebox(journal);

	/* Load fax reports */
	journal = router_load_fax_reports(profile, journal);

	/* Load voice records */
	journal = router_load_voice_records(profile, journal);

	/* Process journal list */
	router_process_journal(journal);

	/* Logout */
	fritzbox_logout(profile, FALSE);
}

/**
 * \brief Load journal function for FRITZ!OS >= 5.50
 * \param profile router info structure
 * \param data_ptr data pointer to optional store journal to
 * \return error code
 */
gboolean fritzbox_load_journal_05_50(struct profile *profile, gchar **data_ptr)
{
	SoupMessage *msg;

	/* Login to box */
	if (fritzbox_login(profile) == FALSE) {
		g_debug("Login failed");
		return FALSE;
	}

	/* Create GET request */
	gchar *url = g_strdup_printf("http://%s/fon_num/foncalls_list.lua", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
	                            "sid", profile->router_info->session_id,
	                            "csv", "",
	                            NULL);
	g_free(url);

	/* Queue message to session */
	soup_session_queue_message(soup_session_async, msg, fritzbox_journal_05_50_cb, profile);

	return TRUE;
}

/**
 * \brief Clear journal
 * \param profile profile pointer
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_clear_journal_05_50(struct profile *profile)
{
	SoupMessage *msg;
	gchar *url;

	/* Login to box */
	if (fritzbox_login(profile) == FALSE) {
		return FALSE;
	}

	url = g_strdup_printf("http://%s/fon_num/foncalls_list.lua", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "sid", profile->router_info->session_id,
	                            "usejournal", "on",
	                            "clear", "",
	                            "callstab", "all",
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
