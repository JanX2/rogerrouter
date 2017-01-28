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

#include <libroutermanager/address-book.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/router.h>
#include <roger/contacts.h>
#include <roger/main.h>
#include <roger/uitools.h>
#include <roger/phone.h>
#include <roger/icons.h>
#include <roger/journal.h>

struct contacts {
	GtkWidget *window;
	GtkWidget *active_user_widget;
	GtkWidget *cancel_button;
	GtkWidget *edit_button;
	GtkWidget *save_button;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *list_box;
	GtkWidget *search_entry;
	GtkWidget *view_port;
	GtkWidget *header_bar_title;

	GtkWidget *details_placeholder_box;

	struct contact *tmp_contact;
	struct contact *new_contact;
};

static struct contacts *contacts = NULL;

/**
 * \brief Phone/dial button clicked
 * \param button phone button widget
 * \param user_data telephone number to dial
 */
static void dial_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct contact *contact;
	gchar *full_number;
	gchar *number = user_data;

	/* Create full number including prefixes */
	full_number = call_full_number(number, FALSE);
	g_assert(full_number != NULL);

	/* Find matching contact */
	contact = contact_find_by_number(full_number);
	g_free(full_number);
	g_assert(contact != NULL);

	contact->number = number;

	/* Show phone window with given contact */
	app_show_phone_window(contact, NULL);
}

/**
 * \brief Update contact detail page
 * \param contact contact structure
 */
