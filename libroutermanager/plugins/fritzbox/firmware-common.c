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
#include <ctype.h>

#include <glib.h>

#include <libsoup/soup.h>

#include <libroutermanager/logging.h>
#include <libroutermanager/router.h>
#include <libroutermanager/network.h>
#include <libroutermanager/call.h>
#include <libroutermanager/ftp.h>

#include "fritzbox.h"
#include "firmware-common.h"

/** phone port names */
struct phone_port fritzbox_phone_ports[NUM_PHONE_PORTS] = {
	/* Analog */
	{"telcfg:settings/MSN/Port0/Name", PORT_ANALOG1, 1},
	{"telcfg:settings/MSN/Port1/Name", PORT_ANALOG2, 2},
	{"telcfg:settings/MSN/Port2/Name", PORT_ANALOG3, 3},
	/* ISDN */
	{"telcfg:settings/NTHotDialList/Name1", PORT_ISDN1, 51},
	{"telcfg:settings/NTHotDialList/Name2", PORT_ISDN2, 52},
	{"telcfg:settings/NTHotDialList/Name3", PORT_ISDN3, 53},
	{"telcfg:settings/NTHotDialList/Name4", PORT_ISDN4, 54},
	{"telcfg:settings/NTHotDialList/Name5", PORT_ISDN5, 55},
	{"telcfg:settings/NTHotDialList/Name6", PORT_ISDN6, 56},
	{"telcfg:settings/NTHotDialList/Name7", PORT_ISDN7, 57},
	{"telcfg:settings/NTHotDialList/Name8", PORT_ISDN8, 58},
	/* DECT */
	{"telcfg:settings/Foncontrol/User1/Name", PORT_DECT1, 60},
	{"telcfg:settings/Foncontrol/User2/Name", PORT_DECT2, 61},
	{"telcfg:settings/Foncontrol/User3/Name", PORT_DECT3, 62},
	{"telcfg:settings/Foncontrol/User4/Name", PORT_DECT4, 63},
	{"telcfg:settings/Foncontrol/User5/Name", PORT_DECT5, 64},
	{"telcfg:settings/Foncontrol/User6/Name", PORT_DECT6, 65},
	/* IP-Phone */
	{"telcfg:settings/VoipExtension0/Name", PORT_IP1, 620},
	{"telcfg:settings/VoipExtension1/Name", PORT_IP2, 621},
	{"telcfg:settings/VoipExtension2/Name", PORT_IP3, 622},
	{"telcfg:settings/VoipExtension3/Name", PORT_IP4, 623},
	{"telcfg:settings/VoipExtension4/Name", PORT_IP5, 624},
	{"telcfg:settings/VoipExtension5/Name", PORT_IP6, 625},
	{"telcfg:settings/VoipExtension6/Name", PORT_IP7, 626},
	{"telcfg:settings/VoipExtension7/Name", PORT_IP8, 627},
	{"telcfg:settings/VoipExtension8/Name", PORT_IP9, 628},
	{"telcfg:settings/VoipExtension9/Name", PORT_IP10, 629},
};

static struct voice_box voice_boxes[5];

/**
 * \brief Extract XML Tag: <TAG>VALUE</TAG>
 * \param data data to parse
 * \param tag tag to extract
 * \return tag value
 */
gchar *xml_extract_tag(const gchar *data, gchar *tag)
{
	gchar *regex_str = g_strdup_printf("<%s>[^<]*</%s>", tag, tag);
	GRegex *regex = NULL;
	GError *error = NULL;
	GMatchInfo *match_info;
	gchar *entry = NULL;
	gint tag_len = strlen(tag);

	regex = g_regex_new(regex_str, 0, 0, &error);
	g_assert(regex != NULL);

	g_regex_match(regex, data, 0, &match_info);

	while (match_info && g_match_info_matches(match_info)) {
		gint start;
		gint end;
		gboolean fetched = g_match_info_fetch_pos(match_info, 0, &start, &end);

		if (fetched == TRUE) {
			gint entry_size = end - start - 2 * tag_len - 5;
			entry = g_malloc0(entry_size + 1);
			strncpy(entry, data + start + tag_len + 2, entry_size);
			break;
		}

		if (g_match_info_next(match_info, NULL) == FALSE) {
			break;
		}
	}

	g_match_info_free(match_info);
	g_free(regex_str);

	return entry;
}

