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

#include <rm/rm.h>

#include <roger/main.h>

static GSList *contacts = NULL;
static GSettings *fritzfon_settings = NULL;

static RmXmlNode *master_node = NULL;

struct fritzfon_book {
	gchar *id;
	gchar *name;
};

struct fritzfon_priv {
	gchar *unique_id;
	gchar *image_url;
	GSList *nodes;
};

static GSList *fritzfon_books = NULL;

static void parse_person(RmContact *contact, RmXmlNode *person)
{
	RmXmlNode *name;
	RmXmlNode *image;
	gchar *image_ptr;
	struct fritzfon_priv *priv = contact->priv;

	/* Get real name entry */
	name = rm_xmlnode_get_child(person, "realName");
	contact->name = name ? rm_xmlnode_get_data(name) : g_strdup("");

	/* Get image */
	image = rm_xmlnode_get_child(person, "imageURL");
	if (image != NULL) {
		image_ptr = rm_xmlnode_get_data(image);
		priv->image_url = image_ptr;
		if (image_ptr != NULL) {
			/* file:///var/InternerSpeicher/FRITZ/fonpix/946684999-0.jpg */
			if (!strncmp(image_ptr, "file://", 7) && strlen(image_ptr) > 28) {
				RmProfile *profile = rm_profile_get_active();
				gchar *url = strstr(image_ptr, "/ftp/");
				gsize len;
				guchar *buffer;
				RmFtp *client;
				GdkPixbufLoader *loader;

				if (!url) {
					url = strstr(image_ptr, "/FRITZ/");
				} else {
					url += 5;
				}

				client = rm_ftp_init(rm_router_get_host(rm_profile_get_active()));
				rm_ftp_login(client, rm_router_get_ftp_user(profile), rm_router_get_ftp_password(profile));
				rm_ftp_passive(client);
				buffer = (guchar*)rm_ftp_get_file(client, url, &len);
				rm_ftp_shutdown(client);

				loader = gdk_pixbuf_loader_new();
				if (gdk_pixbuf_loader_write(loader, buffer, len, NULL)) {
					contact->image = gdk_pixbuf_loader_get_pixbuf(loader);
					contact->image_len = len;
				}
				gdk_pixbuf_loader_close(loader, NULL);
			}
		}
	}
}

static void parse_telephony(RmContact *contact, RmXmlNode *telephony)
{
	RmXmlNode *child;
	gchar *number = NULL;

	/* Check for numbers */
	for (child = rm_xmlnode_get_child(telephony, "number"); child != NULL; child = rm_xmlnode_get_next_twin(child)) {
		const gchar *type;

		type = rm_xmlnode_get_attrib(child, "type");
		if (type == NULL) {
			continue;
		}

		number = rm_xmlnode_get_data(child);
		if (!RM_EMPTY_STRING(number)) {
			RmPhoneNumber *phone_number;

			phone_number = g_slice_new(RmPhoneNumber);
			if (strcmp(type, "mobile") == 0) {
				phone_number->type = RM_PHONE_NUMBER_TYPE_MOBILE;
			} else if (strcmp(type, "home") == 0) {
				phone_number->type = RM_PHONE_NUMBER_TYPE_HOME;
			} else if (strcmp(type, "work") == 0) {
				phone_number->type = RM_PHONE_NUMBER_TYPE_WORK;
			} else if (strcmp(type, "fax_work") == 0) {
				phone_number->type = RM_PHONE_NUMBER_TYPE_FAX_WORK;
			} else if (strcmp(type, "fax_home") == 0) {
				phone_number->type = RM_PHONE_NUMBER_TYPE_FAX_HOME;
			} else if (strcmp(type, "pager") == 0) {
				phone_number->type = RM_PHONE_NUMBER_TYPE_PAGER;
			} else {
				phone_number->type = -1;
				g_debug("Unhandled phone number type: '%s'", type);
			}
			phone_number->number = rm_number_full(number, FALSE);
			contact->numbers = g_slist_prepend(contact->numbers, phone_number);
		}

		g_free(number);
	}
}

