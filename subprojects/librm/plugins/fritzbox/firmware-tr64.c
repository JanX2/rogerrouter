/*
 * The rm project
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
#include <json-glib/json-glib.h>

#include <libgupnp/gupnp.h>
#include <libgupnp/gupnp-device-info.h>

#include <rm/xml.h>
#include <rm/rmprofile.h>
#include <rm/rmfile.h>
#include <rm/rmlog.h>
#include <rm/rmnetwork.h>
#include <rm/rmcsv.h>
#include <rm/rmftp.h>
#include <rm/rmcallentry.h>
#include <rm/rmnumber.h>
#include <rm/rmstring.h>
#include <rm/rmcallentry.h>
#include <rm/rmjournal.h>

#include "fritzbox.h"
#include "firmware-common.h"
#include "firmware-query.h"

#define FIRMWARE_TR64_DEBUG 1

static gint firmware_tr64_security_port = 0;

/**
 * firmware_tr64_get_service:
 * @profile: a #RmProfile
 *
 * Try to get tr64 service
 */
void firmware_tr64_get_service(RmProfile *profile)
{
	SoupMessage *msg;
	gchar *url;

	/* Create message */
	url = g_strdup_printf("http://%s:49000/tr64desc.xml", rm_router_get_host(profile));

	msg = soup_message_new(SOUP_METHOD_GET, url);
	g_free(url);

	soup_session_send_message(rm_soup_session, msg);

	if (msg->status_code != SOUP_STATUS_OK) {
		g_debug("%s(): Received status code: %d", __FUNCTION__, msg->status_code);
		g_object_unref(msg);
		return;
	}

#ifdef FIRMWARE_TR64_DEBUG
	rm_log_save_data("tr64desc.xml", msg->response_body->data, -1);
#endif

	g_object_unref(msg);
}

/**
 * firmware_tr64_create_response:
 * @nonce: a nonce
 * @realm: router realm
 * @user: router user
 * @password: router password
 *
 * Create authentication token
 *
 * Returns: authentication token
 */
gchar *firmware_tr64_create_response(gchar *nonce, gchar *realm, gchar *user, gchar *password)
{
		/** secret = MD5( concat(uid, ":", realm, ":", pwd) ) */
		gchar *secret = g_strconcat(user, ":", realm, ":", password, NULL);
		gchar *secret_md5 = md5_simple(secret);
		/** response = MD5( concat(secret, ":", sn) ) */
		gchar *response = g_strconcat(secret_md5, ":", nonce, NULL);
		gchar *response_md5 = md5_simple(response);

		return response_md5;
}

#define SOUP_MSG_START	"<?xml version='1.0' encoding='utf-8'?>"\
	            				"<s:Envelope s:encodingStyle='http://schemas.xmlsoap.org/soap/encoding/' xmlns:s='http://schemas.xmlsoap.org/soap/envelope/'>"

#define SOUP_MSG_END "</s:Envelope>\r\n"
#define SOUP_MSG_HEADER_START "<s:Header>"
#define SOUP_MSG_HEADER_END "</s:Header>"
#define SOUP_MSG_BODY_START "<s:Body>"
#define SOUP_MSG_BODY_END "</s:Body>"

/**
 * firmware_tr64_request:
 * @profile: a #RmProfile
 * @auth: authentication required flag
 * @control: upnp control
 * @action: soap action
 * @service: soap service
 *
 * Send a tr64 soap request
 *
 * Returns: #SoupMessage as a result of tr64 send request
 */
