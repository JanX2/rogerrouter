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

#include <gtk/gtk.h>

#include <libroutermanager/plugins.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/address-book.h>
#include <libroutermanager/call.h>
#include <libroutermanager/router.h>
#include <libroutermanager/settings.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/osdep.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/password.h>

#include <roger/main.h>
#include <roger/uitools.h>

#include <gdata/gdata.h>
#include <gdata/gdata-oauth1-authorizer.h>

#define ROUTERMANAGER_TYPE_GOOGLE_PLUGIN        (routermanager_google_plugin_get_type ())
#define ROUTERMANAGER_GOOGLE_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_GOOGLE_PLUGIN, RouterManagerGooglePlugin))

typedef struct {
	guint signal_id;
} RouterManagerGooglePluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER_CONFIGURABLE(ROUTERMANAGER_TYPE_GOOGLE_PLUGIN, RouterManagerGooglePlugin, routermanager_google_plugin)

void pref_notebook_add_page(GtkWidget *notebook, GtkWidget *page, gchar *title);
GtkWidget *pref_group_create(GtkWidget *box, gchar *title_str, gboolean hexpand, gboolean vexpand);

static GSList *contacts = NULL;
static GSettings *google_settings = NULL;
static GHashTable *table = NULL;

static GDataContactsService *service = NULL;

#define CLIENT_1 "INvhaCyy2yjRRlm8qA4AAzj+3sKQBup+kAPCVpet6GqnZ42aR9W64a0ZzTGjghMX2MCIWKUxmLF0jsgNL6xPjJx5jYraNaMl"
#define CLIENT_2 "TkrQbFBaKMvdX//D/SIjQNdo4ayrfhy7"
#define CLIENT_3 "bEH2rKXuGLrrZq3usMeC4dVwSdIywc8n8g=="

static GDataOAuth2Authorizer *authorizer = NULL;

struct auth_code_data {
	const gchar *auth_uri;
	GtkWidget *entry;
};

static void auth_uri_link_button_clicked_cb(GtkButton *button, gpointer data)
{
	struct auth_code_data *auth_code_data = data;

	os_execute(auth_code_data->auth_uri);

	gtk_widget_grab_focus(auth_code_data->entry);
}

static void auth_code_entry_changed_cb(GtkEditable *entry, gpointer data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(data), gtk_entry_get_text_length(GTK_ENTRY(entry)) > 0);
}

gchar *ask_user_for_auth_code(const gchar *auth_uri)
{
	GtkWidget *dialog;
	GtkWidget *grid;
	GtkWidget *link_button;
	GtkWidget *label;
	GtkWidget *entry;
	gchar *str;
	gchar *retval = NULL;
	gint dlg_res;
	GtkWidget *btn_ok;
	struct auth_code_data *auth_code_data;

	dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, "<span weight=\"bold\" size=\"larger\">%s</span>", _("Google plugin: Authorization required"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
		_("You need to authorize Roger Router to access your Google contact list."
		"\n\nVisit Google's authorization page by pressing the button below. After you "
		"confirmed the authorization, you will get an authorization code. Enter that code "
		"in the field below to grant Roger Router access to your Google contact list."));

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Cancel"), GTK_RESPONSE_CANCEL);
	btn_ok = gtk_dialog_add_button(GTK_DIALOG(dialog), _("_OK"), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

	gtk_widget_set_sensitive(btn_ok, FALSE);

	grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

	str = g_strconcat("<b>", _("Step 1:"), "</b>", NULL);
	label = gtk_label_new(str);
	g_free(str);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

	link_button = gtk_button_new_with_label(_("Click here to open the Google authorization page in a browser"));
	auth_code_data = g_new0(struct auth_code_data, 1);
	gtk_grid_attach(GTK_GRID(grid), link_button, 1, 0, 2, 1);

	str = g_strconcat("<b>", _("Step 2:"), "</b>", NULL);
	label = gtk_label_new(str);
	g_free(str);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

	gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("Enter code:")), 1, 1, 1, 1);

	entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(entry), "changed", (GCallback)auth_code_entry_changed_cb, (gpointer)btn_ok);
	gtk_widget_set_hexpand(entry, TRUE);
	gtk_grid_attach(GTK_GRID(grid), entry, 2, 1, 1, 1);

	auth_code_data->auth_uri = auth_uri;
	auth_code_data->entry = entry;
	g_signal_connect(G_OBJECT(link_button), "clicked", (GCallback)auth_uri_link_button_clicked_cb, (gpointer)auth_code_data);

	gtk_box_pack_start(GTK_BOX(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog))), grid, FALSE, FALSE, 0);

	gtk_widget_show_all(dialog);

	dlg_res = gtk_dialog_run(GTK_DIALOG(dialog));

	switch (dlg_res) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		break;
	case GTK_RESPONSE_OK:
		retval = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		break;
	}

	g_free(auth_code_data);
	gtk_widget_destroy(dialog);

	return retval;
}