static void contacts_update_details(struct contact *contact)
{
	GtkWidget *detail_photo_image = NULL;
	GtkWidget *detail_name_label = NULL;
	GtkWidget *frame;
	GSList *numbers;
	GSList *addresses;
	GtkWidget *grid;
	gchar *markup;
	gint detail_row = 1;

	grid = gtk_grid_new();

	/* Clear header bar title */
	gtk_label_set_text(GTK_LABEL(contacts->header_bar_title), "");

	/* Check for an active address book */
	if (address_book_available()) {
		if (contact) {
			gtk_widget_set_margin(grid, 18, 18, 18, 18);

			gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
			gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

			frame = gtk_frame_new(NULL);
			gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
			gtk_grid_attach(GTK_GRID(grid), frame, 0, 0, 1, 1);

			detail_photo_image = gtk_image_new();
			gtk_container_add(GTK_CONTAINER(frame), detail_photo_image);

			detail_name_label = gtk_label_new("");
			gtk_label_set_ellipsize(GTK_LABEL(detail_name_label), PANGO_ELLIPSIZE_END);
			gtk_widget_set_halign(detail_name_label, GTK_ALIGN_START);
			gtk_widget_set_hexpand(detail_name_label, TRUE);
			gtk_grid_attach(GTK_GRID(grid), detail_name_label, 1, 0, 1, 1);

			if (contact->image) {
				GdkPixbuf *buf = image_get_scaled(contact->image, 96, 96);
				gtk_image_set_from_pixbuf(GTK_IMAGE(detail_photo_image), buf);
			} else {
				gtk_image_set_from_icon_name(GTK_IMAGE(detail_photo_image), AVATAR_DEFAULT, GTK_ICON_SIZE_DIALOG);
				gtk_image_set_pixel_size(GTK_IMAGE(detail_photo_image), 96);
			}

			markup = g_markup_printf_escaped("<span size=\"x-large\">%s</span>", contact->name);
			gtk_label_set_markup(GTK_LABEL(detail_name_label), markup);
			g_free(markup);
			gtk_label_set_text(GTK_LABEL(contacts->header_bar_title), contact->name);

			for (numbers = contact->numbers; numbers != NULL; numbers = numbers->next) {
				GtkWidget *type;
				GtkWidget *number;
				GtkWidget *dial;
				GtkWidget *phone_image;
				struct phone_number *phone_number = numbers->data;

				switch (phone_number->type) {
				case PHONE_NUMBER_HOME:
					type = ui_label_new(_("Private"));
					break;
				case PHONE_NUMBER_WORK:
					type = ui_label_new(_("Business"));
					break;
				case PHONE_NUMBER_MOBILE:
					type = ui_label_new(_("Mobile"));
					break;
				case PHONE_NUMBER_FAX_HOME:
					type = ui_label_new(_("Private Fax"));
					break;
				case PHONE_NUMBER_FAX_WORK:
					type = ui_label_new(_("Business Fax"));
					break;
				case PHONE_NUMBER_PAGER:
					type = ui_label_new(_("Pager"));
					break;
				default:
					type = ui_label_new(_("Unknown"));
				}

				number = gtk_label_new(phone_number->number);
				gtk_label_set_selectable(GTK_LABEL(number), TRUE);
				gtk_widget_set_halign(number, GTK_ALIGN_START);
				dial = gtk_button_new();
				gtk_widget_set_tooltip_text(dial, _("Dial number"));
				phone_image = gtk_image_new_from_icon_name(APP_ICON_CALL, GTK_ICON_SIZE_BUTTON);
				gtk_button_set_image(GTK_BUTTON(dial), phone_image);

#if GTK_CHECK_VERSION(3,20,0)
				gchar *css_data = g_strdup_printf(".circular-button { border-radius: 20px; -gtk-outline-radius: 20px;}");
#else
				gchar *css_data = g_strdup_printf(".circular-button { border-radius: 20px; outline-radius: 20px; }");
#endif
				GtkCssProvider *css_provider = gtk_css_provider_get_default();
				gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
				g_free(css_data);

				GtkStyleContext *style_context =  gtk_widget_get_style_context(dial);
				gtk_style_context_add_provider(style_context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
				gtk_style_context_add_class(style_context, "circular-button");

				g_signal_connect(dial, "clicked", G_CALLBACK(dial_clicked_cb), phone_number->number);
				gtk_grid_attach(GTK_GRID(grid), type, 0, detail_row, 1, 1);
				gtk_grid_attach(GTK_GRID(grid), number, 1, detail_row, 1, 1);
				gtk_grid_attach(GTK_GRID(grid), dial, 2, detail_row, 1, 1);
				detail_row++;
			}

			for (addresses = contact->addresses; addresses != NULL; addresses = addresses->next) {
				struct contact_address *address = addresses->data;
				GtkWidget *type;
				GString *addr_str = g_string_new("");
				GtkWidget *label;

				switch (address->type) {
				case 0:
					type = ui_label_new(_("Private"));
					break;
				case 1:
					type = ui_label_new(_("Business"));
					break;
				default:
					type = ui_label_new(_("Other"));
				}
				gtk_widget_set_valign(type, GTK_ALIGN_START);
				gtk_grid_attach(GTK_GRID(grid), type, 0, detail_row, 1, 1);

				g_string_append_printf(addr_str, "%s", address->street);
				if (!EMPTY_STRING(address->zip)) {
					g_string_append_printf(addr_str, "\n%s %s", address->zip, address->city);
				} else if (!EMPTY_STRING(address->city)) {
					g_string_append_printf(addr_str, "%s", address->city);
				}

				label = gtk_label_new(addr_str->str);
				gtk_label_set_selectable(GTK_LABEL(label), TRUE);
				gtk_widget_set_halign(label, GTK_ALIGN_START);
				gtk_widget_set_valign(label, GTK_ALIGN_END);
				gtk_grid_attach(GTK_GRID(grid), label, 1, detail_row, 1, 1);

				g_string_free(addr_str, TRUE);
				detail_row++;

			}
			gtk_widget_set_visible(contacts->edit_button, TRUE);
		} else {
			GtkWidget *box;
			GtkWidget *placeholder;
			GtkWidget *img;

			box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
			gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
			gtk_widget_set_hexpand(box, TRUE);
			gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
			gtk_widget_set_vexpand(box, TRUE);
			gtk_grid_attach(GTK_GRID(grid), box, 0, 0, 1, 1);

			img = gtk_image_new_from_icon_name(AVATAR_DEFAULT, GTK_ICON_SIZE_DIALOG);
			gtk_image_set_pixel_size(GTK_IMAGE(img), 128);
			gtk_box_pack_start(GTK_BOX(box), img, TRUE, TRUE, 6);

			placeholder = gtk_label_new(NULL);
			gtk_label_set_markup(GTK_LABEL(placeholder), _("<b>Choose a contact</b>"));
			gtk_box_pack_start(GTK_BOX(box), placeholder, TRUE, TRUE, 6);
			gtk_widget_set_sensitive(box, FALSE);

			gtk_widget_set_visible(contacts->edit_button, FALSE);
		}
	} else {
		gtk_grid_attach(GTK_GRID(grid), contacts->details_placeholder_box, 0, 0, 1, 1);

		gtk_widget_set_visible(contacts->edit_button, FALSE);
	}

	gtk_widget_show_all(grid);

	if (contacts->active_user_widget) {
		gtk_container_remove(GTK_CONTAINER(contacts->view_port), contacts->active_user_widget);
	}

	contacts->active_user_widget = grid;

	gtk_container_add(GTK_CONTAINER(contacts->view_port), grid);
}

/**
 * \brief Destory child widget
 * \param widget child widget
 * \param user_data UNUSED
 */
static void contacts_destroy_child(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
}

static struct contact *contacts_get_selected_contact(void)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(contacts->list_box));
	GtkWidget *child;
	struct contact *contact;

	/* Sanity check #1 */
	if (!row) {
		return NULL;
	}

	/* Get first child */
	child = gtk_container_get_children(GTK_CONTAINER(row))->data;

	/* Sanity check #2 */
	if (!child) {
		return NULL;
	}

	/* Get contact */
	contact = g_object_get_data(G_OBJECT(child), "contact");

	return contact;
}

