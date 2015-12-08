/**
 * The libroutermanager project
 * Copyright (c) 2012-2015 Jan-Michael Brummer
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
#include "firmware-06-35.h"

gchar *xml_extract_tag_value(gchar *data, gchar *tag)
{
	gchar *pos = g_strstr_len(data, -1, tag);
	gchar *value;
	gchar *end;
	gchar *ret = NULL;
	gsize len;

	if (!pos) {
		return ret;
	}

	value = g_strstr_len(pos, -1, "value=\"");
	if (!value) {
		return ret;
	}

	value += 7;

	end = g_strstr_len(value, -1, "\"");
	if (!end) {
		return ret;
	}

	len = end - value;
	if (len > 0) {
		ret = g_malloc0(len);
		memcpy(ret, value, len);
	}

	return ret;
}

/**
 * \brief Extract phone numbers from webpage data
 * \param profile profile structure
 * \param data webpage data
 */
static void fritzbox_detect_controller_06_35(struct profile *profile, const gchar *data)
{
	gint index;
	gint type = 4;

	for (index = 0; index < PORT_MAX; index++) {
		if (!EMPTY_STRING(router_phone_ports[index].name)) {
			if (index < PORT_ISDNALL) {
				/* Analog */
				type = 3;
			} else if (index < PORT_IP1) {
				/* ISDN */
				type = 0;
			} else {
				/* SIP */
				type = 4;
			}
		}
	}

	g_debug("Setting controllers to %d", type);
	g_settings_set_int(profile->settings, "fax-controller", type);
	g_settings_set_int(profile->settings, "phone-controller", type);
}

gboolean fritzbox_get_fax_information_06_35(struct profile *profile)
{
	SoupMessage *msg;
	const gchar *data;
	gsize read;
	gchar *url;

	url = g_strdup_printf("http://%s/fon_devices/fax_option.lua", router_get_host(profile));
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

	log_save_data("fritzbox-06_35-get-settings-fax-option.html", data, read);

	g_assert(data != NULL);

	/* name="headline" value="...name..." > */
	gchar *regex_str = g_strdup_printf("<input.+name=\"headline\" value=\"(?P<name>(\\w|\\s|-)+)\" >");
	GRegex *regex = NULL;
	GError *error = NULL;
	GMatchInfo *match_info;

	regex = g_regex_new(regex_str, 0, 0, &error);
	g_assert(regex != NULL);

	g_regex_match(regex, data, 0, &match_info);

	while (match_info && g_match_info_matches(match_info)) {
		gchar *name = g_match_info_fetch_named(match_info, "name");

		if (name) {
			gchar *scramble = call_scramble_number(name);
			g_debug("Fax-Header: '%s'", scramble);
			g_free(scramble);
			g_settings_set_string(profile->settings, "fax-header", name);
			break;
		}

		if (g_match_info_next(match_info, NULL) == FALSE) {
			break;
		}
	}

	g_match_info_free(match_info);
	g_free(regex_str);

	/* input type="checkbox" id="uiFaxSaveUsb" name="fax_save_usb"  checked  disabled> */
	regex_str = g_strdup_printf("<input type=\"checkbox\" id=\"uiFaxSaveUsb\" name=\"fax_save_usb\"(?P<checked>(\\w|\\s)+)disabled>");
	error = NULL;
	gboolean store = FALSE;

	regex = g_regex_new(regex_str, 0, 0, &error);
	g_assert(regex != NULL);

	g_regex_match(regex, data, 0, &match_info);

	while (match_info && g_match_info_matches(match_info)) {
		gchar *checked = g_match_info_fetch_named(match_info, "checked");

		if (checked && strstr(checked, "checked")) {
			store = TRUE;
			break;
		}

		if (g_match_info_next(match_info, NULL) == FALSE) {
			break;
		}
	}

	g_match_info_free(match_info);
	g_free(regex_str);

	g_settings_set_int(profile->settings, "tam-stick", store);
	g_settings_set_string(profile->settings, "fax-volume", "");

	g_object_unref(msg);

	if (store) {
		url = g_strdup_printf("http://%s/storage/settings.lua", router_get_host(profile));
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

		log_save_data("fritzbox-06_35-get-settings-fax-usb.html", data, read);

		/* <td id="/var/media/ftp/PatriotMemory-01"> */

		regex_str = g_strdup_printf("<td id=\"/var/media/ftp/(?P<volume>(\\w|\\s|\\d|-)+)\"");
		error = NULL;

		regex = g_regex_new(regex_str, 0, 0, &error);
		g_assert(regex != NULL);

		g_regex_match(regex, data, 0, &match_info);

		while (match_info && g_match_info_matches(match_info)) {
			gchar *volume = g_match_info_fetch_named(match_info, "volume");

			if (volume) {
				g_debug("Fax-Storage-Volume: '%s'", volume);
				g_settings_set_string(profile->settings, "fax-volume", volume);
				break;
			}

			if (g_match_info_next(match_info, NULL) == FALSE) {
				break;
			}
		}

		g_match_info_free(match_info);
		g_free(regex_str);

		g_object_unref(msg);
	}

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

	log_save_data("fritzbox-06_35-get-settings-fax-send.html", data, read);

	g_assert(data != NULL);

	/* <option value="num">num */
	regex_str = g_strdup_printf("<option value=\"(?P<msn>\\d+)\">");
	error = NULL;

	regex = g_regex_new(regex_str, 0, 0, &error);
	g_assert(regex != NULL);

	g_regex_match(regex, data, 0, &match_info);

	while (match_info && g_match_info_matches(match_info)) {
		gchar *msn = g_match_info_fetch_named(match_info, "msn");

		if (msn) {
			gchar *formated_number;
			gchar *scramble;

			formated_number = call_format_number(profile, msn, NUMBER_FORMAT_INTERNATIONAL_PLUS);

			scramble = call_scramble_number(msn);
			g_debug("Fax number: '%s'", scramble);
			g_free(scramble);

			g_settings_set_string(profile->settings, "fax-number", msn);

			g_settings_set_string(profile->settings, "fax-ident", formated_number);
			g_free(formated_number);
			break;
		}

		if (g_match_info_next(match_info, NULL) == FALSE) {
			break;
		}
	}

	g_match_info_free(match_info);
	g_free(regex_str);

	g_object_unref(msg);

	return TRUE;
}

