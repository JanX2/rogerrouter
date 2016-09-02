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
 */

#include <string.h>

#include <glib.h>

#include <libroutermanager/rmaddressbook.h>
#include <libroutermanager/router.h>
#include <libroutermanager/rmobject.h>
#include <libroutermanager/rmstring.h>

/**
 * SECTION:rmaddressbook
 * @title: RmAddressBook
 * @short_description: Address book handling functions
 *
 * Address book handles plugins and common address book functions.
 */

static guint rm_addressbook_signal_id = 0;
static GHashTable *rm_addressbook_table = NULL;

/** Internal address book list */
static GSList *rm_addressbook_plugins = NULL;

/**
 * rm_addressbook_find:
 * @profile: a #RmProfile
 *
 * Find address book as requested by profile.
 *
 * Returns: a #RmAddressBook, or %NULL on error
 */
RmAddressBook *rm_addressbook_find(RmProfile *profile)
{
	gchar *name = g_settings_get_string(profile->settings, "address-book");
	GSList *list;

	for (list = rm_addressbook_plugins; list != NULL; list = list->next) {
		RmAddressBook *ab = list->data;

		if (ab && ab->name && name && !strcmp(ab->name, name)) {
			return ab;
		}
	}

	return rm_addressbook_plugins ? rm_addressbook_plugins->data : NULL;
}

/**
 * rm_addressbook_get_contacts:
 *
 * Get all contacts within the main internal address book.
 *
 * Returns: contact list
 */
GSList *rm_addressbook_get_contacts(void)
{
	GSList *list = NULL;
	RmAddressBook *ab = rm_addressbook_find(rm_profile_get_active());

	if (ab) {
		list = ab->get_contacts();
	}

	return list;
}

/**
 * rm_addressbook_available:
 *
 * Returns: %TRUE if address book is available, %FALSE if not
 */
gboolean rm_addressbook_available(void)
{
	RmAddressBook *ab = rm_addressbook_find(rm_profile_get_active());

	return ab != NULL;
}

/**
 * rm_addressbook_remove_contact:
 * @contact: a #RmContact
 *
 * Remove given contact from address book
 *
 * Returns: %TRUE when contacts have been successfully removed, %FALSE on error
 */
gboolean rm_addressbook_remove_contact(RmContact *contact)
{
	RmAddressBook *ab = rm_addressbook_find(rm_profile_get_active());
	gboolean ret = FALSE;

	if (ab && ab->remove_contact) {
		ret = ab->remove_contact(contact);
	}

	return ret;
}

/**
 * rm_addressbook_save_contact:
 * @contact: a #RmContact
 *
 * Try to save contact to address book
 *
 * Returns: %TRUE when contacts have been successfully written, %FALSE on error
 */
gboolean rm_addressbook_save_contact(RmContact *contact)
{
	RmAddressBook *ab = rm_addressbook_find(rm_profile_get_active());
	gboolean ret = FALSE;

	if (ab && ab->save_contact) {
		ret = ab->save_contact(contact);
	}

	return ret;
}

/**
 * rm_addressbook_reload_contacts:
 *
 * Reloads contacts of address book
 *
 * Returns: %TRUE if contacts have been reloaded, %FALSE on error
 */
gboolean rm_addressbook_reload_contacts(void)
{
	RmAddressBook *ab = rm_addressbook_find(rm_profile_get_active());
	gboolean ret = FALSE;

	if (ab && ab->reload_contacts) {
		ret = ab->reload_contacts();
	}

	return ret;
}

/**
 * rm_addressbook_can_save:
 *
 * Checks wether current address book can save data
 *
 * Returns: %TRUE if address book can save data, %FALSE if not.
 */
gboolean rm_addressbook_can_save(void)
{
	RmAddressBook *ab = rm_addressbook_find(rm_profile_get_active());
	gboolean ret = FALSE;

	if (ab && ab->save_contact && ab->remove_contact) {
		ret = TRUE;
	}

	return ret;
}

/**
 * rm_address_book_number_in_contact:
 * @a: a #RmContact
 * @b: a number
 *
 * Checks if number @b is within contact @a.
 *
 * @Returns: 0 if present, -1 otherwise
*/
static gint rm_addressbook_number_in_contact(gconstpointer a, gconstpointer b)
{
	RmContact *contact = (struct contact *)a;
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

/**
 * rm_addressbook_contact_process_cb:
 * @obj: a #RmObject
 * @contact: a #RmContact
 * @user_data: user data
 */
static void rm_addressbook_contact_process_cb(RmObject *obj, RmContact *contact, gpointer user_data)
{
	RmContact *tmp_contact;
	GSList *contacts;

	if (RM_EMPTY_STRING(contact->number)) {
		/* Contact number is not present, abort */
		return;
	}

	contacts = rm_addressbook_get_contacts();
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
		gchar *full_number = rm_call_full_number(contact->number, FALSE);

		list = g_slist_find_custom(contacts, full_number, rm_addressbook_number_in_contact);
		if (list) {
			tmp_contact = list->data;

			g_hash_table_insert(rm_addressbook_table, contact->number, tmp_contact);

			rm_contact_copy(tmp_contact, contact);
		} else {
			/* We have found no entry, mark it in rm_addressbook_table to speedup further lookup */
			tmp_contact = g_slice_alloc0(sizeof(struct contact));
			g_hash_table_insert(rm_addressbook_table, contact->number, tmp_contact);
		}

		g_free(full_number);
	}
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

	if (!rm_addressbook_signal_id) {
		rm_addressbook_table = g_hash_table_new(g_str_hash, g_str_equal);
		rm_addressbook_signal_id = g_signal_connect(G_OBJECT(rm_object), "contact-process", G_CALLBACK(rm_addressbook_contact_process_cb), NULL);
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
}

/**
 * rm_addresbook_get_name:
 *
 * Retrieve name of current address book.
 *
 * Returns: current address book name
 */
gchar *rm_addressbook_get_name(void)
{
	RmAddressBook *ab = rm_addressbook_find(rm_profile_get_active());

	return ab ? ab->name : g_strdup("");
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