static void google_interactive_auth(void)
{
	gchar *authentication_uri, *authorization_code;

	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri(authorizer, NULL, FALSE);

	/* (Present the page at the authentication URI to the user, either in an embedded or stand-alone web browser, and
	 * ask them to grant access to the application and return the code Google gives them.)
	 */
	authorization_code = ask_user_for_auth_code(authentication_uri);

	gdata_oauth2_authorizer_request_authorization(authorizer, authorization_code, NULL, NULL);

	g_free(authentication_uri);

	/* Zero out the code before freeing. */
	if (authorization_code != NULL) {
		memset(authorization_code, 0, strlen (authorization_code));
	}

	g_free(authorization_code);
}

/**
 * \brief Initialize gdata and authenticate user
 * \return error code, 0 on success otherwise error
 */
static int google_init(void)
{
	gchar *user;
	gchar *pwd;
	gchar *c1;
	gchar *c2;
	gchar *c3;

	if (!authorizer) {
		user = g_settings_get_string(google_settings, "user");
		pwd = g_settings_get_string(google_settings, "password");

		if (EMPTY_STRING(user) || EMPTY_STRING(pwd)) {
			return 0;
		}

		c1 = (gchar*)password_decode(CLIENT_1);
		c2 = (gchar*)password_decode(CLIENT_2);
		c3 = (gchar*)password_decode(CLIENT_3);
		authorizer = gdata_oauth2_authorizer_new(c1, c2, c3, GDATA_TYPE_CONTACTS_SERVICE);
		g_free(c3);
		g_free(c2);
		g_free(c1);

		if (authorizer == NULL) {
			g_debug("Could not create authorizer");
			return -1;
		}
	}

	if (!service) {
		service = gdata_contacts_service_new(GDATA_AUTHORIZER(authorizer));
		if (service == NULL) {
			g_debug("Could not contacts service");
			g_object_unref(authorizer);
	  		return -2;
		}
	}

	if (!gdata_service_is_authorized(GDATA_SERVICE(service))) {
		/* Try to use refresh token */
		gchar *token = g_settings_get_string(google_settings, "token");

		if (!EMPTY_STRING(token)) {
			GError *error = NULL;
			g_debug("%s(): Trying to refresh authorization", __FUNCTION__);

			gdata_oauth2_authorizer_set_refresh_token(authorizer, token);
			if (!gdata_authorizer_refresh_authorization(GDATA_AUTHORIZER(authorizer), NULL, &error)) {
				if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
					g_debug("%s(): Authorization refresh error: %s", __FUNCTION__, error->message);
					emit_message("Google Plugin", _("Authorization refresh error"));
				}

				g_error_free(error);

				google_interactive_auth();
			} else {
				g_debug("%s(): Authorization refresh successful", __FUNCTION__);
			}
		} else {
			google_interactive_auth();
		}
	}

	return 0;
}

/**
 * \brief Deinitialize gdata
 * \return error code
 */
static int google_shutdown(void) {
	gchar *token;

	if (service != NULL) {
		g_object_unref(service);
		service = NULL;
	}

	token = gdata_oauth2_authorizer_dup_refresh_token(authorizer);
	g_settings_set_string(google_settings, "token", token ? token : "");

	if (authorizer != NULL) {
		g_object_unref(authorizer);
		authorizer = NULL;
	}

	return 0;
}

/**
 * \brief Read address book
 * \return error code
 */