/**
 * \brief Extract XML input tag: name="TAG" ... value="VALUE"
 * \param data data to parse
 * \param tag tag to extract
 * \return tag value
 */
gchar *xml_extract_input_value(const gchar *data, gchar *tag)
{
	gchar *name = g_strdup_printf("name=\"%s\"", tag);
	gchar *start = g_strstr_len(data, -1, name);
	gchar *val_start = NULL;
	gchar *val_end = NULL;
	gchar *value = NULL;
	gssize val_size;

	g_free(name);
	if (start == NULL) {
		return NULL;
	}

	val_start = g_strstr_len(start, -1, "value=\"");
	g_assert(val_start != NULL);

	val_start += 7;

	val_end = g_strstr_len(val_start, -1, "\"");

	val_size = val_end - val_start;
	g_assert(val_size >= 0);

	value = g_malloc0(val_size + 1);
	memcpy(value, val_start, val_size);

	return value;
}

/**
 * \brief Extract XML list tag: ["TAG"] = "VALUE"
 * \param data data to parse
 * \param tag tag to extract
 * \return tag value
 */
gchar *xml_extract_list_value(const gchar *data, gchar *tag)
{
	gchar *name = g_strdup_printf("\"%s\"", tag);
	gchar *start = g_strstr_len(data, -1, name);
	gchar *val_start = NULL;
	gchar *val_end = NULL;
	gchar *value = NULL;
	gssize val_size;

	g_free(name);
	if (start == NULL) {
		return NULL;
	}

	start += strlen(tag) + 2;

	val_start = g_strstr_len(start, -1, "\"");
	g_assert(val_start != NULL);

	val_start += 1;

	val_end = g_strstr_len(val_start, -1, "\"");

	val_size = val_end - val_start;
	g_assert(val_size >= 0);

	value = g_malloc0(val_size + 1);
	memcpy(value, val_start, val_size);

	return value;
}

/**
 * \brief Extract HTML assignment: TAG = "VALUE"
 * \param data data to parse
 * \param tag tag to extract
 * \param p value is surrounded by " flag
 * \return tag value
 */
gchar *html_extract_assignment(const gchar *data, gchar *tag, gboolean p)
{
	gchar *name = g_strdup_printf("%s", tag);
	gchar *start = g_strstr_len(data, -1, name);
	gchar *val_start = NULL;
	gchar *val_end = NULL;
	gchar *value = NULL;
	gssize val_size;

	g_free(name);
	if (start == NULL) {
		return NULL;
	}

	start += strlen(tag);

	if (p == 1) {
		start += 2;
		val_start = g_strstr_len(start, -1, "\"");
		g_assert(val_start != NULL);

		val_start += 1;

		val_end = g_strstr_len(val_start, -1, "\"");

		val_size = val_end - val_start;
		g_assert(val_size >= 0);
	} else {
		val_start = start;
		g_assert(val_start != NULL);

		val_start += 1;

		val_end = g_strstr_len(val_start, -1, "\n");

		val_size = val_end - val_start - 2;
		g_assert(val_size >= 0);
	}

	value = g_malloc0(val_size + 1);
	memcpy(value, val_start, val_size);

	return value;
}

/**
 * \brief Check if a FRITZ!Box router is present
 * \param router_info - router information structure
 * \return true if fritzbox and type could be retrieved, otherwise false
 */
