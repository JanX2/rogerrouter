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
#include <strings.h>

#include <glib.h>

#include <rm/rmcontact.h>
#include <rm/rmobjectemit.h>
#include <rm/rmrouter.h>

/**
 * SECTION:rmcontact
 * @title: RmContact
 * @short_description: Contact handling functions
 *
 * Contacts represents entries within an address book.
 */

gpointer rm_contact_copy_numbers(gconstpointer src, gpointer data)
{
	RmPhoneNumber *copy = NULL;
	RmPhoneNumber *src_number = (RmPhoneNumber *)src;

	copy = g_slice_new0(RmPhoneNumber);

	copy->type = src_number->type;
	copy->number = g_strdup(src_number->number);

	return copy;
}

/**
 * rm_contact_copy:
 * @src: source #RmContact
 * @dst: destination #RmContact
 *
 * Copies one contact data to another
 */
void rm_contact_copy(RmContact *src, RmContact *dst)
{
	dst->name = g_strdup(src->name);

	if (src->image_len && src->image) {
		dst->image_len = src->image_len;
		dst->image = g_malloc(dst->image_len);
		memcpy(dst->image, src->image, dst->image_len);
	} else {
		dst->image = NULL;
		dst->image_len = 0;
	}

	if (src->numbers) {
		dst->numbers = g_slist_copy_deep(src->numbers, rm_contact_copy_numbers, NULL);
	} else {
		dst->numbers = NULL;
	}

	if (src->addresses) {
		dst->addresses = g_slist_copy(src->addresses);
	} else {
		dst->addresses = NULL;
	}

	dst->number = g_strdup(src->number);
	dst->company = g_strdup(src->company);
	dst->street = g_strdup(src->street);
	dst->zip = g_strdup(src->zip);
	dst->city = g_strdup(src->city);

	dst->priv = src->priv;
}

/**
 * rm_contact_dup:
 * @src: source #RmContact
 *
 * Duplicates a #RmContact
 *
 * Returns: new #RmContact
 */
RmContact *rm_contact_dup(RmContact *src)
{
	RmContact *dst = g_slice_new0(RmContact);

	rm_contact_copy(src, dst);

	return dst;
}

/**
 * rm_contact_name_compare:
 * @a: pointer to first #RmContact
 * @b: pointer to second #RmContact
 *
 * Compares two contacts based on contacts name.
 *
 * Returns: return values of #strcasecmp
 */
gint rm_contact_name_compare(gconstpointer a, gconstpointer b)
{
	RmContact *contact_a = (RmContact *)a;
	RmContact *contact_b = (RmContact *)b;

	return strcasecmp(contact_a->name, contact_b->name);
}

/**
 * rm_contact_find_by_number:
 * @number: phone number
 *
 * Try to find a contact by it's number
 *
 * Returns: a #RmContact if number has been found, or %NULL% if not.
 */
RmContact *rm_contact_find_by_number(gchar *number)
{
	RmContact *contact = g_slice_new0(RmContact);
	GSList *numbers;
	GSList *addresses;
	gint type = -1;

	/** Ask for contact information */
	contact->number = number;
	rm_object_emit_contact_process(contact);

	/* Depending on the number set the active address */
	for (numbers = contact->numbers; numbers != NULL; numbers = numbers->next) {
		RmPhoneNumber *phone_number = numbers->data;

		if (!strcmp(number, phone_number->number)) {
			type = phone_number->type;
			break;
		}
	}

	g_debug("%s(): type %d", __FUNCTION__, type);
	if (type == -1) {
		return contact;
	}

	switch (type) {
	case PHONE_NUMBER_WORK:
	case PHONE_NUMBER_FAX_WORK:
	case PHONE_NUMBER_PAGER:
		type = 1;
		break;
	case PHONE_NUMBER_HOME:
	case PHONE_NUMBER_FAX_HOME:
	case PHONE_NUMBER_MOBILE:
	default:
		type = 0;
		break;
	}

	for (addresses = contact->addresses; addresses != NULL; addresses = addresses->next) {
		RmContactAddress *contact_address = addresses->data;

		if (contact_address->type == type) {
			contact->street = g_strdup(contact_address->street);
			contact->city = g_strdup(contact_address->city);
			contact->zip = g_strdup(contact_address->zip);
			break;
		}
	}

	return contact;
}

/**
 * rm_contact_free_address:
 * @data: pointer to #RmContactAddress
 *
 * Frees #RmContactAddress.
 */
static void rm_contact_free_address(gpointer data)
{
	RmContactAddress *address = data;

	if (!address) {
		g_debug("%s(): No address at all, exit", __FUNCTION__);
		return;
	}

	g_free(address->street);
	address->street = NULL;
	g_free(address->zip);
	address->zip = NULL;
	g_free(address->city);
	address->city = NULL;

	g_slice_free(RmContactAddress, address);
}

/**
 * rm_contact_free_number:
 * @data: pointer to #RmPhoneNumber
 *
 * Frees #RmPhoneNumber.
 */
static void rm_contact_free_number(gpointer data)
{
	RmPhoneNumber *number = data;

	g_free(number->number);
	number->number = NULL;

	g_slice_free(RmPhoneNumber, number);
}

/**
 * rm_contact_free:
 * @contact: a #RmContact
 *
 * Frees a #RmContact.
 */
void rm_contact_free(RmContact *contact)
{
	g_free(contact->name);
	g_free(contact->company);
	g_free(contact->number);
	g_free(contact->street);
	g_free(contact->zip);
	g_free(contact->city);
	g_free(contact->image);

	if (contact->addresses) {
		g_slist_free_full(contact->addresses, rm_contact_free_address);
	}
	if (contact->numbers) {
		g_slist_free_full(contact->numbers, rm_contact_free_number);
	}

	g_slice_free(RmContact, contact);
}
