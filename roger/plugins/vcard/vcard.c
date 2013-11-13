/**
 * Roger Router
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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
#include <stdio.h>
#include <ctype.h>

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libroutermanager/gstring.h>
#include <libroutermanager/contact.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/address-book.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/number.h>

#include <roger/main.h>
#include <roger/pref.h>

#include <vcard.h>

#define ROUTERMANAGER_TYPE_VCARD_PLUGIN        (routermanager_vcard_plugin_get_type ())
#define ROUTERMANAGER_VCARD_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_VCARD_PLUGIN, RouterManagerVCardPlugin))

typedef struct {
	guint signal_id;
} RouterManagerVCardPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER_CONFIGURABLE(ROUTERMANAGER_TYPE_VCARD_PLUGIN, RouterManagerVCardPlugin, routermanager_vcard_plugin)

static GSList *contacts = NULL;

static GSettings *vcard_settings = NULL;

static GList *vcard_list = NULL;
static GList *vcard = NULL;
static struct vcard_data *current_card_data = NULL;
static GString *current_string = NULL;
static gint state = STATE_NEW;
static gint current_position = 0;
static GString *first_name = NULL;
static GString *last_name = NULL;
static GString *uid = NULL;
static GString *company = NULL;
static GString *title = NULL;

static GHashTable *table = NULL;

/**
 * \brief Free header, options and entry line
 * \param psCard pointer to card structure
 */
static void vcard_free_data(struct vcard_data *card_data) {
	/* if header is present, free it and set to NULL */
	if (card_data->header != NULL) {
		g_free(card_data->header);
		card_data->header = NULL;
	}

	/* if options is present, free it and set to NULL */
	if (card_data->options != NULL) {
		g_free(card_data->options);
		card_data->options = NULL;
	}

	/* if entry is present, free it and set to NULL */
	if (card_data->entry != NULL) {
		g_free(card_data->entry);
		card_data ->entry = NULL;
	}

	g_free(card_data);
}

/**
 * \brief Process first/last name structure
 * \param psCard pointer to card structure
 */
static void process_first_last_name(struct vcard_data *card_data) {
	gint len = 0;
	gint index = 0;

	if (card_data == NULL || card_data->entry == NULL) {
		return;
	}

	len = strlen(card_data->entry);

	/* Create last name string */
	last_name = g_string_new("");
	while (index < len) {
		if (
			(card_data->entry[index] != 0x00) &&
			(card_data->entry[index] != ';') &&
			(card_data->entry[index] != 0x0A) &&
			(card_data->entry[index] != 0x0D)) {
			g_string_append_c(last_name, card_data->entry[index++]);
		} else {
			break;
		}
	}

	/* Skip ';' */
	index++;

	/* Create first name string */
	first_name = g_string_new("");
	while (index < len) {
		if (
			(card_data->entry[index] != 0x00) &&
			(card_data->entry[index] != ';') &&
			(card_data->entry[index] != 0x0A) &&
			(card_data->entry[index] != 0x0D)) {
			g_string_append_c(first_name, card_data->entry[index++]);
		} else {
			break;
		}
	}
}

/**
 * \brief Process full name structure
 * \param card_data pointer to card structure
 */
static void process_full_name(struct vcard_data *card_data, struct contact *contact) {
	GString *str;
	gint len = 0;
	gint index = 0;

	g_assert(card_data != NULL);
	g_assert(card_data->entry != NULL);

	len = strlen(card_data->entry);

	/* Create fullname string */
	str = g_string_new("");
	for (index = 0; index < len; index++) {
		if (
			(card_data->entry[index] != 0x00) &&
			(card_data->entry[index] != ';') &&
			(card_data->entry[index] != 0x0A) &&
			(card_data->entry[index] != 0x0D)) {
			g_string_append_c(str, card_data->entry[index]);
		} else {
			break;
		}
	}

	contact->name = g_string_free(str, FALSE);
}

/**
 * \brief Process organization structure
 * \param psCard pointer to card structure
 */