/**
 * \brief Update contact list (clears previous and add all matching contacts)
 */
static void contacts_update_list(void)
{
	GSList *list;
	GSList *contact_list = address_book_get_contacts();
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(contacts->search_entry));
	gint pos = 0;
	struct contact *selected_contact;

	selected_contact = contacts_get_selected_contact();

	/* For all children of list box, call contacts_destory_child() */
	gtk_container_foreach(GTK_CONTAINER(contacts->list_box), contacts_destroy_child, NULL);

	for (list = contact_list; list != NULL; list = list->next, pos++) {
		struct contact *contact = list->data;
		GtkWidget *child_box;
		GtkWidget *img;
		GtkWidget *txt;

		/* Check whether we have a filter set and it matches */
		if (text && (!g_strcasestr(contact->name, text) && !g_strcasestr(contact->company, text))) {
			continue;
		}

		/* Create child box */
		child_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		g_object_set_data(G_OBJECT(child_box), "contact", contact);

		/* Create contact image */
		if (contact->image) {
			img = gtk_image_new_from_pixbuf(image_get_scaled(contact->image, -1, -1));
		} else {
			img = gtk_image_new_from_icon_name(AVATAR_DEFAULT, GTK_ICON_SIZE_DIALOG);
		}
		gtk_box_pack_start(GTK_BOX(child_box), img, FALSE, FALSE, 6);

		/* Add contact name */
		txt = gtk_label_new(contact->name);
		gtk_label_set_ellipsize(GTK_LABEL(txt), PANGO_ELLIPSIZE_END);
		gtk_box_pack_start(GTK_BOX(child_box), txt, FALSE, FALSE, 6);
		gtk_widget_show_all(child_box);

		/* Insert child box to contacts list box */
		gtk_list_box_insert(GTK_LIST_BOX(contacts->list_box), child_box, pos);

		if (selected_contact && !strcmp(selected_contact->name, contact->name)) {
			GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(contacts->list_box), pos);

			gtk_list_box_select_row(GTK_LIST_BOX(contacts->list_box), row);
		}
	}

	/* Update contact details */
	contacts_update_details(selected_contact);
}

/**
 * \brief Search entry changed callback
 * \param entry search entry widget
 * \param user_data pointer to list box
 */
void search_entry_search_changed_cb(GtkSearchEntry *entry, gpointer user_data)
{
	/* Update contact list */
	contacts_update_list();
}

/**
 * \brief Row selected callback
 * \param box list box
 * \param row list bx row
 * \param user_data UNSUED
 */
void contacts_list_box_row_selected_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	GList *childrens;
	struct contact *contact = NULL;

	if (!contacts) {
		return;
	}

	gtk_widget_set_visible(contacts->cancel_button, FALSE);
	gtk_widget_set_visible(contacts->save_button, FALSE);
	gtk_widget_set_visible(contacts->edit_button, TRUE);

	/* Sanity check */
	if (!row) {
		return;
	}

	childrens = gtk_container_get_children(GTK_CONTAINER(row));

	/* Sanity check */
	if (!childrens || !childrens->data) {
		return;
	}

	contact = g_object_get_data(G_OBJECT(childrens->data), "contact");
	if (!contact) {
		return;
	}

	/* Update details */
	contacts_update_details(contact);
}

void refresh_edit_dialog(struct contact *contact);

void remove_phone_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct contact *contact = user_data;
	struct phone_number *number = (struct phone_number *) g_object_get_data(G_OBJECT(button), "number");

	contact->numbers = g_slist_remove(contact->numbers, number);
	refresh_edit_dialog(contact);
}

void remove_address_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct contact *contact = user_data;
	struct contact_address *address = (struct contact_address *) g_object_get_data(G_OBJECT(button), "address");

	contact->addresses = g_slist_remove(contact->addresses, address);
	refresh_edit_dialog(contact);
}

void name_entry_changed_cb(GtkWidget *entry, gpointer user_data)
{
	struct contact *contact = user_data;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	gtk_widget_set_sensitive(contacts->save_button, strlen(text) > 0);

	g_free(contact->name);
	contact->name = g_strdup(text);
}

void number_entry_changed_cb(GtkWidget *entry, gpointer user_data)
{
	struct phone_number *number = user_data;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	g_free(number->number);
	number->number = g_strdup(text);
}

