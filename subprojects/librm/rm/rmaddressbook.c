/*
 * The rm project
 * Copyright (c) 2012-2017 Jan-Michael Brummer
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

#include <glib.h>

#include <rm/rmaddressbook.h>
#include <rm/rmobject.h>
#include <rm/rmobjectemit.h>
#include <rm/rmcontact.h>
#include <rm/rmrouter.h>
#include <rm/rmstring.h>
#include <rm/rmcallentry.h>
#include <rm/rmnumber.h>
#include <rm/rmmain.h>

/**
 * SECTION:rmaddressbook
 * @title: RmAddressBook
 * @short_description: Address book handling functions
 *
 * Address book handles plugins and common address book functions.
 */

static guint rm_addressbook_contact_process_id = 0;
static guint rm_addressbook_contacts_changed_id = 0;
static GHashTable *rm_addressbook_table = NULL;

/** Internal address book list */
static GSList *rm_addressbook_plugins = NULL;

/**
 * rm_addressbook_get:
 * @name: name of address book to lookup
 *
 * Find address book as requested by name.
 *
 * Returns: a #RmAddressBook, or %NULL on error
 */
RmAddressBook *rm_addressbook_get(gchar *name)
{
	GSList *list;

	for (list = rm_addressbook_plugins; list != NULL; list = list->next) {
		RmAddressBook *ab = list->data;

		if (ab && ab->name && name && !strcmp(ab->name, name)) {
			return ab;
		}
	}

	return NULL;
}

/**
 * rm_addressbook_get_contacts:
 * @book: a #RmAddressBook
 *
 * Get all contacts within the main internal address book.
 *
 * Returns: contact list or %NULL if no address book is set.
 */
GSList *rm_addressbook_get_contacts(RmAddressBook *book)
{
	if (book) {
		return book->get_contacts();
	}

	return NULL;
}

/**
 * rm_addressbook_remove_contact:
 * @book: a #RmAddressBook
 * @contact: a #RmContact
 *
 * Remove given contact from address book
 *
 * Returns: %TRUE when contacts have been successfully removed, %FALSE on error
 */
gboolean rm_addressbook_remove_contact(RmAddressBook *book, RmContact *contact)
{
	if (book && book->remove_contact) {
		return book->remove_contact(contact);
	}

	return FALSE;
}

/**
 * rm_addressbook_save_contact:
 * @book: a #RmAddressBook
 * @contact: a #RmContact
 *
 * Try to save contact to address book
 *
 * Returns: %TRUE when contacts have been successfully written, %FALSE on error
 */
gboolean rm_addressbook_save_contact(RmAddressBook *book, RmContact *contact)
{
	if (book && book->save_contact) {
		return book->save_contact(contact);
	}

	return FALSE;
}

/**
 * rm_addressbook_reload_contacts:
 * @book: a #RmAddressBook
 *
 * Reloads contacts of address book
 *
 * Returns: %TRUE if contacts have been reloaded, %FALSE on error
 */
gboolean rm_addressbook_reload_contacts(RmAddressBook *book)
{
	if (book && book->reload_contacts) {
		return book->reload_contacts();
	}

	return FALSE;
}

/**
 * rm_addressbook_can_save:
 * @book: a #RmAddressBook
 *
 * Checks wether current address book can save data
 *
 * Returns: %TRUE if address book can save data, %FALSE if not.
 */
gboolean rm_addressbook_can_save(RmAddressBook *book)
{
	if (book && book->save_contact && book->remove_contact) {
		return TRUE;
	}

	return FALSE;
}

/**
 * rm_address_book_number_in_contact:
 * @a: a #RmContact
 * @b: a number
 *
 * Checks if number @b is within contact @a.
 *
 * Returns: 0 if present, -1 otherwise
*/
static gint rm_addressbook_number_in_contact(gconstpointer a, gconstpointer b)
{
	RmContact *contact = (RmContact *)a;
	gchar *number = (gchar *)b;
	GSList *list;

	for (list = contact->numbers; list != NULL; list = list->next) {
		RmPhoneNumber *phone_number = list->data;

		if (g_strcmp0(phone_number->number, number) == 0) {
			return 0;
		}
	}

	return -1;
}

/**
 * rm_addressbook_contact_process_cb:
 * @obj: a #RmObject
 * @contact: a #RmContact
 * @user_data: user data
 */