static void process_organization(struct vcard_data *card_data) {
	gint len = 0;
	gint index = 0;

	if (card_data == NULL || card_data->entry == NULL) {
		return;
	}

	len = strlen(card_data->entry);

	/* Create company string */
	company = g_string_new("");
	while (index < len) {
		if (
			(card_data->entry[index] != 0x00) &&
			(card_data->entry[index] != ';') &&
			(card_data->entry[index] != 0x0A) &&
			(card_data->entry[index] != 0x0D)) {
			g_string_append_c(company, card_data->entry[index++]);
		} else {
			break;
		}
	}
}

/**
 * \brief Process title structure
 * \param psCard pointer to card structure
 */
static void process_title(struct vcard_data *card_data) {
	gint len = 0;
	gint index = 0;

	if (card_data == NULL || card_data->entry == NULL) {
		return;
	}

	len = strlen(card_data->entry);

	/* Create title string */
	title = g_string_new("");
	while (index < len) {
		if (
			(card_data->entry[index] != 0x00) &&
			(card_data->entry[index] != ';') &&
			(card_data->entry[index] != 0x0A) &&
			(card_data->entry[index] != 0x0D)) {
			g_string_append_c(title, card_data->entry[index++]);
		} else {
			break;
		}
	}
}

/**
 * \brief Process uid structure
 * \param psCard pointer to card structure
 */
static void process_uid(struct vcard_data *card_data) {
	gint len = 0;
	gint index = 0;

	if (card_data == NULL || card_data->entry == NULL) {
		return;
	}

	len = strlen(card_data->entry);

	/* Create uid string */
	uid = g_string_new("");
	while (index < len) {
		if (
			(card_data->entry[index] != 0x00) &&
			(card_data->entry[index] != ';') &&
			(card_data->entry[index] != 0x0A) &&
			(card_data->entry[index] != 0x0D)) {
			g_string_append_c(uid, card_data->entry[index++]);
		} else {
			break;
		}
	}
}

/**
 * \brief Process address structure
 * \param card_data pointer to card structure
 */
static void process_address(struct vcard_data *card_data, struct contact *contact) {
	struct contact_address *address;
	GString *tmp_str;
	gchar *tmp = NULL;

	if (card_data == NULL || card_data->entry == NULL) {
		return;
	}

	if (card_data->options == NULL) {
		g_debug("No options for address, skipping..");
		return;
	}

	address = g_slice_new0(struct contact_address);

	tmp = card_data->entry;

	/* Create address string */
	if (g_strcasestr(card_data->options, "HOME") != NULL) {
		address->type = 0;
	} else {
		address->type = 1;
	}

	/* skip pobox */
	while (*tmp != ';') {
		tmp++;
	}
	tmp++;

	/* skip extended address */
	while (*tmp != ';') {
		tmp++;
	}
	tmp++;

	/* read street */
	tmp_str = g_string_new("");
	while (*tmp != ';') {
		g_string_append_c(tmp_str, *tmp);
		tmp++;
	}
	address->street = tmp_str->str;
	g_string_free(tmp_str, FALSE);
	tmp++;

	/* read locality */
	tmp_str = g_string_new("");
	while (*tmp != ';') {
		g_string_append_c(tmp_str, *tmp);
		tmp++;
	}
	address->city = tmp_str->str;
	g_string_free(tmp_str, FALSE);
	tmp++;

	/* skip region */
	while (*tmp != ';') {
		tmp++;
	}
	tmp++;

	/* read zip code */
	tmp_str = g_string_new("");
	while (*tmp != ';') {
		g_string_append_c(tmp_str, *tmp);
		tmp++;
	}
	address->zip = tmp_str->str;
	g_string_free(tmp_str, FALSE);
	tmp++;

	contact->addresses = g_slist_prepend(contact->addresses, address);

	/* read country */
	/*private_country = g_string_new("");
	while (*tmp != 0x00 && *tmp != 0x0A && *tmp != 0x0D) {
		g_string_append_c(private_country, *tmp);
		tmp++;
	}*/
}

/**
 * \brief Process telephone structure
 * \param card_data pointer to card structure
 */
