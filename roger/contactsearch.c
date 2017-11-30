/*
 * Roger Router
 * Copyright (c) 2012-2017 Jan-Michael Brummer
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

#include <ctype.h>

#include <rm/rm.h>

#include <roger/contactsearch.h>
#include <roger/contacts.h>
#include <roger/icons.h>
#include <roger/main.h>
#include <roger/gd-two-lines-renderer.h>

struct _ContactSearch {
	GtkBox parent_instance;

	GtkWidget *entry;
	GtkEntryCompletion *completion;
};

G_DEFINE_TYPE(ContactSearch, contact_search, GTK_TYPE_BOX);

#define ROW_PADDING_VERT	4
#define ICON_PADDING_LEFT	5
#define ICON_CONTENT_WIDTH	32
#define ICON_PADDING_RIGHT	9
#define ICON_CONTENT_HEIGHT	32
#define TEXT_PADDING_LEFT	0
#define BKMK_PADDING_RIGHT	6

/**
 * phone_number_type_to_string:
 * @type: a #RmPhoneNumberType
 *
 * Convert phone number type to string
 *
 * Returns: phone number type as string
 */
gchar *phone_number_type_to_string(RmPhoneNumberType type)
{
	gchar *tmp;

	switch (type) {
	case RM_PHONE_NUMBER_TYPE_HOME:
		tmp = g_strdup(_("Home"));
		break;
	case RM_PHONE_NUMBER_TYPE_WORK:
		tmp = g_strdup(_("Work"));
		break;
	case RM_PHONE_NUMBER_TYPE_MOBILE:
		tmp = g_strdup(_("Mobile"));
		break;
	case RM_PHONE_NUMBER_TYPE_FAX_HOME:
		tmp = g_strdup(_("Fax Home"));
		break;
	case RM_PHONE_NUMBER_TYPE_FAX_WORK:
		tmp = g_strdup(_("Fax Work"));
		break;
	default:
		tmp = g_strdup(_("Unknown"));
		break;
	}

	return tmp;
}

/**
 * contact_search_set_contact:
 * @widget: a #ContactSearch
 * @contact: a #RmContact
 * @identify: identify contact flag
 *
 * Sets contact information within the widget.
 */
void contact_search_set_contact(ContactSearch *widget, RmContact *contact, gboolean identify)
{
	RmContact *search_contact;

	if (!widget || !contact) {
		return;
	}

	if (identify) {
		/* Copy contact and try to identify it */
		search_contact = rm_contact_dup(contact);
		rm_object_emit_contact_process(search_contact);
	} else {
		search_contact = contact;
	}

	g_object_set_data(G_OBJECT(widget->entry), "number", g_strdup(search_contact->number));

	if (!RM_EMPTY_STRING(search_contact->name)) {
		gtk_entry_set_text(GTK_ENTRY(widget->entry), search_contact->name);
	} else {
		gtk_entry_set_text(GTK_ENTRY(widget->entry), search_contact->number);
	}

	if (identify) {
		rm_contact_free(search_contact);
	}
}

/**
 * contact_search_class_init:
 * @klass: a #ContactSearchClass
 *
 * Initialize Contact Search class
 */
static void contact_search_class_init(ContactSearchClass *klass)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(widget_class, "/org/tabos/roger/contactsearch.glade");

	gtk_widget_class_bind_template_child(widget_class, ContactSearch, entry);
}

/**
 * contact_search_match_func:
 * @completion: a #GtkEntryCompletion
 * @key: key to match
 * @iter: a #GtkTreeIter
 * @user_data: unused
 *
 * Match key agains current iter item
 *
 * Returns: %TRUE if its match, otherwise %FALSE
 */
static gboolean contact_search_match_func(GtkEntryCompletion *completion, const gchar *key, GtkTreeIter *iter, gpointer user_data)
{
	GtkTreeModel *model;
	gchar *item = NULL;
	gboolean ret = FALSE;

	model = gtk_entry_completion_get_model(completion);

	gtk_tree_model_get(model, iter, 1, &item, -1);

	if (item != NULL) {
		if (rm_strcasestr(item, key) != NULL) {
			ret = TRUE;
		}

		g_free (item);
	}

	return ret;
}

static gboolean contact_search_completion_match_selected_cb(GtkEntryCompletion *completion, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	ContactSearch *contact_search = user_data;
	RmPhoneNumber *item;

	gtk_tree_model_get(model, iter, 3, &item, -1);

	gtk_entry_set_text(GTK_ENTRY(contact_search->entry), item->number);

	g_signal_emit_by_name(contact_search->entry, "activate");

	return TRUE;
}