static SoupMessage *firmware_tr64_request(RmProfile *profile, gboolean auth, gchar *control, gchar *action, gchar *service, ...)
{
	SoupMessage *msg;
	SoupMessageHeaders *headers;
	gchar *url;
	GString *request = g_string_new(NULL);
	SoupURI *uri;
	gchar *status;
	gint port;

	if (!auth) {
		url = g_strdup_printf("http://%s/upnp/control/%s", rm_router_get_host(profile), control);
		port = 49000;
	} else {
		url = g_strdup_printf("https://%s/upnp/control/%s", rm_router_get_host(profile), control);
		port = firmware_tr64_security_port;
	}

	uri = soup_uri_new(url);
	soup_uri_set_port(uri, port);
	msg = soup_message_new_from_uri(SOUP_METHOD_POST, uri);
	g_free(url);

	g_string_append(request, SOUP_MSG_START);

	if (auth) {
		g_string_append_printf(request, SOUP_MSG_HEADER_START "<h:InitChallenge xmlns:h=\"http://soap-authentication.org/digest/2001/10/\" s:mustUnderstand=\"1\">\
        <UserID>%s</UserID></h:InitChallenge>" SOUP_MSG_HEADER_END, rm_router_get_login_user(profile));
	}

	g_string_append_printf(request, SOUP_MSG_BODY_START "<u:%s xmlns:u='%s'>", action, service);

	va_list arg;
	gchar *key;

	va_start(arg, service);
	while ((key = va_arg(arg, char *)) != NULL) {
		gchar *val = va_arg(arg, char *);
		g_string_append_printf(request, "<%s>%s</%s>", key, val, key);
	}
	va_end(arg);
	g_string_append_printf(request, "</u:%s>" SOUP_MSG_BODY_END SOUP_MSG_END, action);

#ifdef FIRMWARE_TR64_DEBUG
	g_debug("%s(): SoupRequest: %s", __FUNCTION__, request->str);
#endif

	soup_message_set_request(msg, "text/xml; charset=\"utf-8\"", SOUP_MEMORY_STATIC, request->str, request->len);
	headers = msg->request_headers;
	gchar *header = g_strdup_printf("%s#%s", service, action);
	soup_message_headers_append(headers, "SoapAction", header);

	soup_session_send_message(rm_soup_session, msg);
	g_string_free(request, TRUE);

	if (msg->status_code != SOUP_STATUS_OK) {
		g_debug("%s(): Received status code: %d", __FUNCTION__, msg->status_code);
		rm_log_save_data("tr64-request-error1.xml", msg->response_body->data, -1);
		g_object_unref(msg);
		return NULL;
	}
#ifdef FIRMWARE_TR64_DEBUG
	rm_log_save_data("tr64-request-ok-1.xml", msg->response_body->data, -1);
#endif

	status = xml_extract_tag(msg->response_body->data, "Status");
	if (status && !strcmp(status, "Unauthenticated")) {

#ifdef FIRMWARE_TR64_DEBUG
		g_debug("%s(): Login required", __FUNCTION__);
#endif
		gchar *nonce = xml_extract_tag(msg->response_body->data, "Nonce");
		gchar *realm = xml_extract_tag(msg->response_body->data, "Realm");
		gchar *response;

		response = firmware_tr64_create_response(nonce, realm, rm_router_get_login_user(profile), rm_router_get_login_password(profile));

		request = g_string_new(SOUP_MSG_START);

		g_string_append_printf(request,
									SOUP_MSG_HEADER_START
										"<h:ClientAuth xmlns:h='http://soap-authentication.org/digest/2001/10/' s:mustUnderstand='1'>"
											"<Nonce>%s</Nonce>"
											"<Auth>%s</Auth>"
											"<UserID>%s</UserID>"
											"<Realm>%s</Realm>"
										"</h:ClientAuth>"
									SOUP_MSG_HEADER_END,
									nonce, response, rm_router_get_login_user(profile), realm);

			g_string_append_printf(request, SOUP_MSG_BODY_START "<u:%s xmlns:u='%s'>", action, service);

			va_list arg;
			gchar *key;

			va_start(arg, service);
			while ((key = va_arg(arg, char *)) != NULL) {
				gchar *val = va_arg(arg, char *);
				g_string_append_printf(request, "<%s>%s</%s>", key, val, key);
			}
			va_end(arg);
			g_string_append_printf(request, "</u:%s>" SOUP_MSG_BODY_END SOUP_MSG_END, action);

		g_object_unref(msg);
		msg = soup_message_new_from_uri(SOUP_METHOD_POST, uri);

#ifdef FIRMWARE_TR64_DEBUG
		g_debug("%s(): SoupRequest (auth): %s", __FUNCTION__, request->str);
#endif

		soup_message_set_request(msg, "text/xml; charset=\"utf-8\"", SOUP_MEMORY_STATIC, request->str, request->len);
		headers = msg->request_headers;
		gchar *header = g_strdup_printf("%s#%s", service, action);
		soup_message_headers_append(headers, "SoapAction", header);

		soup_session_send_message(rm_soup_session, msg);
		g_string_free(request, FALSE);

		if (msg->status_code != SOUP_STATUS_OK) {
			g_debug("%s(): Received status code: %d", __FUNCTION__, msg->status_code);
			rm_log_save_data("tr64-request-error2.xml", msg->response_body->data, -1);
			g_object_unref(msg);
			return NULL;
		}

#ifdef FIRMWARE_TR64_DEBUG
		rm_log_save_data("tr64-request-ok-2.xml", msg->response_body->data, -1);
#endif
	}

	return msg;
}

