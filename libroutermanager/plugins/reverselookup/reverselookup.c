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
#include <libroutermanager/xml.h>
#include <libroutermanager/routermanager.h>

#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "reverselookup.h"

#define ROUTERMANAGER_TYPE_REVERSE_LOOKUP_PLUGIN (routermanager_reverse_lookup_plugin_get_type ())
#define ROUTERMANAGER_REVERSE_LOOKUP_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_REVERSE_LOOKUP_PLUGIN, RouterManagerReverseLookupPlugin))

//#define RL_DEBUG 1

typedef struct {
	guint signal_id;
	GHashTable *table;
} RouterManagerReverseLookupPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_REVERSE_LOOKUP_PLUGIN, RouterManagerReverseLookupPlugin, routermanager_reverse_lookup_plugin)

static GHashTable *table = NULL;
/** Global lookup list */
static GSList *lookup_list = NULL;
/** Lookup country code hash table */
static GHashTable *lookup_table = NULL;

static gchar *replace_number(gchar *url, gchar *full_number)
{
	GRegex *number = g_regex_new("%NUMBER%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	gchar *out = g_regex_replace_literal(number, url, -1, 0, full_number, 0, NULL);

	g_regex_unref(number);

	return out;
}

static gboolean extract_element(gchar **in, xmlNode *a_node, gchar **out)
{
	xmlNode *cur_node = NULL;
	gboolean ret = FALSE;

	for (cur_node = a_node; cur_node && !ret; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			if (!strcmp((gchar*)cur_node->name, in[0])) {
				gchar *type = (gchar*)xmlGetProp(cur_node, (xmlChar*)in[1]);

				if (type && !strcmp(type, in[2])) {
					xmlChar *name = xmlNodeListGetString(cur_node->doc, cur_node->children, TRUE);
					*out = g_strdup(g_strstrip((gchar*)name));
					printf("-> %s\n", *out);
					xmlFree(name);
					return TRUE;
				}
			}
		}

		ret = extract_element(in, cur_node->children, out);
	}

	return ret;
}

/**
 * \brief Internal reverse lookup function
 * \param number number to lookup
 * \param name pointer to store name to
 * \param street pointer to store street to
 * \param zip pointer to store zip to
 * \param city pointer to store city to
 * \return TRUE on success, otherwise FALSE
 */