static void contact_add(RmProfile *profile, RmXmlNode *node, gint count)
{
	RmXmlNode *tmp;
	RmContact *contact;
	struct fritzfon_priv *priv;

	contact = g_slice_new0(RmContact);
	priv = g_slice_new0(struct fritzfon_priv);
	contact->priv = priv;

	for (tmp = node->child; tmp != NULL; tmp = tmp->next) {
		if (tmp->name == NULL) {
			continue;
		}

		if (!strcmp(tmp->name, "person")) {
			parse_person(contact, tmp);
		} else if (!strcmp(tmp->name, "telephony")) {
			parse_telephony(contact, tmp);
		} else if (!strcmp(tmp->name, "uniqueid")) {
			priv->unique_id = rm_xmlnode_get_data(tmp);
		} else if (!strcmp(tmp->name, "mod_time")) {
			/* empty */
		} else {
			/* Unhandled node, save it */
			priv->nodes = g_slist_prepend(priv->nodes, rm_xmlnode_copy(tmp));
		}
	}

	contacts = g_slist_insert_sorted(contacts, contact, rm_contact_name_compare);
}

static void phonebook_add(RmProfile *profile, RmXmlNode *node)
{
	RmXmlNode *child;
	gint count = 0;

	for (child = rm_xmlnode_get_child(node, "contact"); child != NULL; child = rm_xmlnode_get_next_twin(child)) {
		contact_add(profile, child, count++);
	}
}

static gint fritzfon_read_book(void)
{
	gchar uri[1024];
	RmXmlNode *node = NULL;
	RmXmlNode *child;
	RmProfile *profile = rm_profile_get_active();
	gchar *owner;
	gchar *name;

	contacts = NULL;

	if (!rm_router_login(profile)) {
		return -1;
	}

	owner = g_settings_get_string(fritzfon_settings, "book-owner");
	name = g_settings_get_string(fritzfon_settings, "book-name");

	snprintf(uri, sizeof(uri), "http://%s/cgi-bin/firmwarecfg", rm_router_get_host(profile));

	SoupMultipart *multipart = soup_multipart_new(SOUP_FORM_MIME_TYPE_MULTIPART);
	soup_multipart_append_form_string(multipart, "sid", profile->router_info->session_id);
	soup_multipart_append_form_string(multipart, "PhonebookId", owner);
	soup_multipart_append_form_string(multipart, "PhonebookExportName", name);
	soup_multipart_append_form_string(multipart, "PhonebookExport", "1");
	SoupMessage *msg = soup_form_request_new_from_multipart(uri, multipart);

	soup_session_send_message(rm_soup_session, msg);

	g_free(owner);
	g_free(name);

	if (msg->status_code != 200) {
		g_warning("Could not get firmware file");
		g_object_unref(msg);
		return FALSE;
	}

	const gchar *data = msg->response_body->data;
	gint read = msg->response_body->length;

	g_return_val_if_fail(data != NULL, -2);
#define FRITZFON_DEBUG 1
#if FRITZFON_DEBUG
	if (read > 0) {
		rm_log_save_data("test-in.xml", data, read);
	}
#endif

	node = rm_xmlnode_from_str(data, read);
	if (node == NULL) {
		g_object_unref(msg);
		return -1;
	}

	master_node = node;

	for (child = rm_xmlnode_get_child(node, "phonebook"); child != NULL; child = rm_xmlnode_get_next_twin(child)) {
		phonebook_add(profile, child);
	}

	g_object_unref(msg);

	//rm_router_logout(profile);

	return 0;
}

GSList *fritzfon_get_contacts(void)
{
	GSList *list = contacts;

	return list;
}