/**
 * firmware_tr64_get_security_port:
 * @profile: a #RmProfile
 *
 * Retrieve security port
 *
 * Returns: security port number if available, otherwise 0
 */
static gint firmware_tr64_get_security_port(RmProfile *profile)
{
	SoupMessage *msg;
	gchar *port = NULL;

	msg = firmware_tr64_request(profile, FALSE, "deviceinfo", "GetSecurityPort", "urn:dslforum-org:service:DeviceInfo:1", NULL);
	if (!msg) {
		return 0;
	}

	port = xml_extract_tag(msg->response_body->data, "NewSecurityPort");

	g_object_unref(msg);

	return atoi(port);
}

/**
 * firmware_tr64_add_call:
 * @list: journal list
 * @profile: a #RmProfile
 * @call: xml master node of call element
 *
 * Add @call to journal @list
 *
 * Returns: Updated journal @list
 */
static GSList *firmware_tr64_add_call(GSList *list, RmProfile *profile, xmlnode *call)
{
	//gchar *id;
	gchar *type;
	gchar *called;
	gchar *caller;
	//gchar *caller_number;
	gchar *name;
	//gchar *number_type;
	//gchar *device;
	gchar *port;
	gchar *date_time;
	gchar *duration;
	//gchar *count;
	gchar *path;
	gchar *remote_name;
	gchar *remote_number;
	gchar *local_name;
	gchar *local_number;
	xmlnode *tmp;
	RmCallEntry *call_entry;
	enum rm_call_entry_types call_type;

	//tmp = xmlnode_get_child(call, "Id");
	//id = xmlnode_get_data(tmp);
	tmp = xmlnode_get_child(call, "Type");
	type = xmlnode_get_data(tmp);
	tmp = xmlnode_get_child(call, "Name");
	name = xmlnode_get_data(tmp);
	tmp = xmlnode_get_child(call, "Duration");
	duration = xmlnode_get_data(tmp);
	tmp = xmlnode_get_child(call, "Date");
	date_time = xmlnode_get_data(tmp);
	tmp = xmlnode_get_child(call, "Device");
	local_name = xmlnode_get_data(tmp);
	tmp = xmlnode_get_child(call, "Path");
	path = xmlnode_get_data(tmp);
	tmp = xmlnode_get_child(call, "Port");
	port = xmlnode_get_data(tmp);

	remote_name = name;

	if (atoi(type) == 3) {
		tmp = xmlnode_get_child(call, "CallerNumber");
		caller = xmlnode_get_data(tmp);
		tmp = xmlnode_get_child(call, "Called");
		called = xmlnode_get_data(tmp);

		remote_number = called;
		local_number = caller;
	} else {
		tmp = xmlnode_get_child(call, "CalledNumber");
		called = xmlnode_get_data(tmp);
		tmp = xmlnode_get_child(call, "Caller");
		caller = xmlnode_get_data(tmp);

		remote_number = caller;
		local_number = called;
	}

	call_type = atoi(type);

#ifdef FIRMWARE_TR64_DEBUG
	if (!RM_EMPTY_STRING(path)) {
		g_debug("%s(): path %s, port %s", __FUNCTION__, path, port);
	}
#endif

	if (port && path) {
		gint port_nr = atoi(port);

		if (port_nr == 6 || (port_nr >= 40 && port_nr < 50)) {
			call_type = RM_CALL_ENTRY_TYPE_VOICE;
		} else if (port_nr == 5) {
			call_type = RM_CALL_ENTRY_TYPE_FAX;
#ifdef FIRMWARE_TR64_DEBUG
			g_debug("%s(): path: %s", __FUNCTION__, path);
#endif
		}
	}

	call_entry = rm_call_entry_new(call_type, date_time, remote_name, remote_number, local_name, local_number, duration, g_strdup(path));
	list = rm_journal_add_call_entry(list, call_entry);

	return list;
}