void fritzbox_extract_phone_names_06_35(struct profile *profile, const gchar *data, gsize read)
{
	gchar *regex_str = g_strdup_printf("<option(\\w|\\s)+value=\"(?P<port>\\d{1,3})\">(?P<name>(\\w|\\s|-)+)</option>");
	GRegex *regex = NULL;
	GError *error = NULL;
	GMatchInfo *match_info;

	regex = g_regex_new(regex_str, 0, 0, &error);
	g_assert(regex != NULL);

	g_regex_match(regex, data, 0, &match_info);

	while (match_info && g_match_info_matches(match_info)) {
		gchar *port = g_match_info_fetch_named(match_info, "port");
		gchar *name = g_match_info_fetch_named(match_info, "name");

		if (port && name) {
			gint val = atoi(port);
			gint index;

			for (index = 0; index < PORT_MAX; index++) {
				if (fritzbox_phone_ports[index].number == val) {
					g_debug("Port %d: '%s'", index, name);
					g_settings_set_string(profile->settings, router_phone_ports[index].name, name);
				}
			}
		}

		if (g_match_info_next(match_info, NULL) == FALSE) {
			break;
		}
	}

	g_match_info_free(match_info);
	g_free(regex_str);
}

/**
 * \brief Get settings via lua-scripts (phone numbers/names, default controller, tam setting, fax volume/settings, prefixes, default dial port)
 * \param profile profile information structure
 * \return error code
 */