static gint fritzfon_get_books(void)
{
	gchar *url;
	RmProfile *profile = rm_profile_get_active();
	SoupMessage *msg;
	struct fritzfon_book *book = NULL;

	if (!rm_router_login(profile)) {
		return -1;
	}

	url = g_strdup_printf("http://%s/fon_num/fonbook_select.lua", rm_router_get_host(profile));
	msg = soup_form_request_new(SOUP_METHOD_GET, url,
				    "sid", profile->router_info->session_id,
				    NULL);
	g_free(url);

	soup_session_send_message(rm_soup_session, msg);
	if (msg->status_code != 200) {
		g_warning("Could not get fonbook file");
		g_object_unref(msg);
		goto end;
	}

	const gchar *data = msg->response_body->data;

	gint read = msg->response_body->length;
	rm_log_save_data("fritzfon-getbooks.html", data, read);

	g_return_val_if_fail(data != NULL, -2);

	gchar *pos = (gchar*)data;
	do {
		pos = strstr(pos, "<label for=\"uiBookid:");
		if (pos) {
			/* Extract ID */
			gchar *end = strstr(pos + 22, "\"");
			g_assert(end != NULL);
			gint len = end - pos - 21;
			gchar *num = g_malloc0(len + 1);
			strncpy(num, pos + 21, len);

			/* Extract Name */
			pos = end;
			end = strstr(pos + 2, "<");
			g_assert(end != NULL);
			len = end - pos - 1;
			gchar *name = g_malloc0(len);
			strncpy(name, pos + 2, len - 1);
			pos = end;

			book = g_slice_new(struct fritzfon_book);
			book->id = num;
			book->name = name;

			fritzfon_books = g_slist_prepend(fritzfon_books, book);
		} else {
			break;
		}

		pos++;
	} while (pos != NULL);

	g_object_unref(msg);

 end:
	if (g_slist_length(fritzfon_books) == 0) {
		book = g_slice_new(struct fritzfon_book);
		book->id = g_strdup("0");
		book->name = g_strdup("Telefonbuch");

		fritzfon_books = g_slist_prepend(fritzfon_books, book);
	}

	//rm_router_logout(profile);

	return 0;
}

gboolean fritzfon_reload_contacts(void)
{
	return fritzfon_read_book() == 0;
}


RmXmlNode *create_phone(char *type, char *number)
{
	RmXmlNode *number_node;

	number_node = rm_xmlnode_new("number");
	rm_xmlnode_set_attrib(number_node, "type", type);

	/* For the meantime set priority to 0, TODO */
	rm_xmlnode_set_attrib(number_node, "prio", "0");
	rm_xmlnode_insert_data(number_node, number, -1);

	return number_node;
}

/**
 * \brief Convert person structure to xml node
 * \param contact person structure
 * \return xml node
 */
static RmXmlNode *contact_to_xmlnode(RmContact *contact)
{
	RmXmlNode *node;
	RmXmlNode *contact_node;
	RmXmlNode *realname_node;
	RmXmlNode *image_node;
	RmXmlNode *telephony_node;
	RmXmlNode *tmp_node;
	GSList *list;
	gchar *tmp;
	struct fritzfon_priv *priv = contact->priv;

	/* Main contact entry */
	node = rm_xmlnode_new("contact");

	/* Person */
	contact_node = rm_xmlnode_new("person");

	realname_node = rm_xmlnode_new("realName");
	rm_xmlnode_insert_data(realname_node, contact->name, -1);
	rm_xmlnode_insert_child(contact_node, realname_node);

	/* ImageURL */
	if (priv && priv->image_url) {
		image_node = rm_xmlnode_new("imageURL");
		rm_xmlnode_insert_data(image_node, priv->image_url, -1);
		rm_xmlnode_insert_child(contact_node, image_node);
	}

	/* Insert person to main node */
	rm_xmlnode_insert_child(node, contact_node);

	/* Telephony */
	if (contact->numbers) {
		gboolean first = TRUE;
		gint id = 0;
		gchar *tmp = g_strdup_printf("%d", g_slist_length(contact->numbers));

		telephony_node = rm_xmlnode_new("telephony");
		rm_xmlnode_set_attrib(telephony_node, "nid", tmp);
		g_free(tmp);

		for (list = contact->numbers; list != NULL; list = list->next) {
			RmPhoneNumber *number = list->data;
			RmXmlNode *number_node;

			number_node = rm_xmlnode_new("number");

			switch (number->type) {
			case RM_PHONE_NUMBER_TYPE_HOME:
				rm_xmlnode_set_attrib(number_node, "type", "home");
				break;
			case RM_PHONE_NUMBER_TYPE_WORK:
				rm_xmlnode_set_attrib(number_node, "type", "work");
				break;
			case RM_PHONE_NUMBER_TYPE_MOBILE:
				rm_xmlnode_set_attrib(number_node, "type", "mobile");
				break;
			case RM_PHONE_NUMBER_TYPE_FAX_WORK:
				rm_xmlnode_set_attrib(number_node, "type", "fax_work");
				break;
			case RM_PHONE_NUMBER_TYPE_FAX_HOME:
				rm_xmlnode_set_attrib(number_node, "type", "fax_home");
				break;
			default:
				continue;
			}

			if (first) {
				/* For the meantime set priority to 1 */
				rm_xmlnode_set_attrib(number_node, "prio", "1");
				first = FALSE;
			}

			tmp = g_strdup_printf("%d", id++);
			rm_xmlnode_set_attrib(number_node, "id", tmp);
			g_free(tmp);

			rm_xmlnode_insert_data(number_node, number->number, -1);
			rm_xmlnode_insert_child(telephony_node, number_node);
		}
		rm_xmlnode_insert_child(node, telephony_node);
	}

	tmp_node = rm_xmlnode_new("mod_time");
	tmp = g_strdup_printf("%u", (unsigned)time(NULL));
	rm_xmlnode_insert_data(tmp_node, tmp, -1);
	rm_xmlnode_insert_child(node, tmp_node);
	g_free(tmp);

	tmp_node = rm_xmlnode_new("uniqueid");
	if (priv && priv->unique_id) {
		rm_xmlnode_insert_data(tmp_node, priv->unique_id, -1);
	}
	rm_xmlnode_insert_child(node, tmp_node);

	if (priv) {
		for (list = priv->nodes; list != NULL; list = list->next) {
			RmXmlNode *priv_node = list->data;
			rm_xmlnode_insert_child(node, priv_node);
		}
	}

	return node;
}