void firmware_tr64_journal_cb(SoupSession *session, SoupMessage *msg, gpointer user_data)
{
	GSList *journal = NULL;
	RmProfile *profile = user_data;
	xmlnode *child;

	if (msg->status_code != SOUP_STATUS_OK) {
		g_debug("%s(): Got invalid data, return code: %d", __FUNCTION__, msg->status_code);
		g_object_unref(msg);
		return;
	}

#ifdef FIRMWARE_TR64_DEBUG
	rm_log_save_data("callist.xml", msg->response_body->data, -1);
#endif

	xmlnode *node = xmlnode_from_str(msg->response_body->data, msg->response_body->length);
	if (node == NULL) {
		g_object_unref(msg);
		return;
	}

	for (child = xmlnode_get_child(node, "Call"); child != NULL; child = xmlnode_get_next_twin(child)) {
		journal = firmware_tr64_add_call(journal, profile, child);
	}

#ifdef FIRMWARE_TR64_DEBUG
	g_debug("%s(): process journal (%d)", __FUNCTION__, g_slist_length(journal));
#endif

	/* Load fax reports */
	journal = rm_router_load_fax_reports(profile, journal);

	/* Load voice records */
	journal = rm_router_load_voice_records(profile, journal);

	/* Process journal list */
	rm_router_process_journal(journal);
}

/**
 * firmware_tr64_load_journal:
 * @profile: a #RmProfile
 *
 * Load journal using x_contact interface
 *
 * Returns: %TRUE on success, otherwise %FALSE
 */
gboolean firmware_tr64_load_journal(RmProfile *profile)
{
	SoupMessage *msg;

	msg = firmware_tr64_request(profile, TRUE, "x_contact", "GetCallList", "urn:dslforum-org:service:X_AVM-DE_OnTel:1", NULL);
	if (!msg) {
		return FALSE;
	}

	gchar *url = xml_extract_tag(msg->response_body->data, "NewCallListURL");
	if (RM_EMPTY_STRING(url)) {
		g_object_unref(msg);
		return FALSE;
	}

#ifdef FIRMWARE_TR64_DEBUG
	rm_log_save_data("getcalllist.xml", msg->response_body->data, -1);
#endif

	g_object_unref(msg);

	msg = soup_message_new(SOUP_METHOD_GET, url);
	g_free(url);

	/* Queue message to session */
	soup_session_queue_message(rm_soup_session, msg, firmware_tr64_journal_cb, profile);

	return TRUE;
}

/**
 * firmware_tr64_load_voice:
 * @profile: a #RmProfile
 * @filename: voice filename
 * @len: length of voice data
 *
 * Load voice file using TR64
 *
 * Returns: voice data or %NULL on error
 */
gchar *firmware_tr64_load_voice(RmProfile *profile, const gchar *filename, gsize *len)
{
	SoupMessage *msg;
	gchar *url;
	gchar *ret = NULL;

	rm_router_login(profile);

	/* Create message */
	url = g_strdup_printf("https://%s:%d%s&sid=%s", rm_router_get_host(profile), firmware_tr64_security_port, filename, profile->router_info->session_id);
	msg = soup_message_new(SOUP_METHOD_GET, url);
	g_free(url);

	soup_session_send_message(rm_soup_session, msg);

	if (msg->status_code != SOUP_STATUS_OK) {
		g_debug("%s(): Received status code: %d", __FUNCTION__, msg->status_code);
		rm_log_save_data("loadvoice-error.xml", msg->response_body->data, -1);
		g_object_unref(msg);
		return NULL;
	}

	*len = msg->response_body->length;
	ret = g_malloc0(*len);
	memcpy(ret, msg->response_body->data, *len);

	g_object_unref(msg);

	return ret;
}

/**
 * firmware_tr64_dial_number:
 * @profile: a #RmProfile
 * @port: dial port
 * @number: target number
 *
 * Dial @number
 *
 * Returns: %TRUE whether number could be dialed, %FALSE on error
 */