gboolean fritzbox_present(struct router_info *router_info)
{
	SoupMessage *msg;
	gsize read;
	const gchar *data;
	gchar *name;
	gchar *version;
	gchar *lang;
	gchar *serial;
	gchar *url;
	gboolean ret = FALSE;

	if (router_info->name != NULL) {
		g_free(router_info->name);
	}

	if (router_info->version != NULL) {
		g_free(router_info->version);
	}

	if (router_info->session_timer != NULL) {
		//g_timer_destroy(router_info->session_timer);
		router_info->session_timer = NULL;
	}

	url = g_strdup_printf("http://%s/jason_boxinfo.xml", router_info->host);
	msg = soup_message_new(SOUP_METHOD_GET, url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_warning("Could not read boxinfo file");
		g_object_unref(msg);
		g_free(url);

		return ret;
	}
	data = msg->response_body->data;
	read = msg->response_body->length;

	log_save_data("fritzbox-present.html", data, read);
	g_return_val_if_fail(data != NULL, FALSE);

	name = xml_extract_tag(data, "j:Name");
	version = xml_extract_tag(data, "j:Version");
	lang = xml_extract_tag(data, "j:Lang");
	serial = xml_extract_tag(data, "j:Serial");

	g_object_unref(msg);
	g_free(url);

	if (name && version && lang && serial) {
		gchar **split;

		router_info->name = g_strdup(name);
		router_info->version = g_strdup(version);
		router_info->lang = g_strdup(lang);
		router_info->serial = g_strdup(serial);

		/* Version: Box.Major.Minor(-XXXXX) */

		split = g_strsplit(router_info->version, ".", -1);

		router_info->box_id = atoi(split[0]);
		router_info->maj_ver_id = atoi(split[1]);
		router_info->min_ver_id = atoi(split[2]);

		g_strfreev(split);
		ret = TRUE;
	} else {
		g_warning("name, version, lang or serial not valid");
	}

	g_free(serial);
	g_free(lang);
	g_free(version);
	g_free(name);

	return ret;
}

/**
 * \brief Logout of box (if session_timer is not present)
 * \param profile profile information structure
 * \param force force logout
 * \return error code
 */
gboolean fritzbox_logout(struct profile *profile, gboolean force)
{
	SoupMessage *msg;
	gchar *url;

	if (profile->router_info->session_timer && !force) {
		return TRUE;
	}

	url = g_strdup_printf("http://%s/cgi-bin/webcm", router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "sid", profile->router_info->session_id,
	                            "security:command/logout", "",
	                            "getpage", "../html/confirm_logout.html",
	                            NULL);
	g_free(url);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}

	g_timer_destroy(profile->router_info->session_timer);
	profile->router_info->session_timer = NULL;

	g_object_unref(msg);
	g_debug("Logout successful");

	return TRUE;
}

/**
 * \brief Read MSNs of data
 * \param profile profile information structure
 * \param data data to parse for MSNs
 */
