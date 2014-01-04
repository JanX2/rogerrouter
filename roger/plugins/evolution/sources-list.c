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

#include <libedataserver/eds-version.h>
#include <libebook/e-book-client.h>

#include <libroutermanager/gstring.h>

#include <ebook-sources.h>

/**
 * \brief Get client for active address book
 * \return ebook client
 */
EBookClient *get_selected_ebook_client(void)
{
	EBookClient *client;
	GError *error = NULL;
	const gchar *uri;

	uri = get_selected_ebook_id();
	if (EMPTY_STRING(uri)) {
		client = e_book_client_new_system(&error);
	} else {
		client = e_book_client_new_from_uri(uri, &error);
	}

	return client;
}

GList *get_ebook_list(void)
{
	ESourceList *address_books;
	GSList *groups, *group;
	GSList *sources, *source;
	GList *ebook_list = NULL;
	struct ebook_data *ebook_data = NULL;
	GError *error;

	if (!e_book_client_get_sources(&address_books, &error)) {
		g_warning("Could not retrieve addressbooks, abort!");
		return NULL;
	}

	groups = e_source_list_peek_groups(address_books);
	if (!groups) {
		g_warning("No groups!");
		return NULL;
	}

	for (group = groups; group != NULL; group = group->next) {
		sources = e_source_group_peek_sources(group->data);

		for (source = sources; source != NULL; source = source->next) {
			ESource *e_source = E_SOURCE(source->data);

			ebook_data = g_slice_new(struct ebook_data);
			ebook_data->name = g_strdup(e_source_peek_name(e_source));
			ebook_data->id = g_strdup(e_source_get_uri(e_source));

			ebook_list = g_list_append(ebook_list, ebook_data);
		}
	}

	return ebook_list;
}

