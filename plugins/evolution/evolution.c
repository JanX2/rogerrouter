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

#include <config.h>

#include <string.h>

#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <rm/rm.h>

#include <roger/main.h>
#include "config.h"

#include <libebook/libebook.h>

#include "ebook-sources.h"

static GSList *contacts = NULL;
static GSettings *ebook_settings = NULL;
static EClient *e_client = NULL;

gboolean evolution_reload(void);

/**
 * \brief Get selected evolution address book Id
 * \return evolution address book
 */
const gchar *get_selected_ebook_id(void)
{
	return g_settings_get_string(ebook_settings, "book");
}

/**
 *\brief free data allocated for an address book data item
 *\param ebook data item to be destroyed
 */
static void free_ebook_data(struct ebook_data *ebook_data)
{
	g_free(ebook_data->name);
	g_free(ebook_data->id);
}

/**
 * \brief free ebook list
 */
void free_ebook_list(GList *ebook_list)
{
	g_list_free_full(ebook_list, (GDestroyNotify)free_ebook_data);
}

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
	gboolean none_selected = RM_EMPTY_STRING(id);

	list = get_ebook_list();
	while (list) {
		struct ebook_data *ebook_data = list->data;

		//g_debug("%s <-> %s", ebook_data->name, id);
		if (none_selected || !strcmp(ebook_data->name, id)) {
			if (none_selected) {
				g_settings_set_string(ebook_settings, "book", ebook_data->name);
			}
			return e_source_registry_ref_source(registry, ebook_data->id);
		}
		list = list->next;
	}

	return NULL;
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
		ESource *e_source = E_SOURCE(source->data);
		ESource *parent_source;

		if (!e_source_get_enabled(e_source)) {
			g_debug("Source %s not enabled... skip it", e_source_get_uid(e_source));
			continue;
		}

		ebook_data = g_slice_new(struct ebook_data);

		parent_source = e_source_registry_ref_source(get_source_registry(), e_source_get_parent(e_source));

		ebook_data->name = g_strdup_printf("%s (%s)", e_source_get_display_name(e_source), e_source_get_display_name(parent_source));
		ebook_data->id = e_source_dup_uid(e_source);

		ebook_list = g_list_append(ebook_list, ebook_data);

		g_object_unref(parent_source);
	}

	g_list_free_full(sources, g_object_unref);

	return ebook_list;
}

void ebook_objects_changed_cb(EBookClientView *view, const GSList *ids, gpointer user_data)
{
	g_debug("%s(): called", __FUNCTION__);

	/* Reload contacts */
	evolution_reload();

	/* Send signal to redraw journal and update contacts view */
	rm_object_emit_contacts_changed();
}