static gboolean do_reverse_lookup(struct lookup *lookup, gchar *number, gchar **name, gchar **street, gchar **zip, gchar **city)
{
	SoupMessage *msg;
	const gchar *data;
	GRegex *reg = NULL;
	GMatchInfo *info = NULL;
	gchar *rl_tmp;
	struct contact *rl_contact;
	gboolean result = FALSE;
	gchar *full_number;
	gchar *rdata = NULL;
	gchar *file;
	gsize len;

	/* get full number according to service preferences */
	full_number = call_full_number(number, lookup->prefix);

#ifdef RL_DEBUG
	g_debug("New lookup for '%s'", full_number);
#endif

	g_assert(name != NULL);
	g_assert(street != NULL);
	g_assert(zip != NULL);
	g_assert(city != NULL);

	gchar *url = replace_number(lookup->url, full_number);
#ifdef RL_DEBUG
	g_debug("URL: %s", url);
#endif
	SoupURI *uri = soup_uri_new(url);
	msg = soup_message_new_from_uri(SOUP_METHOD_GET, uri);
	soup_message_headers_append (msg->request_headers, "User-Agent", "Mozilla/5.0 (Fedora; Linux x86_64)");

	soup_session_send_message(soup_session_sync, msg);
	soup_uri_free(uri);
	g_free(url);
	if (msg->status_code != 200) {
		g_debug("Service: %s received status code: %d", lookup->service, msg->status_code);
		g_object_unref(msg);
		return FALSE;
	}

	data = msg->response_body->data;
	len = msg->response_body->length;
	if (!len) {
		goto end;
	}

	rdata = g_convert_utf8(data, len);

	if (lookup->pattern) {
#ifdef RL_DEBUG
		g_debug("Lookup pattern '%s'", lookup->pattern);
#endif
		reg = g_regex_new(lookup->pattern, G_REGEX_MULTILINE | G_REGEX_DOTALL, 0, NULL);
		if (!reg) {
			goto end;
		}

		if (!g_regex_match(reg, rdata, 0, &info)) {
#ifdef RL_DEBUG
			gchar *tmp_file = g_strdup_printf("rl-%s-%s.html", lookup->service, number);
			log_save_data(tmp_file, rdata, len);
			g_free(tmp_file);
#endif
			goto end;
		}

#ifdef RL_DEBUG
		gchar *tmp_file = g_strdup_printf("rl-found-%s-%s.html", lookup->service, number);
		log_save_data(tmp_file, rdata, len);
		g_free(tmp_file);
#endif

#ifdef RL_DEBUG
		g_debug("g_match_info_matches()");
#endif

		if (!g_match_info_matches(info)) {
			goto end;
		}

#ifdef RL_DEBUG
		g_debug("g_match_info_fetch_named()");
#endif
		rl_tmp = g_match_info_fetch_named(info, "name");
		if (rl_tmp != NULL) {
			*name = strip_html(rl_tmp);
			g_free(rl_tmp);
		} else {
			*name = g_strdup("");
		}

#ifdef RL_DEBUG
		g_debug(" --> Name: '%s'", *name);
#endif

		rl_tmp = g_match_info_fetch_named(info, "street");
		if (rl_tmp != NULL) {
#ifdef RL_DEBUG
			g_debug(" --> street: '%s'", rl_tmp);
#endif
			*street = strip_html(rl_tmp);
			g_free(rl_tmp);
		} else {
			*street = g_strdup("");
		}

		rl_tmp = g_match_info_fetch_named(info, "zip");
		if (rl_tmp != NULL) {
#ifdef RL_DEBUG
			g_debug(" --> zip: '%s'", rl_tmp);
#endif
			*zip = strip_html(rl_tmp);
			g_free(rl_tmp);
		} else {
			*zip = g_strdup("");
		}

		rl_tmp = g_match_info_fetch_named(info, "city");
		if (!EMPTY_STRING(rl_tmp)) {
			gchar **split;

#ifdef RL_DEBUG
			g_debug(" --> city: '%s'", rl_tmp);
#endif
			*city = strip_html(rl_tmp);

			split = g_strsplit(*city, "\n", -1);
			*city = g_strdup(split[0]);
			g_strfreev(split);

			g_free(rl_tmp);
		} else {
			*city = g_strdup("");
		}
	} else {
		htmlDocPtr html;
		xmlNodePtr node;

		html = htmlReadMemory(data, len, lookup->url, "utf-8", HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);

		node = xmlDocGetRootElement(html);

		if (!extract_element(lookup->name, node, name)) {
#ifdef RL_DEBUG
			gchar *tmp_file = g_strdup_printf("rl-%s-%s.html", lookup->service, number);
			log_save_data(tmp_file, rdata, len);
			g_free(tmp_file);
#endif
			goto end;
		}

		if (!extract_element(lookup->street, node, street)) {
#ifdef RL_DEBUG
			gchar *tmp_file = g_strdup_printf("rl-%s-%s.html", lookup->service, number);
			log_save_data(tmp_file, rdata, len);
			g_free(tmp_file);
#endif
			goto end;
		}

		if (!extract_element(lookup->city, node, city)) {
#ifdef RL_DEBUG
			gchar *tmp_file = g_strdup_printf("rl-%s-%s.html", lookup->service, number);
			log_save_data(tmp_file, rdata, len);
			g_free(tmp_file);
#endif
			goto end;
		}

		if (lookup->zip_len) {
			*zip = g_strndup(*city, lookup->zip_len);
			strcpy(*city, *city + lookup->zip_len + 1);
		}
	}

	rl_contact = g_slice_new0(struct contact);
	rl_contact->name = g_strdup(*name);
	rl_contact->street = g_strdup(*street);
	rl_contact->zip = g_strdup(*zip);
	rl_contact->city = g_strdup(*city);
	g_hash_table_insert(table, number, rl_contact);
	result = TRUE;

	rl_tmp = g_strdup_printf("%s;%s;%s;%s;%s\n",
		number,
		rl_contact->name,
		rl_contact->street,
		rl_contact->zip,
		rl_contact->city);

	file = g_build_filename(g_get_user_cache_dir(), "routermanager", "reverselookup", number, NULL);
	file_save(file, rl_tmp, strlen(rl_tmp));
	g_free(file);
	g_free(rl_tmp);

#ifdef RL_DEBUG
	gchar *tmp_file = g_strdup_printf("rl-found-%s-%s.html", lookup->service, number);
	log_save_data(tmp_file, rdata, len);
	g_free(tmp_file);
#endif

end:
	if (rdata) {
		g_free(rdata);
	}

	if (info) {
		g_match_info_free(info);
	}

	if (reg) {
		g_regex_unref(reg);
	}

	g_object_unref(msg);

	return result;
}

/**
 * \brief Get lookup list
 * \param country_code country code
 * \return lookup list
 */