void street_entry_changed_cb(GtkWidget *entry, gpointer user_data)
{
	struct contact_address *address = user_data;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	g_free(address->street);
	address->street = g_strdup(text);
}

void zip_entry_changed_cb(GtkWidget *entry, gpointer user_data)
{
	struct contact_address *address = user_data;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	g_free(address->zip);
	address->zip = g_strdup(text);
}

void city_entry_changed_cb(GtkWidget *entry, gpointer user_data)
{
	struct contact_address *address = user_data;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	g_free(address->city);
	address->city = g_strdup(text);
}

void number_type_changed_cb(GtkWidget *combobox, gpointer user_data)
{
	struct phone_number *number = user_data;

	number->type = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
}

void address_type_changed_cb(GtkWidget *combobox, gpointer user_data)
{
	struct contact_address *address = user_data;

	address->type = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
}

void photo_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkWidget *file_chooser;
	GtkFileFilter *filter;
	gint result;
	struct contact *contact = user_data;

	file_chooser = gtk_file_chooser_dialog_new(_("Open image"), (GtkWindow *) gtk_widget_get_ancestor(button, GTK_TYPE_WINDOW),
	               GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, _("_No image"), 1, NULL);

	filter = gtk_file_filter_new();

	gtk_file_filter_add_mime_type(filter, "image/gif");
	gtk_file_filter_add_mime_type(filter, "image/jpeg");
	gtk_file_filter_add_mime_type(filter, "image/png");
	gtk_file_filter_add_mime_type(filter, "image/tiff");
	gtk_file_filter_add_mime_type(filter, "image/ief");
	gtk_file_filter_add_mime_type(filter, "image/cgm");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(file_chooser), filter);

	result = gtk_dialog_run(GTK_DIALOG(file_chooser));

	if (result == GTK_RESPONSE_ACCEPT) {
		if (contact->image_uri != NULL) {
			g_free(contact->image_uri);
			contact->image_uri = NULL;
		}
		contact->image_uri = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
		if (contact->image_uri != NULL) {
			contact->image = gdk_pixbuf_new_from_file(contact->image_uri, NULL);
		}
	} else if (result == 1) {
		if (contact->image != NULL) {
			contact->image = NULL;
		}
	}
	refresh_edit_dialog(contact);

	gtk_widget_destroy(file_chooser);
}

void contact_add_number(struct contact *contact, gchar *number)
{
	/* Add phone number */
	struct phone_number *phone_number;

	phone_number = g_slice_new(struct phone_number);
	phone_number->type = PHONE_NUMBER_HOME;
	phone_number->number = g_strdup(number);

	contact->numbers = g_slist_prepend(contact->numbers, phone_number);
}

void contact_add_address(struct contact *contact, gchar *street, gchar *zip, gchar *city)
{
	/* Add address */
	struct contact_address *address;

	address = g_slice_new(struct contact_address);
	address->type = 0;
	address->street = g_strdup(street);
	address->zip = g_strdup(zip);
	address->city = g_strdup(city);

	contact->addresses = g_slist_prepend(contact->addresses, address);
}

void contacts_add_detail(gchar *detail)
{
	if (!strncmp(detail, "phone-", 6)) {
		/* Add phone number */
		struct phone_number *phone_number;

		phone_number = g_slice_new(struct phone_number);

		if (!strcmp(detail, "phone-home")) {
			phone_number->type = PHONE_NUMBER_HOME;
		} else if (!strcmp(detail, "phone-work")) {
			phone_number->type = PHONE_NUMBER_WORK;
		} else if (!strcmp(detail, "phone-mobile")) {
			phone_number->type = PHONE_NUMBER_MOBILE;
		} else if (!strcmp(detail, "phone-home-fax")) {
			phone_number->type = PHONE_NUMBER_FAX_HOME;
		} else if (!strcmp(detail, "phone-work-fax")) {
			phone_number->type = PHONE_NUMBER_FAX_WORK;
		} else if (!strcmp(detail, "phone-pager")) {
			phone_number->type = PHONE_NUMBER_PAGER;
		}

		phone_number->number = g_strdup("");

		contacts->tmp_contact->numbers = g_slist_prepend(contacts->tmp_contact->numbers, phone_number);
	} if (!strncmp(detail, "address-", 8)) {
		/* Add address */
		struct contact_address *address;

		address = g_slice_new(struct contact_address);

		if (!strcmp(detail, "address-home")) {
			address->type = 0;
		} else {
			address->type = 1;
		}
		address->street = g_strdup("");
		address->zip = g_strdup("");
		address->city = g_strdup("");

		contacts->tmp_contact->addresses = g_slist_prepend(contacts->tmp_contact->addresses, address);
	}

	refresh_edit_dialog(contacts->tmp_contact);
}