static void process_telephone(struct vcard_data *card_data, struct contact *contact) {
	gchar *tmp = card_data->entry;
	struct phone_number *number;

	if (card_data->options == NULL) {
		g_warning("No option field in telephone entry");
		return;
	}

	number = g_slice_new(struct phone_number);

	if (g_strcasestr(card_data->options, "FAX") != NULL) {
		/*if (g_strcasestr(card_data->options, "WORK") != NULL) {
			if (business_fax == NULL) {
				business_fax = g_string_new("");
				while (*tmp != 0x00 && *tmp != 0x0A && *tmp != 0x0D) {
					g_string_append_c(business_fax, *tmp);
					tmp++;
				}
			}
		} else*/ {
			number->type = PHONE_NUMBER_FAX;
		}
	} else {
		/* Check for cell phone number, and create string if needed */
		if (g_strcasestr(card_data->options, "CELL") != NULL) {
			number->type = PHONE_NUMBER_MOBILE;
		}

		/* Check for home phone number, and create string if needed */
		if (g_strcasestr(card_data->options, "HOME") != NULL) {
			number->type = PHONE_NUMBER_HOME;
		}

		/* Check for work phone number, and create string if needed */
		if (g_strcasestr(card_data->options, "WORK") != NULL) {
			number->type = PHONE_NUMBER_WORK;
		}
	}

	GString *number_str = g_string_new("");
	while (*tmp != 0x00 && *tmp != 0x0A && *tmp != 0x0D) {
		g_string_append_c(number_str, *tmp);
		tmp++;
	}

	number->number = number_str->str;
	g_string_free(number_str, FALSE);

	contact->numbers = g_slist_prepend(contact->numbers, number);
}

/**
 * \brief Process photo structure
 * \param card_data pointer to card structure
 * \param contact contact structure
 */
static void process_photo(struct vcard_data *card_data, struct contact *contact) {
	GdkPixbufLoader *loader;
	guchar *image_ptr;
	gsize len;
	GError *error = NULL;

	if (card_data->options) {
		if (g_strcasestr(card_data->options, "VALUE=URL") != NULL) {
			return;
		}
	}

	image_ptr = g_base64_decode(card_data->entry, &len);
	loader = gdk_pixbuf_loader_new();
	if (gdk_pixbuf_loader_write(loader, image_ptr, len, &error)) {
		contact->image = gdk_pixbuf_loader_get_pixbuf(loader);
	} else {
		g_debug("Error!! (%s)", error->message);
		g_free(image_ptr);
	}
}

/**
 * \brief Create new uid
 */
GString *vcard_create_uid(void) {
	GString *id = g_string_new("");
	gint index = 0;

	for (index = 0; index < 10; index++) {
		int random = g_random_int() % 62;
		random += 48;
		if (random > 57) {
			random += 7;
		}

		if (random > 90) {
			random += 6;
		}

		id = g_string_append_c(id, (char) random);
	}

	return id;
}

/**
 * \brief Parse end of vcard, check for valid entry and add person
 */
static void process_card_end(struct contact *contact) {
	//if (uid == NULL) {
	//	uid = vcard_create_uid();
	//}

	if (company != NULL) {
		contact->company = g_strdup(company->str);
	}

	/*
	if (title != NULL) {
		AddInfo(table, PERSON_TITLE, title->str);
	}
	}*/

	if (!contact->name && first_name != NULL && last_name != NULL) {
		contact->name = g_strdup_printf("%s %s", first_name->str, last_name->str);
	}
	g_debug("Adding: '%s'", contact->name);

	contacts = g_slist_insert_sorted(contacts, contact, contact_name_compare);

	/* Free firstname */
	if (first_name != NULL) {
		g_string_free(first_name, TRUE);
		first_name = NULL;
	}

	/* Free lastname */
	if (last_name != NULL) {
		g_string_free(last_name, TRUE);
		last_name = NULL;
	}

	/* Free company */
	if (company != NULL) {
		g_string_free(company, TRUE);
		company = NULL;
	}

	/* Free title */
	if (title != NULL) {
		g_string_free(title, TRUE);
		title = NULL;
	}

	/* Free uid */
	if (uid != NULL) {
		g_string_free(uid, TRUE);
		uid = NULL;
	}
}

/**
 * \brief Process new data structure (header/options/entry)
 * \param card_data pointer to card data structure
 * \param contact contact data
 */
