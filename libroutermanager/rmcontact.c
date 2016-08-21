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

#include <glib.h>

#include <libroutermanager/rmcontact.h>
#include <libroutermanager/rmobjectemit.h>
#include <libroutermanager/router.h>

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
		dst->numbers = g_slist_copy(src->numbers);
	} else {
		dst->numbers = NULL;
	}

	if (src->addresses) {
		dst->addresses = g_slist_copy(src->addresses);
	} else {
		dst->addresses = NULL;
	}

	if (src->number) {
		dst->number = g_strdup(src->number);
	}

	if (src->company) {
		dst->company = g_strdup(src->company);
	} else {
		dst->company = NULL;
	}
	if (src->street) {
		dst->street = g_strdup(src->street);
	} else {
		dst->street = NULL;
	}
	if (src->zip) {
		dst->zip = g_strdup(src->zip);
	} else {
		dst->zip = NULL;
	}
	if (src->city) {
		dst->city = g_strdup(src->city);
	} else {
		dst->city = NULL;
	}

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
	int type = -1;

	/** Ask for contact information */
	contact->number = number;
	rm_object_emit_contact_process(contact);

	/* Depending on the number set the active address */
	for (numbers = contact->numbers; numbers != NULL; numbers = numbers->next) {
		struct phone_number *phone_number = numbers->data;

		if (!strcmp(number, phone_number->number)) {
			type = phone_number->type;
			break;
		}
	}

	if (type == -1) {
		return contact;
	}

	switch (type) {
	case PHONE_NUMBER_HOME:
	case PHONE_NUMBER_FAX_HOME:
	case PHONE_NUMBER_MOBILE:
	default:
		type = 0;
		break;
	case PHONE_NUMBER_WORK:
	case PHONE_NUMBER_FAX_WORK:
	case PHONE_NUMBER_PAGER:
		type = 1;
		break;
	}

	for (addresses = contact->addresses; addresses != NULL; addresses = addresses->next) {
		RmContactAddress *contact_address = addresses->data;

		if (contact_address->type == type) {
			contact->street = contact_address->street;
			contact->city = contact_address->city;
			contact->zip = contact_address->zip;
			break;
		}
	}

	return contact;
}

/**
 * rm_contact_free:
 * @contact: a #RmContact
 *
 * Frees a #RmContact.
 */
void rm_contact_free(RmContact *contact)
{
	if (contact->name) {
		g_free(contact->name);
		contact->name = NULL;
	}

	g_slice_free(RmContact, contact);
}