extern GSettings *app_settings;

void refresh_edit_dialog(struct contact *contact)
{
	GSList *numbers;
	GSList *addresses;
	GtkWidget *photo_button;
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *scrolled;
	gint detail_row = 1;
	GtkWidget *detail_photo_image = NULL;
	GtkWidget *detail_name_label = NULL;
	GtkWidget *box;
	GtkWidget *separator;

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin(box, 6, 6, 6, 6);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_NONE);
	gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 6);

	gtk_widget_set_vexpand(scrolled, TRUE);
	gtk_widget_set_hexpand(scrolled, TRUE);

	gtk_widget_set_margin(grid, 12, 12, 12, 12);

	gtk_container_add(GTK_CONTAINER(scrolled), grid);

	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	photo_button = gtk_button_new();
	detail_photo_image = gtk_image_new();
	gtk_button_set_image(GTK_BUTTON(photo_button), detail_photo_image);
	g_signal_connect(photo_button, "clicked", G_CALLBACK(photo_button_clicked_cb), contact);
	gtk_grid_attach(GTK_GRID(grid), photo_button, 0, 0, 1, 1);

	detail_name_label = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(detail_name_label), contact ? contact->name : "");
	gtk_entry_set_placeholder_text(GTK_ENTRY(detail_name_label), _("Name"));
	g_signal_connect(G_OBJECT(detail_name_label), "changed", G_CALLBACK(name_entry_changed_cb), contact);
	gtk_widget_set_hexpand(detail_name_label, TRUE);
	gtk_widget_set_valign(detail_name_label, GTK_ALIGN_CENTER);
	gtk_grid_attach(GTK_GRID(grid), detail_name_label, 1, 0, 1, 1);

	if (contact && contact->image) {
		GdkPixbuf *buf = image_get_scaled(contact->image, 96, 96);
		gtk_image_set_from_pixbuf(GTK_IMAGE(detail_photo_image), buf);
	} else {
		gtk_image_set_from_icon_name(GTK_IMAGE(detail_photo_image), AVATAR_DEFAULT, GTK_ICON_SIZE_DIALOG);
		gtk_image_set_pixel_size(GTK_IMAGE(detail_photo_image), 96);
	}

	for (numbers = contact ? contact->numbers : NULL; numbers != NULL; numbers = numbers->next) {
		GtkWidget *number;
		GtkWidget *remove;
		GtkWidget *phone_image;
		struct phone_number *phone_number = numbers->data;
		GtkWidget *type_box;

		type_box = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Home"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Work"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Mobile"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Home Fax"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Work Fax"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Pager"));

		g_signal_connect(G_OBJECT(type_box), "changed", G_CALLBACK(number_type_changed_cb), phone_number);
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_box), phone_number->type);
		gtk_grid_attach(GTK_GRID(grid), type_box, 0, detail_row, 1, 1);

		number = gtk_entry_new();
		gtk_entry_set_placeholder_text(GTK_ENTRY(number), _("Number"));
		gtk_grid_attach(GTK_GRID(grid), number, 1, detail_row, 1, 1);
		gtk_entry_set_text(GTK_ENTRY(number), phone_number->number);
		g_signal_connect(G_OBJECT(number), "changed", G_CALLBACK(number_entry_changed_cb), phone_number);

		remove = gtk_button_new();
		gtk_widget_set_tooltip_text(remove, _("Remove number"));

		phone_image = gtk_image_new_from_icon_name(APP_ICON_TRASH, GTK_ICON_SIZE_BUTTON); 
		gtk_button_set_image(GTK_BUTTON(remove), phone_image);
		g_signal_connect(remove, "clicked", G_CALLBACK(remove_phone_clicked_cb), contact);
		g_object_set_data(G_OBJECT(remove), "number", phone_number);
		gtk_grid_attach(GTK_GRID(grid), remove, 2, detail_row, 1, 1);
		detail_row++;
	}

	for (addresses = contact ? contact->addresses : NULL; addresses != NULL; addresses = addresses->next) {
		struct contact_address *address = addresses->data;
		GtkWidget *street = gtk_entry_new();
		GtkWidget *zip = gtk_entry_new();
		GtkWidget *city = gtk_entry_new();
		GtkWidget *remove;
		GtkWidget *phone_image;
		GtkWidget *type_box;

		type_box = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Home"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Work"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_box), address->type);
		g_signal_connect(G_OBJECT(type_box), "changed", G_CALLBACK(address_type_changed_cb), address);
		gtk_grid_attach(GTK_GRID(grid), type_box, 0, detail_row, 1, 1);

		remove = gtk_button_new();
		gtk_widget_set_tooltip_text(remove, _("Remove address"));
		phone_image = gtk_image_new_from_icon_name("user-trash-symbolic", GTK_ICON_SIZE_BUTTON);
		phone_image = gtk_image_new_from_icon_name(APP_ICON_TRASH, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON(remove), phone_image);
		g_signal_connect(remove, "clicked", G_CALLBACK(remove_address_clicked_cb), contact);
		g_object_set_data(G_OBJECT(remove), "address", address);
		gtk_grid_attach(GTK_GRID(grid), remove, 2, detail_row, 1, 1);

		gtk_entry_set_placeholder_text(GTK_ENTRY(street), _("Street"));
		gtk_entry_set_placeholder_text(GTK_ENTRY(zip), _("ZIP"));
		gtk_entry_set_placeholder_text(GTK_ENTRY(city), _("City"));

		gtk_grid_attach(GTK_GRID(grid), street, 1, detail_row, 1, 1);
		detail_row++;
		gtk_grid_attach(GTK_GRID(grid), zip, 1, detail_row, 1, 1);
		detail_row++;
		gtk_grid_attach(GTK_GRID(grid), city, 1, detail_row, 1, 1);
		detail_row++;

		gtk_entry_set_text(GTK_ENTRY(street), address->street);
		gtk_entry_set_text(GTK_ENTRY(zip), address->zip);
		gtk_entry_set_text(GTK_ENTRY(city), address->city);

		g_signal_connect(street, "changed", G_CALLBACK(street_entry_changed_cb), address);
		g_signal_connect(zip, "changed", G_CALLBACK(zip_entry_changed_cb), address);
		g_signal_connect(city, "changed", G_CALLBACK(city_entry_changed_cb), address);
	}

	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(box), separator, FALSE, FALSE, 0);

	GtkWidget *add_detail_button = gtk_menu_button_new ();

	GMenu *menu = g_menu_new();

	g_menu_append(menu, _("Home phone"), "app.contacts-edit-phone-home");
	g_menu_append(menu, _("Work phone"), "app.contacts-edit-phone-work");
	g_menu_append(menu, _("Mobile phone"), "app.contacts-edit-phone-mobile");
	g_menu_append(menu, _("Home Fax"), "app.contacts-edit-phone-home-fax");
	g_menu_append(menu, _("Work Fax"), "app.contacts-edit-phone-work-fax");
	g_menu_append(menu, _("Pager"), "app.contacts-edit-phone-pager");
	g_menu_append(menu, _("Home address"), "app.contacts-edit-address-home");
	g_menu_append(menu, _("Work address"), "app.contacts-edit-address-work");
	g_menu_freeze(menu);