static void process_data(struct vcard_data *card_data) {
	static struct contact *contact;

	if (!card_data->header || !card_data->entry) {
		return;
	}

	if (strcasecmp(card_data->header, "BEGIN") == 0) {
		/* Begin of vcard */
		vcard = g_list_append(NULL, card_data);
		vcard_list = g_list_append(vcard_list, vcard);
		contact = g_slice_new0(struct contact);

		return;
	} else {
		vcard = g_list_append(vcard, card_data);
	}

	if (strcasecmp(card_data->header, "END") == 0) {
		process_card_end(contact);
	} else 	if (strcasecmp(card_data->header, "FN") == 0) {
		/* Full name */
		process_full_name(card_data, contact);
	} else if (strcasecmp(card_data->header, "END") == 0) {
		/* End of vcard */
		process_card_end(contact);
	} else if (strcasecmp(card_data->header, "N") == 0) {
		/* First and Last name */
		process_first_last_name(card_data);
	} else if (strcasecmp(card_data->header, "TEL") == 0) {
		/* Telephone */
		process_telephone(card_data, contact);
	} else if (strcasecmp(card_data->header, "ORG") == 0) {
		/* Organization */
		process_organization(card_data);
	} else if (strcasecmp(card_data->header, "TITLE") == 0) {
		/* Title */
		process_title(card_data);
	} else if (strcasecmp(card_data->header, "ADR") == 0) {
		/* Address */
		process_address(card_data, contact);
	} else if (strcasecmp(card_data->header, "PHOTO") == 0) {
		/* Photo */
		process_photo(card_data, contact);
	} else if (strcasecmp(card_data->header, "UID") == 0) {
		/* UID */
		process_uid(card_data);
	}
}

/**
 * \brief Parse one char and add it to internal card structure
 * \param chr current char
 */
void parse_char(int chr) {
	switch (state) {
		case STATE_NEW:
			current_card_data = g_malloc0(sizeof(struct vcard_data));
			state = STATE_TAG;
			/* fall-through */
		case STATE_TAG:
			switch (chr) {
				case '\r':
					break;
				case '\n':
					if (current_string != NULL) {
						g_string_free(current_string, TRUE);
					}
					current_string = NULL;
					vcard_free_data(current_card_data);
					state = STATE_NEW;
					break;
				case ':':
					current_card_data->header = g_string_free(current_string, FALSE);
					current_string = NULL;
					state = STATE_ENTRY;
					break;
				case ';':
					current_card_data->header = g_string_free(current_string, FALSE);
					current_string = NULL;
					state = STATE_OPTIONS;
					break;
				default:
					if (current_string == NULL) {
						current_string = g_string_new("");
					}
					g_string_append_c(current_string, chr);
					break;
			}
			break;
		case STATE_OPTIONS:
			switch (chr) {
				case '\r':
					break;
				case '\n':
					g_string_free(current_string, TRUE);
					current_string = NULL;
					vcard_free_data(current_card_data);
					state = STATE_NEW;
					break;
				case ':':
					current_card_data->options = g_string_free(current_string, FALSE);
					current_string = NULL;
					state = STATE_ENTRY;
					break;
				default:
					if (current_string == NULL) {
						current_string = g_string_new("");
					}
					g_string_append_c(current_string, chr);
					break;
			}
			break;
		case STATE_ENTRY:
			switch (chr) {
				case '\r':
					break;
				case '\n':
					if (current_string != NULL) {
						current_card_data->entry = g_string_free(current_string, FALSE);
						process_data(current_card_data);
					}
					current_string = NULL;
					state = STATE_NEW;
					break;
				default:
					if (current_string == NULL) {
						current_string = g_string_new("");
					}
					g_string_append_c(current_string, chr);
					break;
			}
			break;
	}
}

/**
 * \brief Load card file information
 * \param file_name file name to read
 */