void fritzbox_read_msn(struct profile *profile, const gchar *data)
{
	gchar *func;
	gchar *pots_start;
	gchar *pots_end;
	gchar *pots;
	gint pots_len;

	gchar *msns_start;
	gchar *msns_end;
	gchar *msns;
	gint msns_len;

	gchar *sips_start;
	gchar *sips_end;
	gchar *sips;
	gint sips_len;

	gint index;

	/* Check for readFonNumbers() function */
	func = g_strstr_len(data, -1, "readFonNumbers()");
	if (!func) {
		return;
	}

	/* Extract POTS */
	pots_start = g_strstr_len(func, -1, "nrs.pots");
	g_assert(pots_start != NULL);

	pots_start += 11;

	pots_end = g_strstr_len(pots_start, -1, "\"");
	g_assert(pots_end != NULL);

	pots_len = pots_end - pots_start;

	pots = g_slice_alloc0(pots_len + 1);
	strncpy(pots, pots_start, pots_len);
	if (strlen(pots) > 0) {
		g_debug("pots: '%s'", pots);
	}
	g_slice_free1(pots_len + 1, pots);

	/* Extract MSNs */
	for (index = 0; index < 10; index++) {
		msns_start = g_strstr_len(func, -1, "nrs.msn.push");
		g_assert(msns_start != NULL);

		msns_start += 14;

		msns_end = g_strstr_len(msns_start, -1, "\"");
		g_assert(msns_end != NULL);

		msns_len = msns_end - msns_start;

		msns = g_slice_alloc0(msns_len + 1);
		strncpy(msns, msns_start, msns_len);

		if (strlen(msns) > 0) {
			g_debug("msn%d: '%s'", index, msns);
		}
		g_slice_free1(msns_len + 1, msns);

		func = msns_end;
	}

	/* Extract SIPs */
	for (index = 0; index < 19; index++) {
		sips_start = g_strstr_len(func, -1, "nrs.sip.push");
		g_assert(sips_start != NULL);

		sips_start += 14;

		sips_end = g_strstr_len(sips_start, -1, "\"");
		g_assert(sips_end != NULL);

		sips_len = sips_end - sips_start;

		sips = g_slice_alloc0(sips_len + 1);
		strncpy(sips, sips_start, sips_len);

		if (strlen(sips) > 0) {
			g_debug("sip%d: '%s'", index, sips);
		}
		g_slice_free1(sips_len + 1, sips);

		func = sips_end;
	}
}


/**
 * \brief Dial number callback function (logout from box)
 * \param session soup session
 * \param msg soup message
 * \param user_data pointer to profile structure
 */
static void dial_number_cb(SoupSession *session, SoupMessage *msg, gpointer user_data)
{
	struct profile *profile = user_data;

	/* Logout */
	fritzbox_logout(profile, FALSE);
}

/**
 * \brief Depending on type get dialport
 * \param type phone type
 * \return dialport
 */
static gint fritzbox_get_dialport(gint type)
{
	gint index;

	for (index = 0; index < NUM_PHONE_PORTS; index++) {
		if (fritzbox_phone_ports[index].type == type) {
			return fritzbox_phone_ports[index].number;
		}
	}

	return -1;
}

/**
 * \brief Dial number using ClickToDial
 * \param profile profile information structure
 * \param port dial port
 * \param number remote number
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_dial_number(struct profile *profile, gint port, const gchar *number)
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

	g_debug("Call number '%s' on port %s...", call_scramble_number(number), port_str);

	msg = soup_form_request_new(SOUP_METHOD_POST, url,
	                            "telcfg:settings/UseClickToDial", "1",
	                            "telcfg:settings/DialPort", port_str,
	                            "telcfg:command/Dial", number,
	                            "sid", profile->router_info->session_id,
	                            NULL);
	g_free(port_str);
	g_free(url);

	/* Add message to queue */
	soup_session_queue_message(soup_session_async, msg, dial_number_cb, profile);

	return TRUE;
}


/**
 * \brief Hangup call
 * \param profile profile information structure
 * \param port dial port
 * \param number remote number
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_hangup(struct profile *profile, gint port, const gchar *number)
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

	/* Add message to queue */
	soup_session_queue_message(soup_session_async, msg, dial_number_cb, profile);

	return TRUE;
}

/**
 * \brief Load faxbox and add it to journal
 * \param journal journal call list
 * \return journal list with added faxbox
 */