#if GTK_CHECK_VERSION(3,12,0)
	gtk_menu_button_set_use_popover(GTK_MENU_BUTTON(add_detail_button), TRUE);
#endif
	GtkWidget *detail_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start(GTK_BOX(detail_box), gtk_label_new(_("Add detail")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(detail_box), gtk_image_new_from_icon_name("pan-up-symbolic", GTK_ICON_SIZE_MENU), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(add_detail_button), detail_box);
	gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(add_detail_button), G_MENU_MODEL(menu));
 
	gtk_widget_set_halign(add_detail_button, GTK_ALIGN_START);

	gtk_box_pack_end(GTK_BOX(box), add_detail_button, FALSE, FALSE, 0);

	if (contacts->active_user_widget) {
		gtk_container_remove(GTK_CONTAINER(contacts->view_port), contacts->active_user_widget);
	}

	contacts->active_user_widget = box;
	gtk_widget_show_all(contacts->active_user_widget);

	gtk_container_add(GTK_CONTAINER(contacts->view_port), contacts->active_user_widget);
}

void contacts_cancel_button_clicked_cb(GtkComboBox *box, gpointer user_data)
{
	struct contact *contact;

	contact = contacts_get_selected_contact();

	gtk_widget_set_visible(contacts->cancel_button, FALSE);
	gtk_widget_set_visible(contacts->save_button, FALSE);
	gtk_widget_set_visible(contacts->edit_button, TRUE);

	if (contacts->tmp_contact) {
		contact_free(contacts->tmp_contact);
		contacts->tmp_contact = NULL;
	}

	/* Update contact details */
	contacts_update_details(contact);
}