void vcard_load_file(char *file_name) {
	GFile *file;
	GFileInfo *file_info;
	goffset file_size;
	GFileInputStream *input_stream;
	gchar *data;
	gint chr;
	gboolean start_of_line = TRUE;
	gboolean fold = FALSE;
	gint index;

	g_debug("opening: '%s'", file_name);

	/* Open file */
	file = g_file_new_for_path(file_name);
	file_info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	file_size = g_file_info_get_size(file_info);

	data = g_malloc0(file_size);
	input_stream = g_file_read(file, NULL, NULL);
	g_input_stream_read_all(G_INPUT_STREAM(input_stream), data, file_size, NULL, NULL, NULL);

	state = STATE_NEW;

	for (index = 0; index < file_size; index++) {
		chr = data[index];
		if (start_of_line == TRUE) {
			if (chr == '\r' || chr == '\n') {
				/* simple empty line */
				continue;
			}

			if (fold == FALSE && isspace(chr)) {
				/* Ok, we have a fold case, mark it and continue */
				fold = TRUE;
				continue;
			}

			start_of_line = FALSE;
			if (fold == TRUE) {
				fold = FALSE;
			} else {
				parse_char('\n');
			}
		}

		if (chr == '\n') {
			start_of_line = TRUE;
		} else {
			parse_char(chr);
		}
	}

	/* Ensure we get a '\n' */
	parse_char('\n');

	//fclose(file);
}

/**
 * \brief Put char to vcard structure
 * \param data data string
 * \param chr put char
 */
static void vcard_put_char(GString *data, gint chr) {
	if (current_position == 74 && chr != '\r') {
		g_string_append(data, "\r\n ");
		current_position = 1;
	} else if (chr == '\n') {
		current_position = 0;
	}

	data = g_string_append_c(data, chr);
	current_position++;
}

/**
 * \brief printf to data structure
 * \param data data string
 * \param format format string
 */
void vcard_print(GString *data, gchar *format, ...) {
	va_list args;
	int len;
	int size = 100;
	gchar *ptr = NULL;

	if ((ptr = g_malloc(size)) == NULL) {
		return;
	}

	while (1) {
		va_start(args, format);

		len = vsnprintf(ptr, size, format, args);

		va_end(args);

		if (len > -1 && len < size) {
			int index;

			for (index = 0; index < len; index++) {
				vcard_put_char(data, ptr[index]);
			}

			break;
		}

		if (len > -1) {
			size = len + 1;
		} else {
			size *= 2;
		}

		if ((ptr = g_realloc(ptr, size)) == NULL) {
			break;
		}
	}

	if (ptr != NULL) {
		g_free(ptr);
	}
}

/**
 * \brief Find vcard entry via uid
 * \param uid uid
 * \param vcard entry list pointer or NULL
 */
GList *vcard_find_entry(const gchar *uid) {
	GList *list1 = NULL;
	GList *list2 = NULL;
	GList *card = NULL;
	struct vcard_data *data;
	gchar *current_uid;

	for (list1 = vcard_list; list1 != NULL && list1->data != NULL; list1 = list1->next) {
		card = list1->data;

		for (list2 = card; list2 != NULL && list2->data != NULL; list2 = list2->next) {
			data = list2->data;

			if (strcmp(data->header, "UID") == 0) {
				current_uid = data->entry;
				if (current_uid != NULL && !strcmp(current_uid, uid)) {
					return card;
				}
			}
		}
	}

	return NULL;
}

/**
 * \brief Find card data via header and option
 * \param list vcard list structure
 * \param header header string
 * \param option optional option string
 * \return card data structure or NULL
 */
struct vcard_data *find_card_data(GList *list, gchar *header, gchar *option) {
	GList *psTmp = NULL;
	struct vcard_data *data = NULL;

	for (psTmp = list; psTmp != NULL && psTmp->data != NULL; psTmp = psTmp->next) {
		data = psTmp->data;

		if(strcmp(data->header, header) == 0) {
			if (option != NULL && data->options != NULL && strstr(data->options, option)) {
				return data;
			}
		}
	}

	return NULL;
}

/**
 * \brief Write card file information
 * \param file_name file name to read
 */
