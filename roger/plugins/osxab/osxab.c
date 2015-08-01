/**
 * Roger Router
 * Copyright (c) 2015 Jan-Michael Brummer
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
#include <fcntl.h>

#include <gtk/gtk.h>

#include <CoreFoundation/CoreFoundation.h>
#include <AddressBook/ABAddressBookC.h>
#include <AddressBook/ABGlobalsC.h>

#include <libroutermanager/plugins.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/address-book.h>
#include <libroutermanager/call.h>
#include <libroutermanager/contact.h>
#include <libroutermanager/router.h>
#include <libroutermanager/file.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/settings.h>

#define ROUTERMANAGER_TYPE_OSXAB_PLUGIN        (routermanager_osxab_plugin_get_type ())
#define ROUTERMANAGER_OSXAB_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_OSXAB_PLUGIN, RouterManagerOSXAbPlugin))

typedef struct {
	guint signal_id;
} RouterManagerOSXAbPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_OSXAB_PLUGIN, RouterManagerOSXAbPlugin, routermanager_osxab_plugin)

static GSList *contacts = NULL;

static char *cstring(CFStringRef s)
{
	if (!s) {
		return NULL;
	}

	CFIndex length = CFStringGetLength(s);
	CFIndex max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
	char *buffer = (char *) malloc(max_size);

	if (CFStringGetCString(s, buffer, max_size, kCFStringEncodingUTF8)) {
			return buffer;
	}

	free(buffer);

	return NULL;
}

/**
 * \brief Read osxab book
 * \return error code
 */
static int osxab_read_book(void) {
	ABAddressBookRef ab = ABGetSharedAddressBook();
	CFArrayRef entries = ABCopyArrayOfAllPeople(ab);
	CFIndex len = CFArrayGetCount(entries);

	for (int i = 0; i < len; i++) {
		ABPersonRef person = (ABPersonRef) CFArrayGetValueAtIndex(entries, i);
		CFTypeRef firstName = ABRecordCopyValue(person, kABFirstNameProperty);
		CFTypeRef lastName = ABRecordCopyValue(person, kABLastNameProperty);
		CFTypeRef company = ABRecordCopyValue(person, kABOrganizationProperty);
		CFTypeRef addresses = ABRecordCopyValue(person, kABAddressProperty);
		CFTypeRef phones = ABRecordCopyValue(person, kABPhoneProperty);
		CFTypeRef uid = ABRecordCopyUniqueId(person);
		CFStringRef lastname_cstr;
		struct contact *contact;

		if (!firstName && !lastName) {
			/* Company... */
			if (!company) {
				continue;
			}
			firstName = company;
		}

		contact = g_slice_new0(struct contact);
		contact->priv = (gpointer)uid;
		g_debug("Uid: %s", cstring(contact->priv));

		lastname_cstr = cstring(lastName);
		contact->name = g_strdup_printf("%s%s%s",
			firstName ? cstring(firstName) : "",
			lastname_cstr ? " " : "",
			lastname_cstr ? lastname_cstr : "");

		if (addresses) {
			for (int j = 0; j < ABMultiValueCount((ABMultiValueRef)addresses); j++) {
				CFDictionaryRef an_address = ABMultiValueCopyValueAtIndex(addresses, j);
				CFStringRef label = ABMultiValueCopyLabelAtIndex(addresses, j);
				CFStringRef street = CFDictionaryGetValue(an_address, kABAddressStreetKey);
				CFStringRef city = CFDictionaryGetValue(an_address, kABAddressCityKey);
				CFStringRef zip = CFDictionaryGetValue(an_address, kABAddressZIPKey);
				struct contact_address *address = g_slice_new0(struct contact_address);
				CFStringRef tmp;

				address->type = CFStringCompare(label, kABHomeLabel, 0);
				
				tmp = cstring(street);
				address->street = tmp ? tmp : g_strdup("");
				tmp = cstring(city);
				address->city = tmp ? tmp : g_strdup("");
				tmp = cstring(zip);
				address->zip = tmp ? tmp : g_strdup("");
				contact->addresses = g_slist_prepend(contact->addresses, address);
			}
		}
            

		if (phones) {
			for (int j = 0; j < ABMultiValueCount((ABMultiValueRef)phones); j++) {
				CFStringRef an_phone = ABMultiValueCopyValueAtIndex(phones, j);
				CFStringRef label = ABMultiValueCopyLabelAtIndex(phones, j);
				struct phone_number *number = g_slice_new0(struct phone_number);

				if (!CFStringCompare(label, kABPhoneHomeLabel, 0)) {
					number->type = PHONE_NUMBER_HOME;
				} else if (!CFStringCompare(label, kABPhoneWorkLabel, 0)) {
					number->type = PHONE_NUMBER_WORK;
				} else if (!CFStringCompare(label, kABPhoneMobileLabel, 0)) {
					number->type = PHONE_NUMBER_MOBILE;
				} else if (!CFStringCompare(label, kABPhoneHomeFAXLabel, 0)) {
					number->type = PHONE_NUMBER_FAX_HOME;
				} else if (!CFStringCompare(label, kABPhoneWorkFAXLabel, 0)) {
					number->type = PHONE_NUMBER_FAX_WORK;
				} else {
					number->type = PHONE_NUMBER_HOME;
				}
				number->number = cstring(an_phone);
				contact->numbers = g_slist_prepend(contact->numbers, number);
			}
        }

		CFDataRef image = ABPersonCopyImageData(person);
		if (image) {
			GdkPixbufLoader *loader;
			gint image_len = CFDataGetLength(image);

			loader = gdk_pixbuf_loader_new();
			if (gdk_pixbuf_loader_write(loader, CFDataGetBytePtr(image), image_len, NULL)) {
					printf("Image loaded (%d)\n", image_len);
					contact->image = gdk_pixbuf_loader_get_pixbuf(loader);
					contact->image_len = image_len;
			} else {
				printf("Image NOT loaded\n");
			}
			gdk_pixbuf_loader_close(loader, NULL);
		}

		contacts = g_slist_insert_sorted(contacts, contact, contact_name_compare);
	}

	return 0;
}