void contacts_save_button_clicked_cb(GtkComboBox *box, gpointer user_data)
{
	struct contact *contact;
	gboolean ok = g_settings_get_boolean(app_settings, "contacts-hide-warning");

	contact = contacts_get_selected_contact();

	gtk_widget_set_visible(contacts->cancel_button, FALSE);
	gtk_widget_set_visible(contacts->save_button, FALSE);
	gtk_widget_set_visible(contacts->edit_button, TRUE);

	if (!ok) {
		gboolean use_header = roger_uses_headerbar();
		gint response;

		GtkWidget *info_dialog = gtk_message_dialog_new(GTK_WINDOW(contacts->window), use_header ? GTK_DIALOG_USE_HEADER_BAR : 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK_CANCEL, _("Note: Depending on the address book plugin not all information might be saved"));
		GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(info_dialog));
		GtkWidget *check_button = gtk_check_button_new_with_label(_("Do not show again"));

		g_settings_bind(app_settings, "contacts-hide-warning", check_button, "active", G_SETTINGS_BIND_DEFAULT);
		gtk_widget_set_halign(check_button, GTK_ALIGN_CENTER);
		gtk_widget_show(check_button);
		gtk_container_add(GTK_CONTAINER(content), check_button);

		response = gtk_dialog_run(GTK_DIALOG(info_dialog));
		gtk_widget_destroy(info_dialog);

		if (response == GTK_RESPONSE_OK) {
			ok = TRUE;
		}
	}

	if (ok) {
		if (contact) {
			contact_copy(contacts->tmp_contact, contact);
			address_book_save_contact(contact);
		} else {
			address_book_save_contact(contacts->tmp_contact);
		}

		address_book_reload_contacts();
	}

	if (contacts->tmp_contact) {
		contact_free(contacts->tmp_contact);
		contacts->tmp_contact = NULL;
	}

	/* Update contact list */
	contacts_update_list();
}

void contact_editor(struct contact *contact)
{
	contacts->tmp_contact = contact_dup(contact);

	gtk_widget_set_visible(contacts->cancel_button, TRUE);
	gtk_widget_set_visible(contacts->save_button, TRUE);
	gtk_widget_set_visible(contacts->edit_button, FALSE);

	if (contacts->new_contact) {
		struct phone_number *phone_number = g_malloc0(sizeof(struct phone_number));

		phone_number->number = g_strdup(contacts->new_contact->number);

		contacts->tmp_contact->numbers = g_slist_append(contacts->tmp_contact->numbers, phone_number);

		if (EMPTY_STRING(contacts->tmp_contact->name) && !EMPTY_STRING(contacts->new_contact->name)) {
			contacts->tmp_contact->name = g_strdup(contacts->new_contact->name);
		}
		if (!contacts->tmp_contact->addresses && !EMPTY_STRING(contacts->new_contact->street)) {
			struct contact_address *address = g_malloc(sizeof(struct contact_address));

			address->type = 0;
			address->street = g_strdup(contacts->new_contact->street);
			address->city = g_strdup(contacts->new_contact->city);
			address->zip = g_strdup(contacts->new_contact->zip);

			contacts->tmp_contact->addresses = g_slist_append(contacts->tmp_contact->addresses, address);
		}
	}

	refresh_edit_dialog(contacts->tmp_contact);
}

/**
 * \brief Add button clicked callback
 * \param button add button widget
 * \param user_data listbox widget
 */
void add_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct contact *contact;

	/* Create a new contact */
	contact = g_slice_new0(struct contact);
	contact->name = g_strdup("");

	gtk_label_set_text(GTK_LABEL(contacts->header_bar_title), _("New contact"));

	/* Open contact editor with new contact */
	contact_editor(contact);
}

/**
 * \brief Edit button clicked callback
 * \param button edit button widget
 * \param user_data list box widget
 */
void edit_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct contact *contact;

	contact = contacts_get_selected_contact();
	if (!contact) {
		return;
	}

	/* Open contact editor */
	contact_editor(contact);
}

/**
 * \brief Remove button clicked callback
 * \param button remove button widget
 * \param user_data list box widget
 */
void remove_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkWidget *dialog;
	GtkWidget *remove_button;
	struct contact *contact;
	gint result;

	contact = contacts_get_selected_contact();

	/* Sanity check #3 */
	if (!contact) {
		return;
	}

	/* Create dialog */
	dialog = gtk_message_dialog_new(GTK_WINDOW(contacts->window), GTK_DIALOG_USE_HEADER_BAR | GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("Do you want to delete contact '%s'?"), contact->name);

	/* Add cancel button */
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);

	/* Add remove button */
	remove_button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Delete"), GTK_RESPONSE_OK);
	ui_set_destructive_style(remove_button);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if (result == GTK_RESPONSE_OK) {
		/* Remove selected contact */
		address_book_remove_contact(contact);

		/* Update contact list */
		contacts_update_list();
	}
}