void ebook_read_data(EClient *e_client)
{
	EBookClient *client;
	EBookQuery *query;
	gchar *sexp = NULL;
	EContact *e_contact;
	EContactPhoto *photo;
	GdkPixbufLoader *loader;
	GSList *list;
	GSList *ebook_contacts;
	GError *error = NULL;

	/* TODO: Free list */
	contacts = NULL;

	if (!e_client) {
		g_debug("no callback!!!! (Error: %s)", error ? error->message : "?");
		return;
	}

	client = E_BOOK_CLIENT(e_client);

	query = e_book_query_any_field_contains("");
	if (!query) {
		g_warning("Couldn't create query.");
		return;
	}
	sexp = e_book_query_to_string(query);

	EBookClientView *view;

	if (!e_book_client_get_view_sync(client, sexp, &view, NULL, &error)) {
		g_error("get_view_sync");
	}

	g_signal_connect(view, "objects-added", G_CALLBACK(ebook_objects_changed_cb), NULL);
	g_signal_connect(view, "objects-removed", G_CALLBACK(ebook_objects_changed_cb), NULL);
	g_signal_connect(view, "objects-modified", G_CALLBACK(ebook_objects_changed_cb), NULL);
	e_book_client_view_set_fields_of_interest(view, NULL, &error);
	if (error) {
		g_error("set_fields_of_interest()");
	}

	e_book_client_view_set_flags(view, 0, &error);
	if (error) {
		g_error("set_flags()");
	}

	e_book_client_view_start(view, &error);

	if (!e_book_client_get_contacts_sync(client, sexp, &ebook_contacts, NULL, NULL)) {
		g_warning("Couldn't get query results.");
		e_book_query_unref(query);
		g_object_unref(client);
		return;
	}
	g_free(sexp);

	e_book_query_unref(query);

	if (!ebook_contacts) {
		g_debug("%s(): No contacts in book", __FUNCTION__);
		return;
	}
	for (list = ebook_contacts; list != NULL; list = list->next) {
		const gchar *display_name;
		RmContact *contact;
		RmPhoneNumber *phone_number;
		const gchar *number;
		const gchar *company;
		EContactAddress *address;

		g_return_if_fail(E_IS_CONTACT(list->data));
		e_contact = E_CONTACT(list->data);

		display_name = e_contact_get_const(e_contact, E_CONTACT_FULL_NAME);

		if (RM_EMPTY_STRING(display_name)) {
			continue;
		}

		contact = g_slice_new0(RmContact);

		contact->priv = (gpointer)e_contact_get_const(e_contact, E_CONTACT_UID);

		photo = e_contact_get(e_contact, E_CONTACT_PHOTO);
		if (photo) {
			GdkPixbuf *buf = NULL;

			switch (photo->type) {
			case E_CONTACT_PHOTO_TYPE_INLINED:
				loader = gdk_pixbuf_loader_new();

				if (gdk_pixbuf_loader_write(loader, photo->data.inlined.data, photo->data.inlined.length, NULL)) {
					gdk_pixbuf_loader_close(loader, NULL);
					buf = gdk_pixbuf_loader_get_pixbuf(loader);
				} else {
					g_debug("Could not load inlined photo!");
				}
				break;
			case E_CONTACT_PHOTO_TYPE_URI: {
				GRegex *pro = g_regex_new("%25", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);

				if (!strncmp(photo->data.uri, "file://", 7)) {
					gchar *uri = g_regex_replace_literal(pro, photo->data.uri + 7, -1, 0, "%", 0, NULL);
					buf = gdk_pixbuf_new_from_file(uri, NULL);
				} else {
					g_debug("Cannot handle URI: '%s'!", photo->data.uri);
				}
				g_regex_unref(pro);
				break;
			}
			default:
				g_debug("Unhandled photo type: %d", photo->type);
				break;
			}

			contact->image = buf;

			e_contact_photo_free(photo);
		} else {
			contact->image = NULL;
		}

		contact->name = g_strdup(display_name);
		contact->numbers = NULL;

		number = e_contact_get_const(e_contact, E_CONTACT_PHONE_HOME);
		if (!RM_EMPTY_STRING(number)) {
			phone_number = g_slice_new(RmPhoneNumber);
			phone_number->type = RM_PHONE_NUMBER_TYPE_HOME;
			phone_number->number = rm_number_full(number, FALSE);
			contact->numbers = g_slist_prepend(contact->numbers, phone_number);
		}

		number = e_contact_get_const(e_contact, E_CONTACT_PHONE_BUSINESS);
		if (!RM_EMPTY_STRING(number)) {
			phone_number = g_slice_new(RmPhoneNumber);
			phone_number->type = RM_PHONE_NUMBER_TYPE_WORK;
			phone_number->number = rm_number_full(number, FALSE);
			contact->numbers = g_slist_prepend(contact->numbers, phone_number);
		}

		number = e_contact_get_const(e_contact, E_CONTACT_PHONE_MOBILE);
		if (!RM_EMPTY_STRING(number)) {
			phone_number = g_slice_new(RmPhoneNumber);
			phone_number->type = RM_PHONE_NUMBER_TYPE_MOBILE;
			phone_number->number = rm_number_full(number, FALSE);
			contact->numbers = g_slist_prepend(contact->numbers, phone_number);
		}

		number = e_contact_get_const(e_contact, E_CONTACT_PHONE_HOME_FAX);
		if (!RM_EMPTY_STRING(number)) {
			phone_number = g_slice_new(RmPhoneNumber);
			phone_number->type = RM_PHONE_NUMBER_TYPE_FAX_HOME;
			phone_number->number = rm_number_full(number, FALSE);
			contact->numbers = g_slist_prepend(contact->numbers, phone_number);
		}

		number = e_contact_get_const(e_contact, E_CONTACT_PHONE_BUSINESS_FAX);
		if (!RM_EMPTY_STRING(number)) {
			phone_number = g_slice_new(RmPhoneNumber);
			phone_number->type = RM_PHONE_NUMBER_TYPE_FAX_WORK;
			phone_number->number = rm_number_full(number, FALSE);
			contact->numbers = g_slist_prepend(contact->numbers, phone_number);
		}

		company = e_contact_get_const(e_contact, E_CONTACT_ORG);
		if (!RM_EMPTY_STRING(company)) {
			contact->company = g_strdup(company);
		}

		address = e_contact_get(e_contact, E_CONTACT_ADDRESS_HOME);
		if (address) {
			RmContactAddress *c_address = g_slice_new(RmContactAddress);
			c_address->type = 0;
			c_address->street = g_strdup(address->street);
			c_address->zip = g_strdup(address->code);
			c_address->city = g_strdup(address->locality);
			contact->addresses = g_slist_prepend(contact->addresses, c_address);
		}

		address = e_contact_get(e_contact, E_CONTACT_ADDRESS_WORK);
		if (address) {
			RmContactAddress *c_address = g_slice_new(RmContactAddress);
			c_address->type = 1;
			c_address->street = g_strdup(address->street);
			c_address->zip = g_strdup(address->code);
			c_address->city = g_strdup(address->locality);
			contact->addresses = g_slist_prepend(contact->addresses, c_address);
		}

		contacts = g_slist_insert_sorted(contacts, contact, rm_contact_name_compare);
	}

	g_slist_free(ebook_contacts);

	/* Send signal to redraw journal and update contacts view */
	g_debug("%s(): ********************************* loaded", __FUNCTION__);
	rm_object_emit_contacts_changed();
}