GSList *osxab_get_contacts(void)
{
	GSList *list = contacts;

	return list;
}

gboolean osxab_reload_contacts(void)
{
	contacts = NULL;

	osxab_read_book();

	return TRUE;
}

static gboolean osxab_remove_contact(struct contact *contact)
{
	ABAddressBookRef ab = ABGetSharedAddressBook();
	ABPersonRef ref;
	gboolean ret = FALSE;

	g_debug("Uid: %s", cstring(contact->priv));
	ref = ABCopyRecordForUniqueId(ab, contact->priv);
	
	if (ref) {
		ABRemoveRecord(ab, ref);
		ABSave(ab);
		osxab_reload_contacts();
		ret = TRUE;
	}

	return ret;
}

static gboolean osxab_save_contact(struct contact *contact)
{
	ABAddressBookRef ab = ABGetSharedAddressBook();
	ABPersonRef ref;
	CFStringRef cfstring;
	gchar *first_name = NULL;
	gchar *last_name = NULL;
	gchar **split;

	if (!contact->priv) {
		ref = ABPersonCreate();
		contact->priv = (gpointer) ABRecordCopyUniqueId(ref);
		ABAddRecord(ab, ref);
	} else {
		ref = ABCopyRecordForUniqueId(ab, contact->priv);
	}

	split = g_strsplit(contact->name, " ", 2);
	first_name = split[0];
	last_name = split[1];

	cfstring = CFStringCreateWithCString(NULL, first_name ? first_name : "", kCFStringEncodingUTF8);
	ABRecordSetValue(ref, kABFirstNameProperty, cfstring);

	cfstring = CFStringCreateWithCString(NULL, last_name ? last_name : "", kCFStringEncodingUTF8);
	ABRecordSetValue(ref, kABLastNameProperty, cfstring);

	g_strfreev(split);

	if (contact->numbers) {
		ABMutableMultiValueRef multi_phone = ABMultiValueCreateMutable();
    	GSList *list;
	
		for (list = contact->numbers; list != NULL; list = list->next) {
			struct phone_number *number = list->data;
			CFTypeRef type;

			switch (number->type) {
			case PHONE_NUMBER_HOME:
				type = kABPhoneHomeLabel;
				break;
			case PHONE_NUMBER_WORK:
				type = kABPhoneWorkLabel;
				break;
			case PHONE_NUMBER_MOBILE:
				type = kABPhoneMobileLabel;
				break;
			case PHONE_NUMBER_FAX_HOME:
				type = kABPhoneHomeFAXLabel;
				break;
			case PHONE_NUMBER_FAX_WORK:
				type = kABPhoneWorkFAXLabel;
				break;
			default:
				continue;
			}

			cfstring = CFStringCreateWithCString(NULL, number->number, kCFStringEncodingUTF8);
			ABMultiValueAdd(multi_phone, cfstring, type, NULL);
		}
	
		ABRecordSetValue(ref, kABPhoneProperty, multi_phone);
	}

	if (contact->addresses) {
		ABMutableMultiValueRef multi_addresses = ABMultiValueCreateMutable();
    	GSList *list;
	
		for (list = contact->addresses; list != NULL; list = list->next) {
			CFMutableDictionaryRef dic;
			CFTypeRef type;
			struct contact_address *address = list->data;

			switch (address->type) {
			case 0:
				type = kABHomeLabel;
				break;
			case 1:
				type = kABWorkLabel;
				break;
			default:
				continue;
			}

			dic = CFDictionaryCreateMutable(NULL,  0, NULL, NULL);

			cfstring = CFStringCreateWithCString(NULL, address->street, kCFStringEncodingUTF8);
			CFDictionaryAddValue(dic, kABAddressStreetKey, cfstring);

			cfstring = CFStringCreateWithCString(NULL, address->city, kCFStringEncodingUTF8);
			CFDictionaryAddValue(dic, kABAddressCityKey, cfstring);

			cfstring = CFStringCreateWithCString(NULL, address->zip, kCFStringEncodingUTF8);
			CFDictionaryAddValue(dic, kABAddressZIPKey, cfstring);

			ABMultiValueAdd(multi_addresses, dic, type, NULL);
		}

		ABRecordSetValue(ref, kABAddressProperty, multi_addresses);
	}

	ABSave(ab);
	osxab_reload_contacts();
}

struct address_book osxab_book = {
	osxab_get_contacts,
	osxab_reload_contacts,
	osxab_remove_contact,
	osxab_save_contact
};

void impl_activate(PeasActivatable *plugin)
{
	osxab_read_book();

	routermanager_address_book_register(&osxab_book);
}

void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerOSXAbPlugin *osxab_plugin = ROUTERMANAGER_OSXAB_PLUGIN(plugin);

	if (g_signal_handler_is_connected(G_OBJECT(app_object), osxab_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), osxab_plugin->priv->signal_id);
	}
}