void vcard_write_file(char *file_name) {
#if 0
	GString *data = NULL;
	GList *list = NULL;
	struct sPerson *person = NULL;
	GList *entry = NULL;
	gchar *tmp = NULL;
	GList *list2 = NULL;
	struct vcard_data *card_data = NULL;

	data = g_string_new("");

	current_position = 0;

	for (list = psPersonsList; list != NULL && list->data != NULL; list = list->next) {
		person = list->data;

again:
		switch (person->nFlags) {
			case PERSON_FLAG_DELETED:
				break;
			case PERSON_FLAG_NEW:
				card_data = g_malloc0(sizeof(struct vcard_data));
				card_data->header = g_strdup("UID");
				uid = vcard_create_uid();
				person->id = g_string_free(uid, FALSE);
				uid = NULL;
				card_data->entry = g_strdup(person->id);
				vcard = g_list_append(NULL, card_data);
				vcard_list = g_list_append(vcard_list, vcard);
				/* fall-through */
			case PERSON_FLAG_CHANGED:
				entry = vcard_find_entry(person->id);
				if (entry == NULL) {
					if (person->nFlags == PERSON_FLAG_CHANGED) {
						person->nFlags = PERSON_FLAG_NEW;
						goto again;
					}
					continue;
				}

				/* Fullname */
				card_data = find_card_data(entry, "FN", NULL);
				if (card_data == NULL) {
					card_data = g_malloc0(sizeof(struct vcard_data));
					card_data->header = g_strdup("FN");
					entry = g_list_append(entry, card_data);
				} else {
					g_free(card_data->entry);
				}
				if (person->pnDisplayName != NULL) {
					g_free(person->pnDisplayName);
					person->pnDisplayName = NULL;
				}
				person->pnDisplayName = g_strdup_printf("%s %s", person->pnFirstName, person->pnLastName);
				card_data->entry = g_strdup_printf("%s", person->pnDisplayName);

				/* Name */
				card_data = find_card_data(entry, "N", NULL);
				if (card_data == NULL) {
					card_data = g_malloc0(sizeof(struct vcard_data));
					card_data->header = g_strdup("N");
					entry = g_list_append(entry, card_data);
				} else {
					g_free(card_data->entry);
				}
				card_data->entry = g_strdup_printf("%s;%s;;;", person->pnLastName, person->pnFirstName);

				/* Title */
				card_data = find_card_data(entry, "TITLE", NULL);
				if (card_data == NULL) {
					card_data = g_malloc0(sizeof(struct vcard_data));
					card_data->header = g_strdup("TITLE");
					entry = g_list_append(entry, card_data);
				} else {
					g_free(card_data->entry);
				}
				card_data->entry = g_strdup_printf("%s", person->pnTitle);

				/* Organization */
				card_data = find_card_data(entry, "ORG", NULL);
				if (card_data == NULL) {
					card_data = g_malloc0(sizeof(struct vcard_data));
					card_data->header = g_strdup("ORG");
					entry = g_list_append(entry, card_data);
				} else {
					g_free(card_data->entry);
				}
				card_data->entry = g_strdup_printf("%s", person->pnCompany);
 	
				/* Photo */
				if (person->pnNewImage != NULL) {
					gchar *data = NULL;
					gsize len = 0;

					card_data = find_card_data(entry, "PHOTO", NULL);
					if (card_data == NULL) {
						card_data = g_malloc0(sizeof(struct vcard_data));
						card_data->header = g_strdup("PHOTO");
						entry = g_list_append(entry, card_data);
					} else {
						g_free(card_data->entry);
					}

					if (g_file_get_contents(person->pnNewImage, &data, &len, NULL)) {
						gchar *base64 = g_base64_encode((const guchar *) data, len);
						if (card_data->options != NULL) {
							g_free(card_data->options);
						}
						card_data->options = g_strdup("ENCODING=b");
						card_data->entry = g_strdup(base64);
						g_free(tmp);
						g_free(base64);
					}
				} else if (person->image == NULL) {
					card_data = find_card_data(entry, "PHOTO", NULL);
					if (card_data != NULL) {
						entry = g_list_remove(entry, card_data);
					}
				}

				/* Telephone */
				if (person->pnPrivatePhone != NULL && strlen(person->pnPrivatePhone) > 0) {
					card_data = find_card_data(entry, "TEL", "TYPE=HOME,VOICE");
					if (card_data == NULL) {
						card_data = g_malloc0(sizeof(struct vcard_data));
						card_data->header = g_strdup("TEL");
						card_data->options = g_strdup("TYPE=HOME,VOICE");
						entry = g_list_append(entry, card_data);
					} else {
						g_free(card_data->entry);
					}
					card_data->entry = g_strdup(person->pnPrivatePhone);
				}
				if (person->pnBusinessPhone != NULL && strlen(person->pnBusinessPhone) > 0) {
					card_data = find_card_data(entry, "TEL", "TYPE=WORK,VOICE");
					if (card_data == NULL) {
						card_data = g_malloc0(sizeof(struct vcard_data));
						card_data->header = g_strdup("TEL");
						card_data->options = g_strdup("TYPE=WORK,VOICE");
						entry = g_list_append(entry, card_data);
					} else {
						g_free(card_data->entry);
					}
					card_data->entry = g_strdup(person->pnBusinessPhone);
				}
				if (person->pnPrivateMobile != NULL && strlen(person->pnPrivateMobile) > 0) {
					card_data = find_card_data(entry, "TEL", "TYPE=CELL");
					if (card_data == NULL) {
						card_data = g_malloc0(sizeof(struct vcard_data));
						card_data->header = g_strdup("TEL");
						card_data->options = g_strdup("TYPE=CELL");
						entry = g_list_append(entry, card_data);
					} else {
						g_free(card_data->entry);
					}
					card_data->entry = g_strdup(person->pnPrivateMobile);
				}
				if (person->pnPrivateFax != NULL && strlen(person->pnPrivateFax) > 0) {
					card_data = find_card_data(entry, "TEL", "TYPE=HOME,FAX");
					if (card_data == NULL) {
						card_data = g_malloc0(sizeof(struct vcard_data));
						card_data->header = g_strdup("TEL");
						card_data->options = g_strdup("TYPE=HOME,FAX");
						entry = g_list_append(entry, card_data);
					} else {
						g_free(card_data->entry);
					}
					card_data->entry = g_strdup(person->pnPrivateFax);
				}
				if (person->pnBusinessFax != NULL && strlen(person->pnBusinessFax) > 0) {
					card_data = find_card_data(entry, "TEL", "TYPE=WORK,FAX");
					if (card_data == NULL) {
						card_data = g_malloc0(sizeof(struct vcard_data));
						card_data->header = g_strdup("TEL");
						card_data->options = g_strdup("TYPE=WORK,FAX");
						entry = g_list_append(entry, card_data);
					} else {
						g_free(card_data->entry);
					}
					card_data->entry = g_strdup(person->pnBusinessFax);
				}

				/* Address */
				card_data = find_card_data(entry, "ADR", "TYPE=HOME");
				if (card_data == NULL) {
					card_data = g_malloc0(sizeof(struct vcard_data));
					card_data->header = g_strdup("ADR");
					card_data->options = g_strdup("TYPE=HOME");
					entry = g_list_append(entry, card_data);
				} else {
					g_free(card_data->entry);
				}
				card_data->entry = g_strdup_printf(";;%s;%s;;%s;%s",
					person->private_street ? person->private_street : "",
				    person->pnPrivateCity ? person->pnPrivateCity : "",
				    person->pnPrivateZipCode ? person->pnPrivateZipCode : "",
				    person->pnPrivateCountry ? person->pnPrivateCountry : "");

				card_data = find_card_data(entry, "ADR", "TYPE=WORK");
				if (card_data == NULL) {
					card_data = g_malloc0(sizeof(struct vcard_data));
					card_data->header = g_strdup("ADR");
					card_data->options = g_strdup("TYPE=WORK");
					entry = g_list_append(entry, card_data);
				} else {
					g_free(card_data->entry);
				}
				card_data->entry = g_strdup_printf(";;%s;%s;;%s;%s",
					person->pnBusinessStreet ? person->pnBusinessStreet : "",
				    person->pnBusinessCity ? person->pnBusinessCity : "",
				    person->pnBusinessZipCode ? person->pnBusinessZipCode : "",
				    person->pnBusinessCountry ? person->pnBusinessCountry : "");

				/* Fall-through */
			case PERSON_FLAG_UNCHANGED: {
				entry = vcard_find_entry(person->id);
				if (entry == NULL) {
					g_warning("UNCHANGED: Entry is NULL!!");
					continue;
				}

				vcard_print(data, "BEGIN:VCARD\r\n");

				for (list2 = entry; list2 != NULL && list2->data != NULL; list2 = list2->next) {
					card_data = list2->data;

					if (strcmp(card_data->header, "BEGIN") && strcmp(card_data->header, "END")) {
						if (card_data->options != NULL) {
							vcard_print(data, "%s;%s:%s\r\n", card_data->header, card_data->options, card_data->entry);
						} else {
							vcard_print(data, "%s:%s\r\n", card_data->header, card_data->entry);
						}
					}
				}

				vcard_print(data, "END:VCARD\r\n");
				break;
			}
		}
	}

	saveData(file_name, data->str, data->len);

	g_string_free(data, TRUE);
#endif
}