GSList *fritzbox_load_faxbox(GSList *journal)
{
	struct profile *profile = profile_get_active();
	struct ftp *client;
	gchar *user = router_get_ftp_user(profile);
	gchar *response;
	gchar *path;
	gchar *volume_path;

	client = ftp_init(router_get_host(profile));
	if (!client) {
		return journal;
	}

	if (!ftp_login(client, user, router_get_ftp_password(profile))) {
		g_warning("Could not login to router ftp");
		return journal;
	}

	if (!ftp_passive(client)) {
		g_warning("Could not switch to passive mode");
		return journal;
	}

	volume_path = g_settings_get_string(profile->settings, "fax-volume");
	path = g_build_filename(volume_path, "FRITZ/faxbox/", NULL);
	g_free(volume_path);
	response = ftp_list_dir(client, path);
	if (response) {
		gchar **split;
		gint index;

		split = g_strsplit(response, "\n", -1);

		for (index = 0; index < g_strv_length(split); index++) {
			gchar date[9];
			gchar time[6];
			gchar remote_number[32];
			gchar *start;
			gchar *pos;
			gchar *full;
			gchar *number;

			start = strstr(split[index], "Telefax");
			if (!start) {
				continue;
			}

			full = g_strconcat(path, split[index], NULL);
			strncpy(date, split[index], 8);
			date[8] = '\0';

			strncpy(time, split[index] + 9, 5);
			time[2] = ':';
			time[5] = '\0';

			pos = strstr(start + 8, ".");
			strncpy(remote_number, start + 8, pos - start - 8);
			remote_number[pos - start - 8] = '\0';

			if (isdigit(remote_number[0])) {
				number = remote_number;
			} else {
				number = "";
			}

			journal = call_add(journal, CALL_TYPE_FAX, g_strdup_printf("%s %s", date, time), "", number, ("Telefax"), "", "0:01", g_strdup(full));
			g_free(full);
		}

		g_free(response);
	}
	g_free(path);

	return journal;
}

/**
 * \brief Parse voice data structure and add calls to journal
 * \param journal journal call list
 * \param data meta data to parse voice data for
 * \param len length of data
 * \return journal call list with voicebox data
 */
static GSList *fritzbox_parse_voice_data(GSList *journal, const gchar *data, gsize len)
{
	gint index;

	for (index = 0; index < len / sizeof(struct voice_data); index++) {
		struct voice_data *voice_data = (struct voice_data *)(data + index * sizeof(struct voice_data));
		gchar date_time[15];

		/* Skip user/standard welcome message */
		if (!strncmp(voice_data->file, "uvp", 3)) {
			continue;
		}

		if (voice_data->header == 0x5C010000) {
			voice_data->header = GINT_TO_BE(voice_data->header);
			voice_data->type = GINT_TO_BE(voice_data->type);
			voice_data->sub_type = GUINT_TO_BE(voice_data->sub_type);
			voice_data->size = GUINT_TO_BE(voice_data->size);
			voice_data->duration = GUINT_TO_BE(voice_data->duration);
			voice_data->status = GUINT_TO_BE(voice_data->status);
		}

		snprintf(date_time, sizeof(date_time), "%2.2d.%2.2d.%2.2d %2.2d:%2.2d", voice_data->day, voice_data->month, voice_data->year,
		         voice_data->hour, voice_data->minute);
		journal = call_add(journal, CALL_TYPE_VOICE, date_time, "", voice_data->remote_number, "", voice_data->local_number, "0:01", g_strdup(voice_data->file));
	}

	return journal;
}

/**
 * \brief Load voicebox and add it to journal
 * \param journal journal call list
 * \return journal list with added voicebox
 */
GSList *fritzbox_load_voicebox(GSList *journal)
{
	struct ftp *client;
	gchar *path;
	gint index;
	struct profile *profile = profile_get_active();
	gchar *user = router_get_ftp_user(profile);
	gchar *volume_path;

	client = ftp_init(router_get_host(profile));
	if (!client) {
		g_warning("Could not init ftp connection. Please check that ftp is enabled");
		return journal;
	}

	if (!ftp_login(client, user, router_get_ftp_password(profile))) {
		g_warning("Could not login to router ftp");
		return journal;
	}

	volume_path = g_settings_get_string(profile->settings, "fax-volume");
	path = g_build_filename(volume_path, "FRITZ/voicebox/", NULL);
	g_free(volume_path);

	for (index = 0; index < 5; index++) {
		gchar *file = g_strdup_printf("%smeta%d", path, index);
		gchar *file_data;
		gsize file_size = 0;

		if (!ftp_passive(client)) {
			g_warning("Could not switch to passive mode");
			break;
		}

		file_data = ftp_get_file(client, file, &file_size);
		g_free(file);

		if (file_data && file_size) {
			voice_boxes[index].len = file_size;
			voice_boxes[index].data = g_malloc(voice_boxes[index].len);
			memcpy(voice_boxes[index].data, file_data, file_size);
			journal = fritzbox_parse_voice_data(journal, file_data, file_size);
			g_free(file_data);
		} else {
			g_free(file_data);
			break;
		}
	}
	g_free(path);

	return journal;
}

