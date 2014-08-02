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

/**
 * \TODO List:
 *  - Support multiple books
 *  - Write support
 */

#include <string.h>

#include <glib.h>

#include <libroutermanager/address-book.h>
#include <libroutermanager/router.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/gstring.h>

/** Holds the main internal address book */
static struct address_book *internal_book = NULL;
static guint address_book_signal_id = 0;
static GHashTable *table = NULL;

/**
 * \brief Get whole contacts within the main internal address book
 * \return contact list
 */
GSList *address_book_get_contacts(void)
{
	GSList *list = NULL;

	if (internal_book) {
		list = internal_book->get_contacts();
	}

	return list;
}

gboolean address_book_remove_contact(struct contact *contact)
{
	gboolean ret = FALSE;

	if (internal_book && internal_book->remove_contact) {
		ret = internal_book->remove_contact(contact);
	}

	return ret;
}

gboolean address_book_save_contact(struct contact *contact)
{
	gboolean ret = FALSE;

	if (internal_book && internal_book->save_contact) {
		ret = internal_book->save_contact(contact);
	}

	return ret;
}

gboolean address_book_reload_contacts(void)
{
	gboolean ret = FALSE;

	if (internal_book && internal_book->reload_contacts) {
		ret = internal_book->reload_contacts();
	}

	return ret;
}

gboolean address_book_can_save(void)
{
	gboolean ret = FALSE;

	if (internal_book && internal_book->save_contact && internal_book->remove_contact) {
		ret = TRUE;
	}

	return ret;
}

gint address_book_number_compare(gconstpointer a, gconstpointer b)
{
	struct contact *contact = (struct contact *)a;
	gchar *number = (gchar *)b;
	GSList *list = contact->numbers;

	while (list) {
		struct phone_number *phone_number = list->data;
		if (g_strcmp0(phone_number->number, number) == 0) {
			return 0;
		}

		list = list->next;
	}

	return -1;
}

void address_book_contact_process_cb(AppObject *obj, struct contact *contact, gpointer user_data)
{
	struct contact *tmp_contact;
	GSList *contacts;

	if (EMPTY_STRING(contact->number)) {
		/* Contact number is not present, abort */
		return;
	}

	contacts = address_book_get_contacts();
	if (!contacts) {
		return;
	}

	tmp_contact = g_hash_table_lookup(table, contact->number);
	if (tmp_contact) {
		if (!EMPTY_STRING(tmp_contact->name)) {
			contact_copy(tmp_contact, contact);
		} else {
			/* Previous lookup done but no result found */
			return;
		}
	} else {
		GSList *list;
		gchar *full_number = call_full_number(contact->number, FALSE);

		list = g_slist_find_custom(contacts, full_number, address_book_number_compare);
		if (list) {
			tmp_contact = list->data;

			g_hash_table_insert(table, contact->number, tmp_contact);

			contact_copy(tmp_contact, contact);
		} else {
			/* We have found no entry, mark it in table to speedup further lookup */
			tmp_contact = g_slice_alloc0(sizeof(struct contact));
			g_hash_table_insert(table, contact->number, tmp_contact);
		}

		g_free(full_number);
	}
}

/**
 * \brief Register a new address book
 * \param book address book pointer
 */
void routermanager_address_book_register(struct address_book *book)
{
	internal_book = book;

	if (!address_book_signal_id) {
		table = g_hash_table_new(g_str_hash, g_str_equal);
		address_book_signal_id = g_signal_connect(G_OBJECT(app_object), "contact-process", G_CALLBACK(address_book_contact_process_cb), NULL);
	}
}