static int google_read_book(void) {
	GDataQuery *query;
	GDataFeed *feed;
	GList *list;
	GError *error = NULL;
	int chunk_size = 16;
	int result = 0;

	if (google_init()) {
		g_warning("google_init() failed!");
		return -1;
	}

	query = GDATA_QUERY(gdata_contacts_query_new_with_limits(NULL, 1, chunk_size));
	if (query == NULL) {
		g_warning("Contact query failed");
		google_shutdown();
		return -2;
	}

	do {
		feed = gdata_contacts_service_query_contacts(service, query, NULL, NULL, NULL, &error);
		if (feed == NULL) {
			g_warning("Query contacts failed");
			if (error != NULL) {
				g_warning("Error: %s", error->message);
				g_error_free(error);
			}

			return -1;
		}

		list = gdata_feed_get_entries(feed);
		result = list ? g_list_length(list) : 0;

		for (list = gdata_feed_get_entries(feed); list != NULL; list = list->next) {
			GDataContactsContact *gcontact;
			GDataGDName *name = NULL;
			GList *number_list = NULL;
			GList *address_list = NULL;
			const gchar *id;
			GHashTable *table;
			guint8 *photo;
			gsize photo_len;
			gchar *photo_type;
			struct contact *contact;

			if (list->data == NULL) {
				g_warning("Strange... data == NULL");
				continue;
			}

			gcontact = GDATA_CONTACTS_CONTACT(list->data);
			if (gcontact == NULL) {
				g_warning("gcontact == NULL");
				return -2;
			}

			id = gdata_entry_get_id(GDATA_ENTRY(list->data));
			if (id == NULL) {
				g_warning("id == NULL");
				return -2;
			}

			table = g_hash_table_new(NULL, NULL);
			if (table == NULL) {
				g_warning("table == NULL");
				return -2;
			}

			name = gdata_contacts_contact_get_name(gcontact);
			if (!name) {
				g_debug("name is NULL");
				continue;
			}

			contact = g_slice_new0(struct contact);
			contact->priv = g_strdup(id);

			contact->name = g_strdup(gdata_gd_name_get_full_name(name));
			if (contact->name == NULL) {
				g_debug("contact name is NULL");
				continue;
			}

			number_list = gdata_contacts_contact_get_phone_numbers(gcontact);
			while (number_list != NULL && number_list->data != NULL) {
				GDataGDPhoneNumber *number = number_list->data;
				const gchar *type = gdata_gd_phone_number_get_relation_type(number);
				const gchar *num = gdata_gd_phone_number_get_number(number);
				struct phone_number *phone_number;

				if (type == NULL) {
					g_warning("type == NULL");
					break;
				}
				if (num == NULL) {
					g_warning("num == NULL");
					break;
				}

				phone_number = g_slice_new(struct phone_number);
				if (strcmp(type, GDATA_GD_PHONE_NUMBER_WORK) == 0) {
					phone_number->type = PHONE_NUMBER_WORK;
				} else if (strcmp(type, GDATA_GD_PHONE_NUMBER_HOME) == 0) {
					phone_number->type = PHONE_NUMBER_HOME;
				} else if (strcmp(type, GDATA_GD_PHONE_NUMBER_MOBILE) == 0) {
					phone_number->type = PHONE_NUMBER_MOBILE;
				} else if (strcmp(type, GDATA_GD_PHONE_NUMBER_HOME_FAX) == 0) {
					phone_number->type = PHONE_NUMBER_FAX_HOME;
				} else if (strcmp(type, GDATA_GD_PHONE_NUMBER_WORK_FAX) == 0) {
					phone_number->type = PHONE_NUMBER_FAX_WORK;
				}
				phone_number->number = call_full_number(num, FALSE);
				contact->numbers = g_slist_prepend(contact->numbers, phone_number);

				number_list = number_list->next;
			}

			address_list = gdata_contacts_contact_get_postal_addresses(gcontact);
			while (address_list != NULL && address_list->data != NULL) {
				GDataGDPostalAddress *gaddress = address_list->data;
				const gchar *type = gdata_gd_postal_address_get_relation_type(gaddress);
				struct contact_address *address;

				if (type == NULL) {
					g_warning("type == NULL");
					break;
				}

				address = g_slice_new0(struct contact_address);
				if (!strcmp(type, GDATA_GD_POSTAL_ADDRESS_WORK)) {
					address->type = 1;
				} else {
					address->type = 0;
				}

				const gchar *tmp = gdata_gd_postal_address_get_street(gaddress);
				address->street = g_strdup(tmp ? tmp : "");
				tmp = gdata_gd_postal_address_get_city(gaddress);
				address->city = g_strdup(tmp ? tmp : "");
				//address->country = g_strdup(gdata_gd_postal_address_get_country(gaddress));
				tmp = gdata_gd_postal_address_get_postcode(gaddress);
				address->zip = g_strdup(tmp ? tmp : "");

				contact->addresses = g_slist_prepend(contact->addresses, address);

				address_list = address_list->next;
			}

			photo = gdata_contacts_contact_get_photo(gcontact, service, &photo_len, &photo_type, NULL, NULL);
			if (photo != NULL) {
				GdkPixbufLoader *loader;

				loader = gdk_pixbuf_loader_new();
				if (gdk_pixbuf_loader_write(loader, photo, photo_len, NULL)) {
					contact->image = gdk_pixbuf_loader_get_pixbuf(loader);
					contact->image_len = photo_len;
				}
				gdk_pixbuf_loader_close(loader, NULL);
			}

			contacts = g_slist_insert_sorted(contacts, contact, contact_name_compare);
		}

		gdata_query_next_page(GDATA_QUERY(query));
	} while (result == chunk_size);

	g_debug("done");
	return 0;
}