/**
 * \brief Convert phonebooks to xml node
 * \return xml node
 */
RmXmlNode *phonebook_to_xmlnode(void)
{
	RmXmlNode *node;
	RmXmlNode *child;
	RmXmlNode *book;
	GSList *list;

	/* Create general phonebooks node */
	node = rm_xmlnode_new("phonebooks");

	/* Currently we only support one phonebook, TODO */
	book = rm_xmlnode_new("phonebook");
	rm_xmlnode_set_attrib(book, "owner", g_settings_get_string(fritzfon_settings, "book-owner"));
	rm_xmlnode_set_attrib(book, "name", g_settings_get_string(fritzfon_settings, "book-name"));
	rm_xmlnode_insert_child(node, book);

	/* Loop through persons list and add only non-deleted entries */
	for (list = contacts; list != NULL; list = list->next) {
		RmContact *contact = list->data;

		/* Convert each contact and add it to current phone book */
		child = contact_to_xmlnode(contact);
		rm_xmlnode_insert_child(book, child);
	}

	return node;
}

gboolean fritzfon_save(void)
{
	RmXmlNode *node;
	RmProfile *profile = rm_profile_get_active();
	gchar *data;
	gint len;
	SoupBuffer *buffer;

	if (strlen(g_settings_get_string(fritzfon_settings, "book-owner")) > 2) {
		g_warning("Cannot save online address books");
		return FALSE;
	}

	if (!rm_router_login(profile)) {
		return FALSE;
	}

	node = phonebook_to_xmlnode();

	data = rm_xmlnode_to_formatted_str(node, &len);
#define FRITZFON_DEBUG 1
#ifdef FRITZFON_DEBUG
	gchar *file;
	g_debug("len: %d", len);
	file = g_strdup("/tmp/test.xml");
	if (len > 0) {
		rm_file_save(file, data, len);
	}
	g_free(file);
#endif
//	return FALSE;

	buffer = soup_buffer_new(SOUP_MEMORY_TAKE, data, len);

	/* Create POST message */
	gchar *url = g_strdup_printf("http://%s/cgi-bin/firmwarecfg", rm_router_get_host(profile));
	SoupMultipart *multipart = soup_multipart_new(SOUP_FORM_MIME_TYPE_MULTIPART);
	soup_multipart_append_form_string(multipart, "sid", profile->router_info->session_id);
	soup_multipart_append_form_string(multipart, "PhonebookId", g_settings_get_string(fritzfon_settings, "book-owner"));
	soup_multipart_append_form_file(multipart, "PhonebookImportFile", "dummy", "text/xml", buffer);
	SoupMessage *msg = soup_form_request_new_from_multipart(url, multipart);

	soup_session_send_message(rm_soup_session, msg);
	soup_buffer_free(buffer);
	g_free(url);

	if (msg->status_code != 200) {
		g_warning("Could not send phonebook");
		g_object_unref(msg);
		return FALSE;
	}
	g_object_unref(msg);

	return TRUE;
}