static void rm_addressbook_contact_process_cb(RmObject *obj, RmContact *contact, gpointer user_data)
{
	RmAddressBook *book = rm_profile_get_addressbook(rm_profile_get_active());
	RmContact *tmp_contact;
	GSList *contacts;
	gchar *number = contact->number;

	if (RM_EMPTY_STRING(contact->number)) {
		/* Contact number is not present, abort */
		return;
	}

	contacts = rm_addressbook_get_contacts(book);
	if (!contacts) {
		return;
	}

	tmp_contact = g_hash_table_lookup(rm_addressbook_table, contact->number);
	if (tmp_contact) {
		if (!RM_EMPTY_STRING(tmp_contact->name)) {
			rm_contact_copy(tmp_contact, contact);
		} else {
			/* Previous lookup done but no result found */
			return;
		}
	} else {
		GSList *list;
		gchar *full_number = rm_number_full(contact->number, FALSE);

		list = g_slist_find_custom(contacts, full_number, rm_addressbook_number_in_contact);
		if (list) {
			tmp_contact = list->data;

			g_hash_table_insert(rm_addressbook_table, g_strdup(contact->number), rm_contact_dup(tmp_contact));

			rm_contact_copy(tmp_contact, contact);
		} else {
			/* We have found no entry, mark it in rm_addressbook_table to speedup further lookup */
			tmp_contact = g_slice_alloc0(sizeof(RmContact));
			g_hash_table_insert(rm_addressbook_table, g_strdup(contact->number), tmp_contact);
		}

		g_free(full_number);
	}

	contact->number = number;
}

/**
 * rm_addressbook_contacts_changed_cb:
 * @obj: a #RmObject
 * @user_data: user data
 *
 * Contacts have changed (new, deleted contacts or new address book). Clear address book hash table.
 */
static void rm_addressbook_contacts_changed_cb(RmObject *obj, gpointer user_data)
{
	g_hash_table_remove_all(rm_addressbook_table);
}

/**
 * rm_addressbook_table_free:
 * @data: a #RmContact
 *
 * Frees contact data.
 */
static void rm_addressbook_table_free(void *data)
{
	rm_contact_free(data);
}

/**
 * rm_addressbook_register:
 * @book: a #RmAddressBook
 *
 * Register a new address book.
 */
void rm_addressbook_register(RmAddressBook *book)
{
	rm_addressbook_plugins = g_slist_prepend(rm_addressbook_plugins, book);

	if (!rm_addressbook_contact_process_id) {
		rm_addressbook_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, rm_addressbook_table_free);
		rm_addressbook_contact_process_id = g_signal_connect(G_OBJECT(rm_object), "contact-process", G_CALLBACK(rm_addressbook_contact_process_cb), NULL);
		rm_addressbook_contacts_changed_id = g_signal_connect(G_OBJECT(rm_object), "contacts-changed", G_CALLBACK(rm_addressbook_contacts_changed_cb), NULL);
	}
}

/**
 * rm_addressbook_unregister:
 * @book: a #RmAddressBook
 *
 * Unregister a new address book.
 */
void rm_addressbook_unregister(RmAddressBook *book)
{
	rm_addressbook_plugins = g_slist_remove(rm_addressbook_plugins, book);

	if (g_slist_length(rm_addressbook_plugins) < 1) {
		g_signal_handler_disconnect(G_OBJECT(rm_object), rm_addressbook_contact_process_id);
		g_signal_handler_disconnect(G_OBJECT(rm_object), rm_addressbook_contacts_changed_id);
		g_hash_table_destroy(rm_addressbook_table);
		rm_addressbook_table = NULL;
	}
}

/**
 * rm_addresbook_get_name:
 * @book: a #RmAddressBook
 *
 * Retrieve name of current address book.
 *
 * Returns: current address book name
 */
gchar *rm_addressbook_get_name(RmAddressBook *book)
{
	return book ? g_strdup(book->name) : g_strdup("");
}

/**
 * rm_addresbook_get_sub_name:
 * @book: a #RmAddressBook
 *
 * Retrieve sub name of current address book.
 *
 * Returns: current sub address book name
 */
gchar *rm_addressbook_get_sub_name(RmAddressBook *book)
{
	if (book && book->get_active_book_name()) {
		return g_strdup(book->get_active_book_name());
	}

	return g_strdup(R_("All contacts"));
}

/**
 * rm_addressbook_get_plugins:
 *
 * Get a list of all address book plugins.
 *
 * Returns: list of address book plugins
 */
GSList *rm_addressbook_get_plugins(void)
{
	return rm_addressbook_plugins;
}

/**
 * rm_addressbook_get_sub_books:
 *
 * Get all sub books provides by given address book plugin
 *
 * Returns: strv of all sub books or %NULL
 */
gchar **rm_addressbook_get_sub_books(RmAddressBook *book)
{
	if (book && book->get_sub_books) {
		return book->get_sub_books();
	}

	return NULL;
}

void rm_addressbook_set_sub_book(RmAddressBook *book, gchar *name)
{
	if (book && book->set_sub_book) {
		if (book->set_sub_book(name)) {
			rm_object_emit_contacts_changed();
		}
	}
}