#if 0
void google_set_image(GDataContactsContact *gcontact, struct sPerson *contact) {
	if (contact->pnNewImage != NULL) {
		gchar *pnData;
		gsize nLength;

		if (g_file_get_contents(contact->pnNewImage, &pnData, &nLength, NULL)) {
			gdata_contacts_contact_set_photo(gcontact, service, (guint8 *) pnData, nLength, "image/jpeg", NULL, NULL);
		}
	} else if (contact->psImage == NULL) {
		gdata_contacts_contact_set_photo(gcontact, service, NULL, 0, NULL, NULL, NULL);
	}
}

/**
 * \brief Save address book
 * \return error code
 */
int googleSaveBook(void) {
	GList *list;
	GError *error = NULL;
	struct profile *profile = profile_get_active();

	g_debug("started");
	if (google_init()) {
		g_debug("google_init() failed!");
		return -1;
	}

	for (list = contactsList; list != NULL && list->data != NULL; list = list->next) {
		struct sPerson *contact = list->data;

		if (contact->nFlags == PERSON_FLAG_UNCHANGED) {
			continue;
		}

		if (contact->nFlags & PERSON_FLAG_NEW) {
			GDataGDName *name;
			GDataGDPhoneNumber *psPhone;
			GDataContactsContact *contact;
			GDataGDPostalAddress *psAddress;
			GDataGDOrganization *organization;
			gchar *idUrl = NULL;

			g_debug("Adding new person");

			/* Create new contact */
			contact = gdata_contacts_contact_new(NULL);
			g_assert(GDATA_IS_CONTACTS_CONTACT(contact));

			/* Add group id */
			idUrl = g_strdup_printf("http://www.google.com/m8/feeds/groups/%s/base/6", googleGetUser(profile));
			gdata_contacts_contact_add_group(contact, idUrl);
			g_free(idUrl);

			/* Set display name */
			gdata_entry_set_title(GDATA_ENTRY(contact), contact->pnDisplayName);

			/* Set name */
			name = gdata_gd_name_new(contact->pnFirstName, contact->pnLastName);
			if (contact->pnTitle != NULL && strlen(contact->pnTitle) > 0) {
				gdata_gd_name_set_prefix(name, contact->pnTitle);
			}
			gdata_contacts_contact_set_name(contact, name);
			g_object_unref(name);

			/* Set company */
			if (contact->pnCompany != NULL) {
				organization = gdata_gd_organization_new(contact->pnCompany, NULL, NULL, NULL, TRUE);

				gdata_contacts_contact_add_organization(contact, organization);

				g_object_unref(organization);
			}

			/* Add private phone */
			if (contact->pnPrivatePhone != NULL && strlen(contact->pnPrivatePhone) > 0) {
				psPhone = gdata_gd_phone_number_new(contact->pnPrivatePhone, GDATA_GD_PHONE_NUMBER_HOME, NULL, NULL, TRUE);
				gdata_contacts_contact_add_phone_number(contact, psPhone);
				g_object_unref(psPhone);
			}

			/* Add business phone */
			if (contact->pnBusinessPhone != NULL && strlen(contact->pnBusinessPhone) > 0) {
				psPhone = gdata_gd_phone_number_new(contact->pnBusinessPhone, GDATA_GD_PHONE_NUMBER_WORK, NULL, NULL, TRUE);
				gdata_contacts_contact_add_phone_number(contact, psPhone);
				g_object_unref(psPhone);
			}

			/* Add private mobile */
			if (contact->pnPrivateMobile != NULL && strlen(contact->pnPrivateMobile) > 0) {
				psPhone = gdata_gd_phone_number_new(contact->pnPrivateMobile, GDATA_GD_PHONE_NUMBER_MOBILE, NULL, NULL, TRUE);
				gdata_contacts_contact_add_phone_number(contact, psPhone);
				g_object_unref(psPhone);
			}

			/* Add private fax */
			if (contact->pnPrivateFax != NULL && strlen(contact->pnPrivateFax) > 0) {
				psPhone = gdata_gd_phone_number_new(contact->pnPrivateFax, GDATA_GD_PHONE_NUMBER_HOME_FAX, NULL, NULL, TRUE);
				gdata_contacts_contact_add_phone_number(contact, psPhone);
				g_object_unref(psPhone);
			}

			/* Add business fax */
			if (contact->pnBusinessFax != NULL && strlen(contact->pnBusinessFax) > 0) {
				psPhone = gdata_gd_phone_number_new(contact->pnBusinessFax, GDATA_GD_PHONE_NUMBER_WORK_FAX, NULL, NULL, TRUE);
				gdata_contacts_contact_add_phone_number(contact, psPhone);
				g_object_unref(psPhone);
			}

			/* Add business address */
			psAddress = gdata_gd_postal_address_new(GDATA_GD_POSTAL_ADDRESS_WORK, NULL, TRUE);
			if (psAddress != NULL) {
				if (contact->pnBusinessStreet != NULL && strlen(contact->pnBusinessStreet) > 0) {
					gdata_gd_postal_address_set_street(psAddress, contact->pnBusinessStreet);
				}
				if (contact->pnBusinessCity != NULL && strlen(contact->pnBusinessCity) > 0) {
					gdata_gd_postal_address_set_city(psAddress, contact->pnBusinessCity);
				}
				if (contact->pnBusinessCountry != NULL && strlen(contact->pnBusinessCountry) > 0) {
					gdata_gd_postal_address_set_country(psAddress, contact->pnBusinessCountry, NULL);
				}
				if (contact->pnBusinessZipCode != NULL && strlen(contact->pnBusinessZipCode) > 0) {
					gdata_gd_postal_address_set_postcode(psAddress, contact->pnBusinessZipCode);
				}

				gdata_contacts_contact_add_postal_address(contact, psAddress);

				g_object_unref(psAddress);
			}

			/* Add private address */
			psAddress = gdata_gd_postal_address_new(GDATA_GD_POSTAL_ADDRESS_HOME, NULL, TRUE);
			if (psAddress != NULL) {
				if (contact->pnPrivateStreet != NULL && strlen(contact->pnPrivateStreet) > 0) {
					gdata_gd_postal_address_set_street(psAddress, contact->pnPrivateStreet);
				}
				if (contact->pnPrivateCity != NULL && strlen(contact->pnPrivateCity) > 0) {
					gdata_gd_postal_address_set_city(psAddress, contact->pnPrivateCity);
				}
				if (contact->pnPrivateCountry != NULL && strlen(contact->pnPrivateCountry) > 0) {
					gdata_gd_postal_address_set_country(psAddress, contact->pnPrivateCountry, NULL);
				}
				if (contact->pnPrivateZipCode != NULL && strlen(contact->pnPrivateZipCode) > 0) {
					gdata_gd_postal_address_set_postcode(psAddress, contact->pnPrivateZipCode);
				}

				gdata_contacts_contact_add_postal_address(contact, psAddress);

				g_object_unref(psAddress);
			}

			/* Insert contact */
			contact = gdata_contacts_service_insert_contact(GDATA_CONTACTS_SERVICE(service), contact, NULL, &error);

			/* Now add image */
			google_set_image(contact, contact);

			/* Update person id */
			contact->id = (gchar *) gdata_entry_get_id(GDATA_ENTRY(contact));
		}

		if (contact->nFlags & PERSON_FLAG_DELETED) {
			GDataEntry *psEntry;

			/* Query contact */
			psEntry = gdata_service_query_single_entry(GDATA_SERVICE(service), gdata_contacts_service_get_primary_authorization_domain(), contact->id, NULL, GDATA_TYPE_CONTACTS_CONTACT, NULL, NULL);
			if (psEntry != NULL) {
				/* Delete valid entry */
				gdata_service_delete_entry(GDATA_SERVICE(service), gdata_contacts_service_get_primary_authorization_domain(), psEntry, NULL, NULL);

				g_object_unref(psEntry);
			}
		}

		if (contact->nFlags & PERSON_FLAG_CHANGED) {
			GDataEntry *psEntry;
			GDataContactsContact *psNewContact;
			GDataContactsContact *contact;
			GDataGDName *name;
			GDataGDPostalAddress *psAddress;
			GDataGDOrganization *organization;
			GList *number_list;
			GError *error = NULL;
			gboolean bBusinessPhone = FALSE;
			gboolean bBusinessFax = FALSE;
			gboolean bPrivateFax = FALSE;
			gboolean bPrivatePhone = FALSE;
			gboolean bPrivateMobile = FALSE;

			g_debug("Changed entry");

			/* Query contact */
			psEntry = gdata_service_query_single_entry(GDATA_SERVICE(service), gdata_contacts_service_get_primary_authorization_domain(), contact->id, NULL, GDATA_TYPE_CONTACTS_CONTACT, NULL, NULL);
			if (psEntry == NULL) {
				g_debug("psEntry is NULL");
				continue;
			}
			g_assert(GDATA_IS_CONTACTS_CONTACT(psEntry));

			/* Convert to contact */
			contact = GDATA_CONTACTS_CONTACT(psEntry);

			/* Update title */
			gchar *pnDisplayName = g_strdup_printf("%s %s", contact->pnFirstName, contact->pnLastName);
			gdata_entry_set_title(GDATA_ENTRY(contact), pnDisplayName);

			/* Get name information and update name information */
			name = gdata_contacts_contact_get_name(contact);
			if (name != NULL) {
				if (contact->pnFirstName != NULL && strlen(contact->pnFirstName) > 0) {
					gdata_gd_name_set_given_name(name, contact->pnFirstName);
				} else {
				}

				if (contact->pnLastName != NULL && strlen(contact->pnLastName) > 0) {
					gdata_gd_name_set_family_name(name, contact->pnLastName);
				}
				if (pnDisplayName != NULL && strlen(contact->pnDisplayName) > 0) {
					gdata_gd_name_set_full_name(name, pnDisplayName);
				}
				if (contact->pnTitle != NULL && strlen(contact->pnTitle) > 0) {
					gdata_gd_name_set_prefix(name, contact->pnTitle);
				}
				g_debug("Set names done");
				//g_object_unref(name);
			}

			/* Update company */
			if (contact->pnCompany != NULL && strlen(contact->pnCompany) > 0) {
				organization = gdata_contacts_contact_get_primary_organization(contact);

				if (organization == NULL) {
					organization = gdata_gd_organization_new(contact->pnCompany, NULL, NULL, NULL, TRUE);

					gdata_contacts_contact_add_organization(contact, organization);
				} else {
					gdata_gd_organization_set_name(organization, contact->pnCompany);
				}

				//g_object_unref(organization);
			}

			/* Set numbers */
			number_list = gdata_contacts_contact_get_phone_numbers(contact);
			while (number_list != NULL && number_list->data != NULL) {
				GDataGDPhoneNumber *psNumber = number_list->data;

				if (!strcmp(gdata_gd_phone_number_get_relation_type(psNumber), GDATA_GD_PHONE_NUMBER_WORK)) {
					bBusinessPhone = TRUE;
					if (strlen(contact->pnBusinessPhone) <= 0) {
						number_list = g_list_remove(number_list, psNumber);
						continue;
					} else {
						gdata_gd_phone_number_set_number(psNumber, contact->pnBusinessPhone);
					}
				} else if (!strcmp(gdata_gd_phone_number_get_relation_type(psNumber), GDATA_GD_PHONE_NUMBER_HOME)) {
					bPrivatePhone = TRUE;
					if (strlen(contact->pnPrivatePhone) <= 0) {
						number_list = g_list_remove(number_list, psNumber);
						continue;
					} else {
						gdata_gd_phone_number_set_number(psNumber, contact->pnPrivatePhone);
					}
				} else if (!strcmp(gdata_gd_phone_number_get_relation_type(psNumber), GDATA_GD_PHONE_NUMBER_MOBILE)) {
					bPrivateMobile = TRUE;
					if (strlen(contact->pnPrivateMobile) <= 0) {
						number_list = g_list_remove(number_list, psNumber);
						continue;
					} else {
						gdata_gd_phone_number_set_number(psNumber, contact->pnPrivateMobile);
					}
				} else if (!strcmp(gdata_gd_phone_number_get_relation_type(psNumber), GDATA_GD_PHONE_NUMBER_HOME_FAX)) {
					bPrivateFax = TRUE;
					if (strlen(contact->pnPrivateFax) <= 0) {
						number_list = g_list_remove(number_list, psNumber);
						continue;
					} else {
						gdata_gd_phone_number_set_number(psNumber, contact->pnPrivateFax);
					}
				} else if (!strcmp(gdata_gd_phone_number_get_relation_type(psNumber), GDATA_GD_PHONE_NUMBER_WORK_FAX)) {
					bBusinessFax = TRUE;
					if (strlen(contact->pnBusinessFax) <= 0) {
						number_list = g_list_remove(number_list, psNumber);
						continue;
					} else {
						gdata_gd_phone_number_set_number(psNumber, contact->pnBusinessFax);
					}
				}
				//g_object_unref(psNumber);

				if (number_list != NULL) {
					number_list = number_list->next;
				}
			}
			if (bBusinessPhone == FALSE && strlen(contact->pnBusinessPhone) > 0) {
				GDataGDPhoneNumber *psNumber = gdata_gd_phone_number_new(contact->pnBusinessPhone, GDATA_GD_PHONE_NUMBER_WORK, NULL, NULL, FALSE);

				gdata_contacts_contact_add_phone_number(contact, psNumber);
			}

			if (bBusinessFax == FALSE && strlen(contact->pnBusinessFax) > 0) {
				GDataGDPhoneNumber *psNumber = gdata_gd_phone_number_new(contact->pnBusinessFax, GDATA_GD_PHONE_NUMBER_WORK_FAX, NULL, NULL, FALSE);

				gdata_contacts_contact_add_phone_number(contact, psNumber);
			}

			if (bPrivatePhone == FALSE && strlen(contact->pnPrivatePhone) > 0) {
				GDataGDPhoneNumber *psNumber = gdata_gd_phone_number_new(contact->pnPrivatePhone, GDATA_GD_PHONE_NUMBER_HOME, NULL, NULL, FALSE);

				gdata_contacts_contact_add_phone_number(contact, psNumber);
			}

			if (bPrivateFax == FALSE && strlen(contact->pnPrivateFax) > 0) {
				GDataGDPhoneNumber *psNumber = gdata_gd_phone_number_new(contact->pnPrivateFax, GDATA_GD_PHONE_NUMBER_HOME_FAX, NULL, NULL, FALSE);

				gdata_contacts_contact_add_phone_number(contact, psNumber);
			}

			if (bPrivateMobile == FALSE && strlen(contact->pnPrivateMobile) > 0) {
				GDataGDPhoneNumber *psNumber = gdata_gd_phone_number_new(contact->pnPrivateMobile, GDATA_GD_PHONE_NUMBER_MOBILE, NULL, NULL, FALSE);

				gdata_contacts_contact_add_phone_number(contact, psNumber);
			}

			g_debug("Set numbers done");

			psAddress = gdata_gd_postal_address_new(GDATA_GD_POSTAL_ADDRESS_WORK, NULL, TRUE);
			if (contact->pnBusinessStreet != NULL && strlen(contact->pnBusinessStreet) > 0) {
				gdata_gd_postal_address_set_street(psAddress, contact->pnBusinessStreet);
			}
			if (contact->pnBusinessCity != NULL && strlen(contact->pnBusinessCity) > 0) {
				gdata_gd_postal_address_set_city(psAddress, contact->pnBusinessCity);
			}
			if (contact->pnBusinessCountry != NULL && strlen(contact->pnBusinessCountry) > 0) {
				gdata_gd_postal_address_set_country(psAddress, contact->pnBusinessCountry, NULL);
			}
			if (contact->pnBusinessZipCode != NULL && strlen(contact->pnBusinessZipCode) > 0) {
				gdata_gd_postal_address_set_postcode(psAddress, contact->pnBusinessZipCode);
			}

			psAddress = gdata_gd_postal_address_new(GDATA_GD_POSTAL_ADDRESS_HOME, NULL, TRUE);
			if (contact->pnPrivateStreet != NULL && strlen(contact->pnPrivateStreet) > 0) {
				gdata_gd_postal_address_set_street(psAddress, contact->pnPrivateStreet);
			}
			if (contact->pnPrivateCity != NULL  && strlen(contact->pnPrivateCity) > 0) {
				gdata_gd_postal_address_set_city(psAddress, contact->pnPrivateCity);
			}
			if (contact->pnPrivateCountry != NULL  && strlen(contact->pnPrivateCountry) > 0) {
				gdata_gd_postal_address_set_country(psAddress, contact->pnPrivateCountry, NULL);
			}
			if (contact->pnPrivateZipCode != NULL && strlen(contact->pnPrivateZipCode) > 0) {
				gdata_gd_postal_address_set_postcode(psAddress, contact->pnPrivateZipCode);
			}

			google_set_image(contact, contact);

			g_debug("Update entry");
			psNewContact = GDATA_CONTACTS_CONTACT(gdata_service_update_entry(GDATA_SERVICE(service), gdata_contacts_service_get_primary_authorization_domain(), GDATA_ENTRY(contact), NULL, &error));
			if (error != NULL) {
				g_warning("Could not update contact entry: %s", error->message);
				g_error_free(error);
				error = NULL;
			} else {
				g_debug("Update entry done");
				if (psNewContact) {
					g_debug("Unref");
					g_object_unref(psNewContact);
				}
			}
		}

		contact->nFlags = PERSON_FLAG_UNCHANGED;
	}
	return 0;
}
#endif