/**
 * \brief Load fax file via FTP
 * \param profile profile structure
 * \param filename fax filename
 * \param len pointer to store the data length to
 * \return fax data
 */
gchar *fritzbox_load_fax(struct profile *profile, const gchar *filename, gsize *len)
{
	struct ftp *client;
	gchar *user = router_get_ftp_user(profile);

	client = ftp_init(router_get_host(profile));
	ftp_login(client, user, router_get_ftp_password(profile));

	ftp_passive(client);

	return ftp_get_file(client, filename, len);
}

/**
 * \brief Load voice file via FTP
 * \param profile profile structure
 * \param name voice filename
 * \param len pointer to store the data length to
 * \return voice data
 */
gchar *fritzbox_load_voice(struct profile *profile, const gchar *name, gsize *len)
{
	struct ftp *client;
	gchar *filename = g_strconcat("/", g_settings_get_string(profile->settings, "fax-volume"), "/FRITZ/voicebox/rec/", name, NULL);
	gchar *user = router_get_ftp_user(profile);
	gchar *ret = NULL;

	client = ftp_init(router_get_host(profile));
	if (!client) {
		g_debug("Could not init ftp connection");
		return ret;
	}

	ftp_login(client, user, router_get_ftp_password(profile));

	ftp_passive(client);

	ret = ftp_get_file(client, filename, len);
	g_free(filename);

	return ret;
}

/**
 * \brief Find phone port by dial port
 * \param dial_port dial port number
 * \param phone port on success, otherwise -1
 */
gint fritzbox_find_phone_port(gint dial_port)
{
	gint index;

	for (index = 0; index < NUM_PHONE_PORTS; index++) {
		if (fritzbox_phone_ports[index].number == dial_port) {
			return fritzbox_phone_ports[index].type;
		}
	}

	return -1;
}

/**
 * \brief Extract IP address from router
 * \param profile profile pointer
 * \return current IP address or NULL on error
 */
gchar *fritzbox_get_ip(struct profile *profile)
{
	SoupMessage *msg;
	SoupURI *uri;
	gchar *ip = NULL;
	gchar *request;
	SoupMessageHeaders *headers;

	/* Create POST message */
	gchar *url = g_strdup_printf("http://%s/upnp/control/WANIPConn1", router_get_host(profile));

	uri = soup_uri_new(url);
	soup_uri_set_port(uri, 49000);
	msg = soup_message_new_from_uri(SOUP_METHOD_POST, uri);
	g_free(url);

	request = g_strdup(
	              "<?xml version='1.0' encoding='utf-8'?>"
	              " <s:Envelope s:encodingStyle='http://schemas.xmlsoap.org/soap/encoding/' xmlns:s='http://schemas.xmlsoap.org/soap/envelope/'>"
	              " <s:Body>"
	              " <u:GetExternalIPAddress xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\" />"
	              " </s:Body>"
	              " </s:Envelope>\r\n");

	soup_message_set_request(msg, "text/xml; charset=\"utf-8\"", SOUP_MEMORY_STATIC, request, strlen(request));
	headers = msg->request_headers;
	soup_message_headers_append(headers, "SoapAction", "urn:schemas-upnp-org:service:WANIPConnection:1#GetExternalIPAddress");

	soup_session_send_message(soup_session_sync, msg);

	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return NULL;
	}

	ip = xml_extract_tag(msg->response_body->data, "NewExternalIPAddress");

	g_object_unref(msg);

	g_debug("Got IP data (%s)", ip);

	return ip;
}