static void ebook_read_cb(GObject *source, GAsyncResult *res, gpointer user_data)
{
	GError *error = NULL;

	e_client = e_book_client_connect_finish(res, &error);
	if (!e_client) {
		g_warning("Error finishing client connection. Error: %s", error ?  error->message : "?");
	} else {
		ebook_read_data(e_client);
	}
}

/**
 * \brief Read ebook list (async)
 * \return error code
 */
gboolean ebook_read_book(void)
{
	ESource *source = get_selected_ebook_esource();

	if (!source) {
		g_debug("Book could not be found....");
		return FALSE;
	}

	e_book_client_connect(source,
#if EDS_CHECK_VERSION(3, 16, 0)
			      5,
#endif
			      NULL, ebook_read_cb, NULL);

	return TRUE;
}

/**
 * \brief Read ebook list (sync)
 * \return error code
 */
gboolean ebook_read_book_sync(void)
{
	EClient *client;
	ESource *source = get_selected_ebook_esource();

	if (!source) {
		g_debug("Book could not be found....");
		return FALSE;
	}

	client = e_book_client_connect_sync(source,
#if EDS_CHECK_VERSION(3, 16, 0)
					    5,
#endif
					    NULL, NULL);
	if (client) {
		ebook_read_data(client);
	}

	return TRUE;
}

GSList *evolution_get_contacts(void)
{
	GSList *list = contacts;

	return list;
}

gboolean evolution_reload(void)
{
	ebook_read_book_sync();

	return TRUE;
}

gboolean evolution_remove_contact(RmContact *contact)
{
	EBookClient *client;
	gboolean ret = FALSE;

	if (!e_client) {
		return FALSE;
	}

	client = E_BOOK_CLIENT(e_client);

	ret = e_book_client_remove_contact_by_uid_sync(client, contact->priv, NULL, NULL);
	if (ret) {
		ebook_read_book_sync();
	}

	return ret;
}

static void evolution_set_image(EContact *e_contact, RmContact *contact)
{
	if (contact->image != NULL) {
		EContactPhoto photo;
		GError *error = NULL;

		if (gdk_pixbuf_save_to_buffer(contact->image, (gchar**)&photo.data.inlined.data, &photo.data.inlined.length, "png", &error, NULL)) {
			photo.type = E_CONTACT_PHOTO_TYPE_INLINED;
			photo.data.inlined.mime_type = NULL;
			e_contact_set(e_contact, E_CONTACT_PHOTO, &photo);
		} else {
			g_warning("%s(): gdk_pixbuf_save_to_buffer failed (%s)", __FUNCTION__, error? error->message : "");
		}
	} else if (contact->image == NULL) {
		e_contact_set(e_contact, E_CONTACT_PHOTO, NULL);
	}
}