/**
 * contact_search_init:
 * @widget: a #ContactSearch
 *
 * Initialize ContactSearch widget
 */
static void contact_search_init(ContactSearch *widget)
{
	GtkListStore *store;
	GSList *list;
	RmAddressBook *book;
	GtkCellRenderer *cell;

	gtk_widget_init_template(GTK_WIDGET(widget));
	gtk_entry_set_activates_default(GTK_ENTRY(widget->entry), TRUE);

	widget->completion = gtk_entry_completion_new();
	//g_object_unref(widget->completion);

        store = gtk_list_store_new(4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

	book = rm_profile_get_addressbook(rm_profile_get_active());
	if (!book) {
		GSList *book_plugins = rm_addressbook_get_plugins();

		if (book_plugins) {
			book = book_plugins->data;
		}
	}

	if (book) {
		g_debug("%s(): book '%s'", __FUNCTION__, rm_addressbook_get_name(book));
	}
	list = rm_addressbook_get_contacts(book);

        while (list != NULL) {
                RmContact *contact = list->data;
		GtkTreeIter iter;

                if (contact != NULL) {
			GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), AVATAR_DEFAULT, 32, 0, NULL);
			GSList *numbers = contact->numbers;

			for (numbers = contact->numbers; numbers != NULL; numbers = numbers->next) {
				RmPhoneNumber *phone_number = numbers->data;
				gchar *num_str = g_strdup_printf("%s: %s", phone_number_type_to_string(phone_number->type), phone_number->number);

				gtk_list_store_insert_with_values(store, &iter, -1, 0, pixbuf, 1, contact->name, 2, num_str, 3, phone_number, -1);
			}
                }

                list = list->next;
        }
        gtk_entry_completion_set_model(widget->completion, GTK_TREE_MODEL(store));
	g_signal_connect(widget->completion, "match-selected", G_CALLBACK(contact_search_completion_match_selected_cb), widget);

	cell = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget->completion), cell, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget->completion), cell, "pixbuf", 0, NULL);

	gtk_cell_renderer_set_padding(cell, ICON_PADDING_LEFT, ROW_PADDING_VERT);
	gtk_cell_renderer_set_fixed_size(cell, (ICON_PADDING_LEFT + ICON_CONTENT_WIDTH + ICON_PADDING_RIGHT), ICON_CONTENT_HEIGHT);
	gtk_cell_renderer_set_alignment (cell, 0.0, 0.5);

	cell = gd_two_lines_renderer_new();
	g_object_set(cell, "ellipsize", PANGO_ELLIPSIZE_END, "text-lines", 2, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget->completion), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget->completion), cell, "text", 1);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget->completion), cell, "line-two", 2);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget->completion), cell, TRUE);
	gtk_entry_completion_set_match_func(widget->completion, contact_search_match_func, NULL, NULL);

	gtk_entry_set_completion(GTK_ENTRY(widget->entry), widget->completion);
}

/**
 * contact_search_new:
 *
 * Create a new contact search widget
 *
 * Returns: a ContactSearch widget
 */
GtkWidget *contact_search_new(void)
{
	return g_object_new(CONTACT_TYPE_SEARCH, NULL);
}

/**
 * contact_search_get_number:
 * @widget: a #ContactSearch
 *
 * Get current phone number
 *
 * Returns: current phone number
 */
gchar *contact_search_get_number(ContactSearch *widget)
{
	gchar *number = g_object_get_data(G_OBJECT(widget->entry), "number");

	if (RM_EMPTY_STRING(number)) {
		number = (gchar*)gtk_entry_get_text(GTK_ENTRY(widget->entry));
	}

	return number;
}

/**
 * contact_search_clear:
 * @widget: a #ContactSearch
 *
 * Clear contact search entry
 */
void contact_search_clear(ContactSearch *widget)
{
	gtk_entry_set_text(GTK_ENTRY(widget->entry), "");
}

/**
 * contact_search_set_text:
 * @widget: a #ContactSearch
 * @text: text to set in contact search widget
 *
 * Sets @text in @widget.
 */
void contact_search_set_text(ContactSearch *widget, gchar *text)
{
	gtk_entry_set_text(GTK_ENTRY(widget->entry), text);
}

/**
 * contact_search_get_text:
 * @widget: a #ContactSearch
 *
 * Retieves current text in @widget.
 *
 * Returns: current text
 */
const gchar *contact_search_get_text(ContactSearch *widget)
{
	return gtk_entry_get_text(GTK_ENTRY(widget->entry));
}