GSList *vcard_get_contacts(void)
{
	return contacts;
}

struct address_book vcard_book = {
	vcard_get_contacts,
	NULL, //vcard_reload_contacts,
	NULL, //vcard_remove_contact,
	NULL, //vcard_save_contact
};

gint vcard_number_compare(gconstpointer a, gconstpointer b)
{
	struct contact *contact = (struct contact *)a;
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

void vcard_contact_process_cb(AppObject *obj, struct contact *contact, gpointer user_data)
{
	struct contact *vcard_contact;

	if (EMPTY_STRING(contact->number)) {
		/* Contact number is not present, abort */
		return;
	}

	vcard_contact = g_hash_table_lookup(table, contact->number);
	if (vcard_contact) {
		if (!EMPTY_STRING(vcard_contact->name)) {
			contact_copy(vcard_contact, contact);
		} else {
			/* Previous lookup done but no result found */
			return;
		}
	} else {
		GSList *list;
		gchar *full_number = call_full_number(contact->number, FALSE);

		list = g_slist_find_custom(contacts, full_number, vcard_number_compare);
		if (list) {
			vcard_contact = list->data;

			g_hash_table_insert(table, contact->number, vcard_contact);

			contact_copy(vcard_contact, contact);
		} else {
			/* We have found no entry, mark it in table to speedup further lookup */
			vcard_contact = g_malloc0(sizeof(struct contact));
			g_hash_table_insert(table, contact->number, vcard_contact);
		}

		g_free(full_number);
	}
}

void impl_activate(PeasActivatable *plugin)
{
	RouterManagerVCardPlugin *vcard_plugin = ROUTERMANAGER_VCARD_PLUGIN(plugin);
	gchar *name ;

	vcard_settings = g_settings_new("org.tabos.roger.plugins.vcard");

	table = g_hash_table_new(g_str_hash, g_str_equal);

	name = g_settings_get_string(vcard_settings, "filename");

	vcard_load_file(name);

	vcard_plugin->priv->signal_id = g_signal_connect(G_OBJECT(app_object), "contact-process", G_CALLBACK(vcard_contact_process_cb), NULL);

	routermanager_address_book_register(&vcard_book);
}

void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerVCardPlugin *vcard_plugin = ROUTERMANAGER_VCARD_PLUGIN(plugin);

	if (g_signal_handler_is_connected(G_OBJECT(app_object), vcard_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), vcard_plugin->priv->signal_id);
	}

	g_object_unref(vcard_settings);

	if (table) {
		g_hash_table_destroy(table);
	}
}

void filename_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Select vcard file"), pref_get_window(), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		gtk_entry_set_text(GTK_ENTRY(user_data), folder);
		contacts = NULL;
		vcard_load_file(folder);

		g_free(folder);
	}

	gtk_widget_destroy(dialog);
}

GtkWidget *impl_create_configure_widget(PeasGtkConfigurable *config)
{
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *group;
	GtkWidget *report_dir_label;

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

	report_dir_label = ui_label_new(_("VCard file"));
	gtk_grid_attach(GTK_GRID(grid), report_dir_label, 0, 1, 1, 1);

	GtkWidget *report_dir_entry = gtk_entry_new();
	GtkWidget *report_dir_button = gtk_button_new_with_label(_("Select"));

	gtk_widget_set_hexpand(report_dir_entry, TRUE);
	g_settings_bind(vcard_settings, "filename", report_dir_entry, "text", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(report_dir_button, "clicked", G_CALLBACK(filename_button_clicked_cb), report_dir_entry);

	gtk_grid_attach(GTK_GRID(grid), report_dir_entry, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), report_dir_button, 2, 1, 1, 1);

	group = pref_group_create(grid, _("Contact book"), TRUE, FALSE);

	return group;
}