gboolean evolution_save_contact(RmContact *contact)
{
	EBookClient *client;
	EContact *e_contact;
	gboolean ret = FALSE;
	GError *error = NULL;
	GSList *numbers;
	GSList *addresses;

	if (!e_client) {
		return FALSE;
	}

	client = E_BOOK_CLIENT(e_client);

	if (!contact->priv) {
		e_contact = e_contact_new();
	} else {
		ret = e_book_client_get_contact_sync(client, contact->priv, &e_contact, NULL, &error);
		if (!ret) {
			return FALSE;
		}

		/* Remove entries we can fill */
		e_contact_set(e_contact, E_CONTACT_PHONE_HOME, "");
		e_contact_set(e_contact, E_CONTACT_PHONE_BUSINESS, "");
		e_contact_set(e_contact, E_CONTACT_PHONE_MOBILE, "");
		e_contact_set(e_contact, E_CONTACT_PHONE_HOME_FAX, "");
		e_contact_set(e_contact, E_CONTACT_PHONE_BUSINESS_FAX, "");

		e_contact_set(e_contact, E_CONTACT_ADDRESS_HOME, NULL);
		e_contact_set(e_contact, E_CONTACT_ADDRESS_WORK, NULL);
	}

	e_contact_set(e_contact, E_CONTACT_FULL_NAME, contact->name ? contact->name : "");

	for (numbers = contact->numbers; numbers != NULL; numbers = numbers->next) {
		RmPhoneNumber *number = numbers->data;
		gint type;

		switch (number->type) {
		case RM_PHONE_NUMBER_TYPE_HOME:
			type = E_CONTACT_PHONE_HOME;
			break;
		case RM_PHONE_NUMBER_TYPE_WORK:
			type = E_CONTACT_PHONE_BUSINESS;
			break;
		case RM_PHONE_NUMBER_TYPE_MOBILE:
			type = E_CONTACT_PHONE_MOBILE;
			break;
		case RM_PHONE_NUMBER_TYPE_FAX_HOME:
			type = E_CONTACT_PHONE_HOME_FAX;
			break;
		case RM_PHONE_NUMBER_TYPE_FAX_WORK:
			type = E_CONTACT_PHONE_BUSINESS_FAX;
			break;
		default:
			continue;
		}
		e_contact_set(e_contact, type, number->number);
	}

	for (addresses = contact->addresses; addresses != NULL; addresses = addresses->next) {
		RmContactAddress *address = addresses->data;
		EContactAddress e_address;
		EContactAddress *ep_address = &e_address;
		gint type;

		memset(ep_address, 0, sizeof(EContactAddress));

		switch (address->type) {
		case 0:
			type = E_CONTACT_ADDRESS_HOME;
			break;
		case 1:
			type = E_CONTACT_ADDRESS_WORK;
			break;
		default:
			continue;
		}
		ep_address->street = address->street;
		ep_address->locality = address->city;
		ep_address->code = address->zip;
		e_contact_set(e_contact, type, ep_address);
	}

	evolution_set_image(e_contact, contact);

	if (!contact->priv) {
		ret = e_book_client_add_contact_sync(client, e_contact, NULL, NULL, &error);
	} else {
		ret = e_book_client_modify_contact_sync(client, e_contact, NULL, &error);
	}

	if (!ret && error) {
		g_debug("Error saving contact. '%s'", error->message);
	}

	g_object_unref(client);

	if (ret) {
		ebook_read_book_sync();
	}

	return ret;
}

gchar *evolution_get_active_book_name(void)
{
	return g_strdup(get_selected_ebook_id());
}

gchar **evolution_get_sub_books(void)
{
	GList *source, *sources;
	gchar **ret = NULL;

	sources = get_ebook_list();

	for (source = sources; source != NULL; source = source->next) {
		struct ebook_data *ebook_data = source->data;

		ret = rm_strv_add(ret, ebook_data->name);
	}

	return ret;
}

gboolean evolution_set_sub_book(gchar *name)
{
	g_settings_set_string(ebook_settings, "book", name);

	contacts = NULL;
	ebook_read_book();

	return TRUE;
}

RmAddressBook evolution_book = {
	"Evolution",
	evolution_get_active_book_name,
	evolution_get_contacts,
	evolution_remove_contact,
	evolution_save_contact,
	evolution_get_sub_books,
	evolution_set_sub_book
};

gboolean evolution_plugin_init(RmPlugin *plugin)
{
	ebook_settings = rm_settings_new_profile("org.tabos.roger.plugins.evolution", "evolution", (gchar*)rm_profile_get_name(rm_profile_get_active()));

	ebook_read_book();

	rm_addressbook_register(&evolution_book);

	return TRUE;
}

gboolean evolution_plugin_shutdown(RmPlugin *plugin)
{
	rm_addressbook_unregister(&evolution_book);
	g_clear_object(&ebook_settings);

	return TRUE;
}

RM_PLUGIN(evolution);
