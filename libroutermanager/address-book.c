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

#include <glib.h>

#include <libroutermanager/address-book.h>

/** Holds the main internal address book */
static struct address_book *internal_book = NULL;

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

/**
 * \brief Register a new address book
 * \param book address book pointer
 */
void routermanager_address_book_register(struct address_book *book)
{
	internal_book = book;
}
