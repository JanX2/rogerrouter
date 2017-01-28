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

#ifndef __RM_ADDRESSBOOK_H__
#define __RM_ADDRESSBOOK_H__

//#if !defined (__RM_H_INSIDE__) && !defined(RM_COMPILATION)
//#error "Only <rm/rm.h> can be included directly."
//#endif

#include <rm/rmcontact.h>

G_BEGIN_DECLS

typedef struct {
	/** Address book plugin name */
	gchar *name;
	gchar *(*get_active_book_name)(void);
	GSList *(*get_contacts)(void);
	gboolean (*reload_contacts)(void);
	gboolean (*remove_contact)(RmContact *contact);
	gboolean (*save_contact)(RmContact *contact);
	gchar **(*get_sub_books)(void);
	gboolean (*set_sub_book)(gchar *name);
} RmAddressBook;

RmAddressBook *rm_addressbook_get(gchar *name);
GSList *rm_addressbook_get_contacts(RmAddressBook *book);
gboolean rm_addressbook_reload_contacts(RmAddressBook *book);
gboolean rm_addressbook_remove_contact(RmAddressBook *book, RmContact *contact);
gboolean rm_addressbook_save_contact(RmAddressBook *book, RmContact *contact);
gboolean rm_addressbook_can_save(RmAddressBook *book);
void rm_addressbook_register(RmAddressBook *book);
void rm_addressbook_unregister(RmAddressBook *book);
gchar *rm_addressbook_get_name(RmAddressBook *book);
gchar *rm_addressbook_get_sub_name(RmAddressBook *book);
gchar **rm_addressbook_get_sub_books(RmAddressBook *book);
void rm_addressbook_set_sub_book(RmAddressBook *book, gchar *name);
GSList *rm_addressbook_get_plugins(void);

G_END_DECLS

#endif
