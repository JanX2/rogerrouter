/**
 * Roger Router
 * Copyright (c) 2012-2014 Jan-Michael Brummer
 *
 * This file is part of Roger Router.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <libebook/libebook.h>
#include "ebook-sources.h"

GList *get_ebook_list(void);

/**
 * \brief Retrieve source registry
 * \return Registry pointer
 */
static ESourceRegistry *get_source_registry(void)
{
	static ESourceRegistry *registry = NULL;
	GError *error = NULL;

	if (!registry) {
		registry = e_source_registry_new_sync(NULL, &error);
		if (!registry) {
			g_warning("Could not get source registry. Error: %s", error->message);
		}
	}

	return registry;
}

/**
 * \brief Get ESource for active address book
 * \return Esource
 */
static ESource *get_selected_ebook_esource(void)
{
	GList *list;
	ESourceRegistry *registry = get_source_registry();
	const gchar *id = get_selected_ebook_id();

	g_debug("registry: %p id %s", registry, id);
	list = get_ebook_list();
	g_debug("list: %p", list);
	while (list) {
		struct ebook_data *ebook_data = list->data;

		g_debug("%s <-> %s", ebook_data->name, id);
		if (!strcmp(ebook_data->name, id)) {
			return e_source_registry_ref_source(registry, ebook_data->id);
		}
		list = list->next;
	}

	return NULL;
}

/**
 * \brief Get client for active address book
 * \return ebook client
 */
EBookClient *get_selected_ebook_client(void)
{
	EBookClient *client;
	GError *error = NULL;
	ESource *source = get_selected_ebook_esource();

	if (!source) {
		g_debug("Book could not be found....");
		return NULL;
	}

	client = e_book_client_new(source, &error);
	if (!client) {
		g_warning("Could not get selected ebook client. Error: %s", error->message);
	}

	return client;
}

/**
 * \brief Retrieve a list of all address book sources
 * \return List of address book sources
 */
GList *get_ebook_list(void)
{
	GList *source, *sources;
	GList *ebook_list = NULL;
	struct ebook_data *ebook_data = NULL;

	sources = e_source_registry_list_sources(get_source_registry(), E_SOURCE_EXTENSION_ADDRESS_BOOK);

	for (source = sources; source != NULL; source = source->next) {
		ESource *e_source = E_SOURCE(source -> data);
		ESource *parent_source;

		if (!e_source_get_enabled(e_source)) {
			g_debug("Source %s not enabled... skip it", e_source_get_uid(e_source));
			continue;
		}

		ebook_data = g_slice_new(struct ebook_data);
		g_debug("Parent: %s", e_source_get_parent(e_source));
		g_debug("UID: %s", e_source_get_uid(e_source));
		g_debug("Display name: %s", e_source_get_display_name(e_source));

		parent_source = e_source_registry_ref_source(get_source_registry(), e_source_get_parent(e_source));
		g_debug("Parent name: %s", e_source_get_display_name(parent_source));

		ebook_data->name = g_strdup_printf("%s (%s)", e_source_get_display_name(e_source), e_source_get_display_name(parent_source));
		ebook_data->id = e_source_dup_uid(e_source);

		ebook_list = g_list_append(ebook_list, ebook_data);

		g_object_unref(parent_source);
	}

	g_list_free_full(sources, g_object_unref);

	return ebook_list;
}
