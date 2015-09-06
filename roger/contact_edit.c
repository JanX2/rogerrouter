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

#include <string.h>
#include <gtk/gtk.h>

#include <libroutermanager/address-book.h>
#include <libroutermanager/contact.h>
#include <libroutermanager/router.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/call.h>
#include <libroutermanager/appobject-emit.h>
#include <roger/contacts.h>
#include <roger/main.h>
#include <roger/uitools.h>
#include <roger/phone.h>
#include <roger/icons.h>
#include <roger/journal.h>

static GtkWidget *edit_dialog = NULL;
static GtkWidget *edit_widget = NULL;

void add_detail_button_clicked_cb(GtkComboBox *box, gpointer user_data);
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

	gtk_dialog_set_response_sensitive(GTK_DIALOG(edit_dialog), GTK_RESPONSE_ACCEPT, strlen(text) > 0);

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
	GtkWidget *content;

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_vexpand(scrolled, TRUE);
	gtk_widget_set_hexpand(scrolled, TRUE);

	gtk_widget_set_margin(grid, 18, 18, 18, 18);

	gtk_container_add(GTK_CONTAINER(scrolled), grid);

	gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
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
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Private"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Business"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Mobile"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Private Fax"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Business Fax"));
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
		//phone_image = gtk_image_new_from_icon_name("user-trash-symbolic", GTK_ICON_SIZE_BUTTON);
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
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Private"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Business"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_box), address->type);
		g_signal_connect(G_OBJECT(type_box), "changed", G_CALLBACK(address_type_changed_cb), address);
		gtk_grid_attach(GTK_GRID(grid), type_box, 0, detail_row, 1, 1);

		remove = gtk_button_new();
		gtk_widget_set_tooltip_text(remove, _("Remove address"));
		//phone_image = gtk_image_new_from_icon_name("user-trash-symbolic", GTK_ICON_SIZE_BUTTON);
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

	GtkWidget *add_detail_button = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(add_detail_button), _("Add detail"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(add_detail_button), _("Phone"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(add_detail_button), _("Address"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(add_detail_button), 0);
	g_signal_connect(add_detail_button, "changed", G_CALLBACK(add_detail_button_clicked_cb), contact);
	gtk_grid_attach(GTK_GRID(grid), add_detail_button, 0, detail_row, 1, 1);

	content = gtk_dialog_get_content_area(GTK_DIALOG(edit_dialog));
	if (edit_widget) {
		gtk_widget_destroy(edit_widget);
	}

	edit_widget = scrolled;

	gtk_container_add(GTK_CONTAINER(content), scrolled);
	gtk_widget_show_all(scrolled);
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

void add_detail_button_clicked_cb(GtkComboBox *box, gpointer user_data)
{
	gint active = gtk_combo_box_get_active(box);
	struct contact *contact = user_data;

	if (active == 1) {
		/* Add phone number */
		struct phone_number *phone_number;

		phone_number = g_slice_new(struct phone_number);
		phone_number->type = PHONE_NUMBER_HOME;
		phone_number->number = g_strdup("");

		contact->numbers = g_slist_prepend(contact->numbers, phone_number);

		refresh_edit_dialog(contact);
	} else if (active == 2) {
		/* Add address */
		struct contact_address *address;

		address = g_slice_new(struct contact_address);
		address->type = 0;
		address->street = g_strdup("");
		address->zip = g_strdup("");
		address->city = g_strdup("");

		contact->addresses = g_slist_prepend(contact->addresses, address);

		refresh_edit_dialog(contact);
	} else {
		refresh_edit_dialog(contact);
	}
}

extern GSettings *app_settings;

void contact_editor(struct contact *contact, GtkWidget *parent)
{
	GtkWidget *dialog;
	gboolean use_header = roger_uses_headerbar();

	dialog = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", use_header, NULL);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Contact"));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
	GtkWidget *save = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Save"), GTK_RESPONSE_OK);

	ui_set_suggested_style(save);

	edit_dialog = dialog;

	refresh_edit_dialog(contact);

	gtk_window_set_transient_for(GTK_WINDOW(edit_dialog), GTK_WINDOW(parent));

	gtk_widget_set_size_request(edit_dialog, 500, 500);
	int response = gtk_dialog_run(GTK_DIALOG(edit_dialog));
	gtk_widget_destroy(edit_dialog);

	edit_dialog = NULL;
	edit_widget = NULL;

	if (response == GTK_RESPONSE_OK) {
		gboolean ok = g_settings_get_boolean(app_settings, "contacts-hide-warning");

		if (!ok) {
			GtkWidget *info_dialog = gtk_message_dialog_new(GTK_WINDOW(parent), use_header ? GTK_DIALOG_USE_HEADER_BAR : 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK_CANCEL, _("Note: Depending on the address book plugin not all information might be saved"));
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
			address_book_save_contact(contact);
		}

		address_book_reload_contacts();
	}
}