GSList *google_get_contacts(void)
{
	GSList *list = contacts;

	return list;
}

gchar *google_get_active_book_name(void)
{
	return g_strdup("Google");
}

struct address_book google_book = {
	"Google",
	google_get_active_book_name,
	google_get_contacts,
	NULL,//google_reload_contacts,
	NULL,//google_remove_contact,
	NULL//google_save_contact
};

void impl_activate(PeasActivatable *plugin)
{
	//RouterManagerGooglePlugin *google_plugin = ROUTERMANAGER_GOOGLE_PLUGIN(plugin);

	google_settings = rm_settings_plugin_new("org.tabos.roger.plugins.google", "google");

	table = g_hash_table_new(g_str_hash, g_str_equal);

	google_read_book();

	//google_plugin->priv->signal_id = g_signal_connect(G_OBJECT(app_object), "contact-process", G_CALLBACK(google_contact_process_cb), NULL);

	routermanager_address_book_register(&google_book);
}

void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerGooglePlugin *google_plugin = ROUTERMANAGER_GOOGLE_PLUGIN(plugin);

	routermanager_address_book_unregister(&google_book);

	if (g_signal_handler_is_connected(G_OBJECT(app_object), google_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), google_plugin->priv->signal_id);
	}

	google_shutdown();

	g_object_unref(google_settings);

	if (table) {
		g_hash_table_destroy(table);
	}
}

GtkWidget *impl_create_configure_widget(PeasGtkConfigurable *config)
{
	GtkWidget *user_entry;
	GtkWidget *password_entry;
	GtkWidget *label;
	GtkWidget *grid;
	GtkWidget *group;

	grid = gtk_grid_new();
	gtk_widget_set_margin(grid, 18, 18, 18, 18);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	label = gtk_label_new(_("User"));
	gtk_widget_set_sensitive(label, FALSE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

	user_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(grid), user_entry, 1, 0, 1, 1);
	g_settings_bind(google_settings, "user", user_entry, "text", G_SETTINGS_BIND_DEFAULT);

	label = gtk_label_new(_("Password"));
	gtk_widget_set_sensitive(label, FALSE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

	password_entry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
	gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 1, 1);
	g_settings_bind(google_settings, "password", password_entry, "text", G_SETTINGS_BIND_DEFAULT);

	group = pref_group_create(grid, _("Access data"), TRUE, FALSE);

	return group;
}