gboolean fritzbox_get_settings_06_35(struct profile *profile)
{
	SoupMessage *msg;
	const gchar *data;
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

	log_save_data("fritzbox-06_35-get-settings-0.html", data, read);
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

	log_save_data("fritzbox-06_35-get-settings-1.html", data, read);
	g_assert(data != NULL);

	fritzbox_extract_phone_names_06_35(profile, data, read);

	/* Try to detect controller */
	fritzbox_detect_controller_06_35(profile, data);

	gchar *dialport = xml_extract_tag_value((gchar*)data, "option selected");
	if (dialport) {
		gint port = atoi(dialport);
		gint phone_port = fritzbox_find_phone_port(port);
		g_debug("Dial port: %s, phone_port: %d", dialport, phone_port);
		router_set_phone_port(profile, phone_port);
	}
	g_free(dialport);

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

	log_save_data("fritzbox-06_35-get-settings-2.html", data, read);
	g_assert(data != NULL);

	gchar *value;

	value = xml_extract_input_value_r(data, "lkz");
	if (value != NULL && strlen(value) > 0) {
		g_debug("lkz: '%s'", value);
	}
	g_settings_set_string(profile->settings, "country-code", value);
	g_free(value);

	value = xml_extract_input_value_r(data, "lkz_prefix");
	if (value != NULL && strlen(value) > 0) {
		g_debug("lkz prefix: '%s'", value);
	}
	g_settings_set_string(profile->settings, "international-call-prefix", value);
	g_free(value);

	value = xml_extract_input_value_r(data, "okz");
	if (value != NULL && strlen(value) > 0) {
		g_debug("okz: '%s'", value);
	}
	g_settings_set_string(profile->settings, "area-code", value);
	g_free(value);

	value = xml_extract_input_value_r(data, "okz_prefix");
	if (value != NULL && strlen(value) > 0) {
		g_debug("okz prefix: '%s'", value);
	}
	g_settings_set_string(profile->settings, "national-call-prefix", value);
	g_free(value);

	g_object_unref(msg);

	/* Extract Fax information */
	fritzbox_get_fax_information_06_35(profile);

	/* The end - exit */
	fritzbox_logout(profile, FALSE);

	return TRUE;
}

/**
 * \brief Dial number using new ui format
 * \param profile profile information structure
 * \param port dial port
 * \param number remote number
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_dial_number_06_35(struct profile *profile, gint port, const gchar *number)
{
	SoupMessage *msg;
	gchar *port_str;
	gchar *url;
	gchar *scramble;

	/* Login to box */
	if (fritzbox_login(profile) == FALSE) {
		return FALSE;
	}

	/* Create GET message */
	port_str = g_strdup_printf("%d", fritzbox_get_dialport(port));

	scramble = call_scramble_number(number);
	g_debug("Call number '%s' on port %s...", scramble, port_str);
	g_free(scramble);

	url = g_strdup_printf("http://%s/fon_num/foncalls_list.lua", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
	                            "sid", profile->router_info->session_id,
	                            "dial", number,
	                            NULL);
	g_free(url);
	g_free(port_str);

	/* Send message */
	soup_session_send_message(soup_session_async, msg);
	fritzbox_logout(profile, FALSE);

	return TRUE;
}

/**
 * \brief Hangup call using new ui format
 * \param profile profile information structure
 * \param port dial port
 * \param number remote number
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_hangup_06_35(struct profile *profile, gint port, const gchar *number)
{
	SoupMessage *msg;
	gchar *port_str;
	gchar *url;
	gchar *scramble;

	/* Login to box */
	if (fritzbox_login(profile) == FALSE) {
		return FALSE;
	}

	/* Create GET message */
	port_str = g_strdup_printf("%d", fritzbox_get_dialport(port));

	scramble = call_scramble_number(number);
	g_debug("Hangup call '%s' on port %s...", scramble, port_str);
	g_free(scramble);

	url = g_strdup_printf("http://%s/fon_num/foncalls_list.lua", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
	                            "sid", profile->router_info->session_id,
	                            "hangup", "",
	                            NULL);
	g_free(url);
	g_free(port_str);

	/* Send message */
	soup_session_send_message(soup_session_async, msg);
	fritzbox_logout(profile, FALSE);

	return TRUE;
}