gboolean contacts_window_delete_event_cb(GtkWidget *widget, GdkEvent event, gpointer data)
{
	contacts->window = NULL;
	contacts->active_user_widget = NULL;

	if (contacts->new_contact) {
		contact_free(contacts->new_contact);
		contacts->new_contact = NULL;
	}

	g_free(contacts);
	contacts = NULL;

	return FALSE;
}

static void contacts_contacts_changed_cb(AppObject *object, gpointer user_data)
{
	/* Update contact list */
	contacts_update_list();
}

void contacts_set_contact(struct contacts *contacts, struct contact *contact)
{
	if (contacts->new_contact) {
		contact_free(contacts->new_contact);
		contacts->new_contact = NULL;
	}

	if (!contact) {
		return;
	}

	contacts->new_contact = contact_dup(contact);
}

/**
 * \brief Contacts window
 */
void app_contacts(struct contact *contact)
{
	GtkWidget *header_bar;
	GtkBuilder *builder;
	GtkWidget *contacts_header_bar_left;
	GtkWidget *parent;
	GtkWidget *placeholder_image;

	/* Only allow one contact window at a time */
	if (contacts) {
		gtk_window_present(GTK_WINDOW(contacts->window));
		return;
	}

	builder = gtk_builder_new_from_resource("/org/tabos/roger/contacts.glade");
	if (!builder) {
		g_warning("Could not load contacts ui");
		return;
	}

	contacts = g_malloc0(sizeof(struct contacts));
	contacts_set_contact(contacts, contact);

	parent = journal_get_window();

	contacts->window = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_window"));
	gtk_window_set_transient_for(GTK_WINDOW(contacts->window), parent ? GTK_WINDOW(parent) : NULL);

	header_bar = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_header_bar"));
	contacts->list_box = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_list_box"));
	contacts->search_entry = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_search_entry"));
	contacts->edit_button = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_edit_button"));
	contacts->cancel_button = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_cancel_button"));
	contacts->save_button = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_save_button"));
	ui_set_suggested_style(contacts->save_button);
	contacts->add_button = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_add_button"));
	contacts->remove_button = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_remove_button"));
	contacts->view_port = GTK_WIDGET(gtk_builder_get_object(builder, "view_port"));
	contacts_header_bar_left = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_header_bar_left"));
	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(contacts_header_bar_left), address_book_get_name());

	contacts->header_bar_title = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_header_bar_label"));
	placeholder_image = GTK_WIDGET(gtk_builder_get_object(builder, "placeholder_image"));

	gtk_image_set_from_pixbuf(GTK_IMAGE(placeholder_image), gdk_pixbuf_new_from_resource_at_scale("/org/tabos/roger/images/address-book.svg", 128, 128, TRUE, NULL));

	contacts->details_placeholder_box = GTK_WIDGET(gtk_builder_get_object(builder, "details_placeholder_box"));

	if (roger_uses_headerbar()) {
		GtkWidget *contacts_header_bar_right = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_header_bar_right"));
		gtk_window_set_titlebar(GTK_WINDOW(contacts->window), header_bar);
		gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(contacts_header_bar_right), TRUE);

		gchar *css_data = g_strdup_printf(".round-corner { border-top-right-radius: 7px; }");
		GtkCssProvider *css_provider = gtk_css_provider_get_default();
		gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
		g_free(css_data);

		GtkStyleContext *style_context =  gtk_widget_get_style_context(contacts_header_bar_right);
		gtk_style_context_add_provider(style_context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
		gtk_style_context_add_class(style_context, "round-corner");

	} else {
		GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(builder, "contacts_window_grid"));

		gtk_window_set_title(GTK_WINDOW(contacts->window), _("Contacts"));
		gtk_grid_attach(GTK_GRID(grid), header_bar, 0, 0, 3, 1);
	}

	/* Only set buttons to sensitive if we can write to the selected book */
	if (!address_book_can_save()) {
		gtk_widget_set_sensitive(contacts->add_button, FALSE);
		gtk_widget_set_sensitive(contacts->remove_button, FALSE);
		gtk_widget_set_sensitive(contacts->edit_button, FALSE);
	}

	/* Update contact list */
	contacts_update_list();

	gtk_builder_connect_signals(builder, NULL);

	g_signal_connect(app_object, "contacts-changed", G_CALLBACK(contacts_contacts_changed_cb), NULL);

	/* Show window */
	gtk_widget_show_all(contacts->window);

	extern GtkApplication *roger_app;
	gtk_application_add_window(roger_app, GTK_WINDOW(contacts->window));

	g_object_unref(G_OBJECT(builder));
}