GSList *get_lookup_list(const gchar *country_code)
{
	GSList *list = NULL;

	/* If country code is NULL, return current lookup list */
	if (!country_code) {
		return lookup_list;
	}

	if (!lookup_table) {
		return NULL;
	}

	/* Extract country code list from hashtable */
	list = g_hash_table_lookup(lookup_table, (gpointer) atol(country_code));

	return list;
}

/**
 * \brief Extract country code from full number
 * \param full_number full international number
 * \return country code or NULL on error
 */
gchar *get_country_code(const gchar *full_number)
{
	gchar sub_string[7];
	int index;
	int len = strlen(full_number);

	for (index = 6; index > 0; index--) {
		if (len <= index) {
			continue;
		}

		strncpy(sub_string, full_number, index);
		sub_string[index] = '\0';

		if (g_hash_table_lookup(lookup_table, (gpointer) atol(sub_string))) {
			return g_strdup(sub_string);
		}
	}

	return NULL;
}

/**
 * \brief Reverse lookup function
 * \param number number to lookup
 * \param name pointer to store name to
 * \param street pointer to store street to
 * \param zip pointer to store zip to
 * \param city pointer to store city to
 * \return TRUE on success, otherwise FALSE
 */
static gboolean reverse_lookup(gchar *number, gchar **name, gchar **street, gchar **zip, gchar **city)
{
	struct lookup *lookup = NULL;
	GSList *list = NULL;
	gchar *full_number = NULL;
	gchar *country_code = NULL;
	gboolean found = FALSE;
	gint international_prefix_len;
	struct profile *profile = profile_get_active();
	struct contact *rl_contact;

	if (!profile) {
		return FALSE;
	}

	/* In case we do not have a number, abort */
	if (EMPTY_STRING(number) || !isdigit(number[0])) {
		return FALSE;
	}

#ifdef RL_DEBUG
	g_debug("Input number '%s'", number);
#endif

	rl_contact = g_hash_table_lookup(table, number);
	if (rl_contact) {
		if (!EMPTY_STRING(rl_contact->name)) {
			*name = g_strdup(rl_contact->name);
			*street = g_strdup(rl_contact->street);
			*zip = g_strdup(rl_contact->zip);
			*city = g_strdup(rl_contact->city);
			return TRUE;
		}

		return FALSE;
	}

	/* Get full number and extract country code if possible */
	full_number = call_full_number(number, TRUE);
	if (!full_number) {
		return FALSE;
	}

#ifdef RL_DEBUG
	g_debug("full number '%s'", full_number);
#endif

	country_code = get_country_code(full_number);
	g_free(full_number);

	international_prefix_len = strlen(router_get_international_prefix(profile));
#ifdef RL_DEBUG
	if (!country_code) {
		g_debug("Warning: Could not get country code!!");
	} else {
		g_debug("Country code: %s", country_code + international_prefix_len);
	}
#endif

	if (!country_code) {
		return FALSE;
	}

	if (strcmp(country_code + international_prefix_len, router_get_country_code(profile_get_active()))) {
		/* if country code is not the same as the router country code, loop through country list */
		list = get_lookup_list(country_code + international_prefix_len);
	} else {
		/* if country code is the same as the router country code, use default plugin */
		list = get_lookup_list(router_get_country_code(profile_get_active()));
	}

	g_free(country_code);

	for (; list != NULL && list->data != NULL; list = list->next) {
		lookup = list->data;

#ifdef RL_DEBUG
		g_debug("Using service '%s'", lookup->service);
#endif

		found = do_reverse_lookup(lookup, number, name, street, zip, city);
		/* in case we found some valid data, break loop */
		if (found) {
			break;
		}
	}

	if (!found) {
		rl_contact = g_slice_new0(struct contact);

		g_hash_table_insert(table, number, rl_contact);
	}

	return found;
}

/**
 * \brief Add lookup from xmlnode
 * \param psNode xml node structure
 */