gboolean fritzfon_remove_contact(RmContact *contact)
{
	contacts = g_slist_remove(contacts, contact);
	return fritzfon_save();
}

void fritzfon_set_image(RmContact *contact)
{
	struct fritzfon_priv *priv = g_slice_new0(struct fritzfon_priv);
	RmProfile *profile = rm_profile_get_active();
	RmFtp *client = rm_ftp_init(rm_router_get_host(profile));
	gchar *volume_path;
	gchar *path;
	gchar *file_name;
	gchar *hash;
	gchar *data;
	gsize size;

	contact->priv = priv;
	rm_ftp_login(client, rm_router_get_ftp_user(profile), rm_router_get_ftp_password(profile));

	volume_path = g_settings_get_string(profile->settings, "fax-volume");
	hash = g_strdup_printf("%s%s", volume_path, contact->image_uri);
	file_name = g_strdup_printf("%d.jpg", g_str_hash(hash));
	g_free(hash);
	path = g_strdup_printf("%s/FRITZ/fonpix/", volume_path);
	g_free(volume_path);

	data = rm_file_load(contact->image_uri, &size);
	rm_ftp_put_file(client, file_name, path, data, size);
	rm_ftp_shutdown(client);

	priv->image_url = g_strdup_printf("file:///var/media/ftp/%s%s", path, file_name);
	g_free(path);
	g_free(file_name);
}

gboolean fritzfon_save_contact(RmContact *contact)
{
	if (!contact->priv) {
		if (contact->image_uri) {
			fritzfon_set_image(contact);
		}
		contacts = g_slist_insert_sorted(contacts, contact, rm_contact_name_compare);
	} else {
		if (contact->image_uri) {
			fritzfon_set_image(contact);
		}
	}
	return fritzfon_save();
}

gchar *fritzfon_get_active_book_name(void)
{
	return g_strdup(g_settings_get_string(fritzfon_settings, "book-name"));
}

gchar **fritzfon_get_sub_books(void)
{
	GSList *list;
	gchar **ret = NULL;

	for (list = fritzfon_books; list != NULL; list = list->next) {
		struct fritzfon_book *book = list->data;

		ret = rm_strv_add(ret, book->name);
	}

	return ret;
}

gboolean fritzfon_set_sub_book(gchar *name)
{
	GSList *list;

	for (list = fritzfon_books; list != NULL; list = list->next) {
		struct fritzfon_book *book = list->data;

		if (!strcmp(book->name, name)) {
			g_settings_set_string(fritzfon_settings, "book-owner", book->id);
			g_settings_set_string(fritzfon_settings, "book-name", book->name);

			contacts = NULL;
			fritzfon_read_book();

			return TRUE;
		}
	}

	return FALSE;
}

RmAddressBook fritzfon_book = {
	"FritzFon",
	fritzfon_get_active_book_name,
	fritzfon_get_contacts,
	fritzfon_reload_contacts,
	fritzfon_remove_contact,
	fritzfon_save_contact,
	fritzfon_get_sub_books,
	fritzfon_set_sub_book
};

gboolean fritzfon_plugin_init(RmPlugin *plugin)
{
	fritzfon_settings = rm_settings_new("org.tabos.roger.plugins.fritzfon");

	fritzfon_get_books();
	fritzfon_read_book();

	rm_addressbook_register(&fritzfon_book);

	return TRUE;
}

gboolean fritzfon_plugin_shutdown(RmPlugin *plugin)
{
	rm_addressbook_unregister(&fritzfon_book);
	g_clear_object(&fritzfon_settings);

	return TRUE;
}

RM_PLUGIN(fritzfon);
