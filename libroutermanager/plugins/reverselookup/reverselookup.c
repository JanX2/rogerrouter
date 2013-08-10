/**
 * The libroutermanager project
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

#include <glib.h>

#include <libroutermanager/call.h>
#include <libroutermanager/router.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/file.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/network.h>
#include <libroutermanager/logging.h>
#include <libroutermanager/lookup.h>

#define ROUTERMANAGER_TYPE_REVERSE_LOOKUP_PLUGIN (routermanager_reverse_lookup_plugin_get_type ())
#define ROUTERMANAGER_REVERSE_LOOKUP_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_REVERSE_LOOKUP_PLUGIN, RouterManagerReverseLookupPlugin))

typedef struct {
	guint signal_id;
	GHashTable *table;
} RouterManagerReverseLookupPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_REVERSE_LOOKUP_PLUGIN, RouterManagerReverseLookupPlugin, routermanager_reverse_lookup_plugin)

static GHashTable *table = NULL;

static gboolean reverse_lookup(gchar *number, gchar **name, gchar **address, gchar **zip, gchar **city)
{
	SoupMessage *msg;
	const gchar *data;
	GRegex *reg;
	GMatchInfo *info;
	gchar *rl_tmp;
	struct contact *rl_contact;
	gboolean result = FALSE;
	gchar *full_number;

	/* In case we do not have a number, abort */
	if (EMPTY_STRING(number) || !isdigit(number[0])) {
		return FALSE;
	}

	full_number = call_full_number(number, FALSE);

	rl_contact = g_hash_table_lookup(table, full_number);
	if (rl_contact) {
		g_free(full_number);

		if (!EMPTY_STRING(rl_contact->name)) {
			*name = g_strdup(rl_contact->name);
			*address = g_strdup(rl_contact->street);
			*zip = g_strdup(rl_contact->zip);
			*city = g_strdup(rl_contact->city);
			return TRUE;
		}

		return FALSE;
	}
	//g_debug("New lookup for '%s'", full_number);

	g_assert(name != NULL);
	g_assert(address != NULL);
	g_assert(zip != NULL);
	g_assert(city != NULL);

	msg = soup_form_request_new(SOUP_METHOD_POST, "http://www.11880.com/inverssuche/index/search",
		"_dvform_posted", "1",
		"phoneNumber", full_number,
		NULL);

	soup_session_send_message(soup_session_sync, msg);
	if (msg->status_code != 200) {
		g_debug("Received status code: %d", msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}

	rl_contact = g_slice_new0(struct contact);

	data = msg->response_body->data;

	reg = g_regex_new("<a class=\"namelink\"\\s+href=\"/[^\"]*.html\"><strong>(?P<name>[^<]+)</strong></a>.{10,900}<p class=\"data track\">\\s*(?P<address>(.+))<br />\\s*(?P<zip>\\d{5}) (?P<city>[^<\n]+)\\s*</p>", G_REGEX_MULTILINE | G_REGEX_DOTALL, 0, NULL);
	if (!reg) {
		goto end;
	}

	if (!g_regex_match(reg, data, 0, &info)) {
		//gchar *tmp_file = g_strdup_printf("rl-%s.html", number);
		//file_save(tmp_file, data, msg->response_body->length);
		//g_free(tmp_file);
		goto end;
	}

	if (!g_match_info_matches(info)) {
		goto end;
	}

	if ((rl_tmp = g_match_info_fetch_named(info, "name"))) {
		*name = strip_html(rl_tmp);
		g_free(rl_tmp);
	} else {
		*name = g_strdup("");
	}

	//g_debug("Match found: %s -> %s", number, *name);

	if ((rl_tmp = g_match_info_fetch_named(info, "address"))) {
		*address = strip_html(rl_tmp);
		g_free(rl_tmp);
	} else {
		*address = g_strdup("");
	}

	if ((rl_tmp = g_match_info_fetch_named(info, "zip"))) {
		*zip = strip_html(rl_tmp);
		g_free(rl_tmp);
	} else {
		*zip = g_strdup("");
	}

	if ((rl_tmp = g_match_info_fetch_named(info, "city"))) {
		*city = strip_html(rl_tmp);
		g_free(rl_tmp);
	} else {
		*city = g_strdup("");
	}

	g_match_info_free(info);

	//g_debug("Found entry, adding");

	rl_contact->name = g_strdup(*name);
	rl_contact->street = g_strdup(*address);
	rl_contact->zip = g_strdup(*zip);
	rl_contact->city = g_strdup(*city);
	result = TRUE;

end:
	g_hash_table_insert(table, full_number, rl_contact);

	return result;
}

/**
 * \brief Contact process callback (searches for reverselookup and set city name)
 * \param obj app object
 * \param contact contact structure
 * \param user_data pointer to reverselookup plugin structure
 */
static void reverse_lookup_contact_process_cb(AppObject *obj, struct contact *contact, gpointer user_data)
{
	RouterManagerReverseLookupPlugin *reverselookup_plugin = user_data;
	struct contact *rl_contact;
	gchar *name;
	gchar *street;
	gchar *zip;
	gchar *city;

	/* In case we do not have a number or we already have a valid name, abort */
	if (EMPTY_STRING(contact->number) || !EMPTY_STRING(contact->name)) {
		return;
	}

	rl_contact = g_hash_table_lookup(reverselookup_plugin->priv->table, contact->number);
	if (rl_contact) {
		if (!EMPTY_STRING(rl_contact->name)) {
			g_free(contact->name);
			contact->name = g_strdup(rl_contact->name);
			contact->lookup = TRUE;
		}
		return;
	}

	rl_contact = g_slice_new0(struct contact);

	if (reverse_lookup(contact->number, &name, &street, &zip, &city)) {
		g_free(contact->name);
		contact->name = name;
		contact->lookup = TRUE;

		rl_contact->name = name;
		rl_contact->street = street;
		rl_contact->zip = zip;
		rl_contact->city = city;
	}

	g_hash_table_insert(reverselookup_plugin->priv->table, contact->number, rl_contact);
}

/**
 * \brief Activate plugin
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	RouterManagerReverseLookupPlugin *reverselookup_plugin = ROUTERMANAGER_REVERSE_LOOKUP_PLUGIN(plugin);

	reverselookup_plugin->priv->table = g_hash_table_new(g_str_hash, g_str_equal);
	table = g_hash_table_new(g_str_hash, g_str_equal);

	routermanager_lookup_register(reverse_lookup);

	/* Connect to "contact-process" signal */
	if (1 == 0) {
		reverselookup_plugin->priv->signal_id = g_signal_connect(G_OBJECT(app_object), "contact-process", G_CALLBACK(reverse_lookup_contact_process_cb), reverselookup_plugin);
	}
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerReverseLookupPlugin *reverselookup_plugin = ROUTERMANAGER_REVERSE_LOOKUP_PLUGIN(plugin);

	/* If signal handler is connected: disconnect */
	if (g_signal_handler_is_connected(G_OBJECT(app_object), reverselookup_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), reverselookup_plugin->priv->signal_id);
	}
}