static void lookup_add(xmlnode *node)
{
	struct lookup *lookup = NULL;
	xmlnode *child = NULL;
	gchar *service = NULL;
	gchar *prefix = NULL;
	gchar *url = NULL;
	gchar *pattern = NULL;
	gchar **name = NULL;
	gchar **street = NULL;
	gchar **city = NULL;
	gint zip_len = 0;

	child = xmlnode_get_child(node, "service");
	g_assert(child != NULL);
	service = xmlnode_get_data(child);

	child = xmlnode_get_child(node, "prefix");
	g_assert(child != NULL);
	prefix = xmlnode_get_data(child);

	child = xmlnode_get_child(node, "url");
	g_assert(child != NULL);
	url = xmlnode_get_data(child);

	child = xmlnode_get_child(node, "pattern");
	if (!child) {
		gchar *tmp;

		child = xmlnode_get_child(node, "name");
		g_assert(child != NULL);
		tmp = xmlnode_get_data(child);
		name = g_strsplit(tmp, " ", -1);

		child = xmlnode_get_child(node, "street");
		g_assert(child != NULL);
		tmp = xmlnode_get_data(child);
		street = g_strsplit(tmp, " ", -1);

		child = xmlnode_get_child(node, "city");
		g_assert(child != NULL);
		tmp = xmlnode_get_data(child);
		city = g_strsplit(tmp, " ", -1);

		tmp = (gchar*)xmlnode_get_attrib(child, "zip");
		if (tmp) {
			zip_len = atoi(tmp);
		}
	} else {
		pattern = xmlnode_get_data(child);
		while ((child = xmlnode_get_next_twin(child)) != NULL) {
			gchar *entry = xmlnode_get_data(child);
			gchar *tmp = g_strconcat(pattern, "\\R", entry, NULL);
			g_free(entry);
			g_free(pattern);
			pattern = tmp;
		}
	}

	lookup = g_slice_alloc0(sizeof(struct lookup));
	g_debug(" o Service: '%s', prefix: %s", service, prefix);
	lookup->service = service;
	lookup->prefix = prefix[ 0 ] == '1';
	lookup->url = url;
	lookup->pattern = pattern;
	lookup->name = name;
	lookup->street = street;
	lookup->city = city;
	lookup->zip_len = zip_len;

	lookup_list = g_slist_prepend(lookup_list, lookup);
}

/**
 * \brief Add country code from xmlnode
 * \param node xml node structure
 */
static void country_code_add(xmlnode *node)
{
	xmlnode *child = NULL;
	const gchar *code = NULL;

	code = xmlnode_get_attrib(node, "code");
	g_debug("Country Code: %s", code);

	lookup_list = NULL;

	for (child = xmlnode_get_child(node, "lookup"); child != NULL; child = xmlnode_get_next_twin(child)) {
		lookup_add(child);
	}
	lookup_list = g_slist_reverse(lookup_list);

	g_hash_table_insert(lookup_table, (gpointer) atol(code), lookup_list);
}

static void lookup_read_cache(gchar *dir_name)
{
#ifndef RL_DEBUG
	GDir *dir;
	GError *error = NULL;
	const gchar *file_name;

	if (!dir_name) {
		return;
	}

	dir = g_dir_open(dir_name, 0, &error);
	if (!dir) {
		g_debug("Could not open lookup directory");
		return;
	}

	while ((file_name = g_dir_read_name(dir))) {
		gchar *data;
		gchar *uri;
		gchar **split;
		struct contact *contact;

		uri = g_build_filename(dir_name, file_name, NULL);
		data = file_load(uri, NULL);
		g_free(uri);

		g_assert(data != NULL);

		split = g_strsplit(data, ";", -1);

		contact = g_slice_new0(struct contact);

		contact->name = g_strdup(split[1]);
		contact->street = g_strdup(split[2]);
		contact->zip = g_strdup(split[3]);
		contact->city = g_strstrip(g_strdup(split[4]));

		g_hash_table_insert(table, g_strdup(split[0]), contact);

		g_strfreev(split);
	}
#endif
}

/**
 * \brief Activate plugin
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	RouterManagerReverseLookupPlugin *reverselookup_plugin = ROUTERMANAGER_REVERSE_LOOKUP_PLUGIN(plugin);
	xmlnode *node, *child;
	gchar *file;

	reverselookup_plugin->priv->table = g_hash_table_new(g_str_hash, g_str_equal);
	table = g_hash_table_new(g_str_hash, g_str_equal);

	file = g_build_filename(g_get_home_dir(), "lookup.xml", NULL);
	if (!g_file_test(file, G_FILE_TEST_EXISTS)) {
		g_free(file);

		file = g_build_filename(get_directory(ROUTERMANAGER_PLUGINS), "reverselookup", "lookup.xml", NULL);
	}

	node = read_xml_from_file(file);
	if (!node) {
		g_debug("Could not read %s", file);
		g_free(file);
		return;
	}
	g_debug("ReverseLookup: '%s'", file);
	g_free(file);

	/* Create new lookup hash table */
	lookup_table = g_hash_table_new(NULL, NULL);

	for (child = xmlnode_get_child(node, "country"); child != NULL; child = xmlnode_get_next_twin(child)) {
		/* Add new country code lists to hash table */
		country_code_add(child);
	}

	file = g_build_filename(g_get_user_cache_dir(), "routermanager", "reverselookup", NULL);
	g_mkdir_with_parents(file, 0770);
	lookup_read_cache(file);
	g_free(file);

	xmlnode_free(node);

	routermanager_lookup_register(reverse_lookup);
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