/**
 * \brief Reconnect network
 * \param profile profile pointer
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_reconnect(struct profile *profile)
{
	SoupMessage *msg;
	SoupURI *uri;
	gchar *request;
	SoupMessageHeaders *headers;

	/* Create POST message */
	gchar *url = g_strdup_printf("http://%s:49000/upnp/control/WANIPConn1", router_get_host(profile));

	uri = soup_uri_new(url);
	soup_uri_set_port(uri, 49000);
	msg = soup_message_new_from_uri(SOUP_METHOD_POST, uri);
	g_free(url);

	request = g_strdup(
	              "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
	              " <s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
	              " <s:Body>"
	              " <u:ForceTermination xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\" />"
	              " </s:Body>"
	              " </s:Envelope>\r\n");

	soup_message_set_request(msg, "text/xml; charset=\"utf-8\"", SOUP_MEMORY_STATIC, request, strlen(request));
	headers = msg->request_headers;
	soup_message_headers_append(headers, "SoapAction", "urn:schemas-upnp-org:service:WANIPConnection:1#ForceTermination");

	soup_session_send_message(soup_session_sync, msg);

	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}

	g_object_unref(msg);

	return TRUE;
}

/**
 * \brief Delete fax file from router
 * \param profile profile pointer
 * \param filename fax filename to delete
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_delete_fax(struct profile *profile, const gchar *filename)
{
	struct ftp *client;
	gchar *user = router_get_ftp_user(profile);

	client = ftp_init(router_get_host(profile));
	ftp_login(client, user, router_get_ftp_password(profile));

	ftp_passive(client);

	return ftp_delete_file(client, filename);
}

/**
 * \brief Delete voice file from router
 * \param profile profile pointer
 * \param filename voice filename to delete
 * \return TRUE on success, otherwise FALSE
 */
gboolean fritzbox_delete_voice(struct profile *profile, const gchar *filename)
{
	struct ftp *client;
	struct voice_data *voice_data;
	gpointer modified_data;
	gint nr;
	gint count;
	gint index;
	gint offset = 0;
	gchar *name;

	nr = filename[4] - '0';
	if (!voice_boxes[nr].data || voice_boxes[nr].len == 0) {
		return FALSE;
	}

	/* Modify data */
	count = voice_boxes[nr].len / sizeof(struct voice_data);
	modified_data = g_malloc((count - 1) * sizeof(struct voice_data));

	for (index = 0; index < count; index++) {
		voice_data = (struct voice_data *)(voice_boxes[nr].data + index * sizeof(struct voice_data));
		if (strncmp(voice_data->file, filename, strlen(filename)) != 0) {
			memcpy(modified_data + offset, voice_boxes[nr].data + index * sizeof(struct voice_data), sizeof(struct voice_data));
			offset += sizeof(struct voice_data);
		}
	}

	/* Write data to router */
	client = ftp_init(router_get_host(profile));
	ftp_login(client, router_get_ftp_user(profile), router_get_ftp_password(profile));

	gchar *path = g_build_filename(g_settings_get_string(profile->settings, "fax-volume"), "FRITZ/voicebox/", NULL);
	gchar *remote_file = g_strdup_printf("meta%d", nr);
	if (!ftp_put_file(client, remote_file, path, modified_data, offset)) {
		g_free(modified_data);
		g_free(remote_file);
		g_free(path);
		return FALSE;
	}

	g_free(remote_file);
	g_free(path);

	/* Modify internal data structure */
	g_free(voice_boxes[nr].data);
	voice_boxes[nr].data = modified_data;
	voice_boxes[nr].len = offset;

	/* Delete voice file */
	name = g_build_filename(g_settings_get_string(profile->settings, "fax-volume"), "FRITZ/voicebox/rec", filename, NULL);
	if (!ftp_delete_file(client, name)) {
		g_free(name);
		return FALSE;
	}

	g_free(name);

	return TRUE;
}