gboolean firmware_tr64_dial_number(RmProfile *profile, gint port, const gchar *number)
{
	SoupMessage *msg;
	gint idx = -1;
	gchar *name;
	gchar *prefix = NULL;
	gint i;

	for (i = 0; i < PORT_MAX - 2; i++) {
		if (fritzbox_phone_ports[i].type == port) {
			idx = i;
			break;
		}
	}

	if (idx == -1) {
		return FALSE;
	}

	switch (port) {
	case PORT_ISDNALL...PORT_ISDN8:
		prefix = g_strdup("ISDN: ");
		break;
	case PORT_ANALOG1...PORT_ANALOG3:
		prefix = g_strdup("POTS: ");
		break;
	case PORT_DECT1...PORT_DECT6:
	default:
		prefix = g_strdup("DECT: ");
		break;
	}

	name = g_strdup_printf("%s%s", prefix, g_settings_get_string(fritzbox_settings, fritzbox_phone_ports[idx].setting_name));

	msg = firmware_tr64_request(profile, TRUE, "x_voip", "X_AVM-DE_DialSetConfig", "urn:dslforum-org:service:X_VoIP:1", "NewX_AVM-DE_PhoneName", name, NULL);
	if (!msg) {
		return FALSE;
	}

	if (msg->status_code != SOUP_STATUS_OK) {
		g_debug("%s(): Received status code: %d", __FUNCTION__, msg->status_code);
		rm_log_save_data("dialsetconfig-error.xml", msg->response_body->data, -1);
		g_object_unref(msg);
		return FALSE;
	}
	g_object_unref(msg);

	/* Now dial number */
	msg = firmware_tr64_request(profile, TRUE, "x_voip", "X_AVM-DE_DialNumber", "urn:dslforum-org:service:X_VoIP:1", "NewX_AVM-DE_PhoneNumber", number, NULL);
	if (!msg) {
		return FALSE;
	}

	if (msg->status_code != SOUP_STATUS_OK) {
		g_debug("%s(): Received status code: %d", __FUNCTION__, msg->status_code);
		rm_log_save_data("dialnumber-error.xml", msg->response_body->data, -1);
		g_object_unref(msg);
		return FALSE;
	}
	g_object_unref(msg);

	return TRUE;
}

gboolean firmware_tr64_get_dial_config(RmProfile *profile)
{
	SoupMessage *msg;

	msg = firmware_tr64_request(profile, TRUE, "x_voip", "X_AVM-DE_DialGetConfig", "urn:dslforum-org:service:X_VoIP:1", NULL);
	if (!msg) {
		return FALSE;
	}

	if (msg->status_code != SOUP_STATUS_OK) {
		g_debug("%s(): Received status code: %d", __FUNCTION__, msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}
	rm_log_save_data("getdialconfig.xml", msg->response_body->data, msg->response_body->length);
	g_object_unref(msg);

	return TRUE;
}

gboolean firmware_tr64_set_dial_config(RmProfile *profile)
{
	SoupMessage *msg;

	msg = firmware_tr64_request(profile, TRUE, "x_voip", "X_AVM-DE_DialSetConfig", "urn:dslforum-org:service:X_VoIP:1", "NewX_AVM-DE_PhoneName", "Roger", NULL);
	if (!msg) {
		return FALSE;
	}

	if (msg->status_code != SOUP_STATUS_OK) {
		g_debug("%s(): Received status code: %d", __FUNCTION__, msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}
	rm_file_save("setdialconfig.xml", msg->response_body->data, msg->response_body->length);
	g_object_unref(msg);

	return TRUE;
}

gboolean firmware_tr64_get_numbers(RmProfile *profile)
{
	SoupMessage *msg;

	msg = firmware_tr64_request(profile, TRUE, "x_voip", "X_AVM-DE_GetNumbers", "urn:dslforum-org:service:X_VoIP:1", NULL);
	if (!msg) {
		return FALSE;
	}

	if (msg->status_code != SOUP_STATUS_OK) {
		g_debug("%s(): Received status code: %d", __FUNCTION__, msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}
	rm_file_save("numbers.xml", msg->response_body->data, msg->response_body->length);
	g_object_unref(msg);

	return TRUE;
}

/**
 * firmware_tr64_is_available:
 * @profile: a #RmProfile
 *
 * Check if TR64 is available
 *
 * Returns: %TRUE if TR64 is available, or %FALSE if not
 */
gboolean firmware_tr64_is_available(RmProfile *profile)
{
	firmware_tr64_security_port = firmware_tr64_get_security_port(profile);

#ifdef FIRMWARE_TR64_DEBUG
		g_debug("%s(): Security port %d", __FUNCTION__, firmware_tr64_security_port);
#endif

	return firmware_tr64_security_port != 0;
}

