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
#include <libroutermanager/gstring.h>
#include <libroutermanager/router.h>
#include <libroutermanager/call.h>
#include <libroutermanager/appobject-emit.h>
#include <roger/contacts.h>
#include <roger/contact_edit.h>
#include <roger/main.h>
#include <roger/uitools.h>
#include <roger/phone.h>
#include <roger/icons.h>

void contacts_fill_list(gpointer fill, const gchar *text);

static GtkWidget *detail_grid = NULL;
static GtkWidget *contacts_window = NULL;
static GtkWidget *contacts_window_grid = NULL;

void dial_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct contact *contact;
	gchar *full_number;
	gchar *number = user_data;

	full_number = call_full_number(number, FALSE);

	contact = contact_find_by_number(full_number);

	app_show_phone_window(contact, NULL);
	g_free(full_number);
}

void contacts_clear_details(void)
{
	if (detail_grid) {
		gtk_container_remove(GTK_CONTAINER(contacts_window_grid), detail_grid);
		detail_grid = NULL;
	}
}

void contacts_clear_cb(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(widget);
}

void contacts_clear(GtkWidget *box)
{
	gtk_container_foreach(GTK_CONTAINER(box), contacts_clear_cb, NULL);
}

void contacts_update_details(struct contact *contact)
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
	gtk_widget_set_margin(grid, 20, 25, 20, 25);

	if (contact) {
		gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
		gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

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
			phone_image = get_icon(APP_ICON_CALL, GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image(GTK_BUTTON(dial), phone_image);
			gtk_button_set_relief(GTK_BUTTON(dial), GTK_RELIEF_NONE);
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
	}
	gtk_widget_show_all(grid);

	if (detail_grid) {
		gtk_container_remove(GTK_CONTAINER(contacts_window_grid), detail_grid);
	}

	detail_grid = grid;
	gtk_grid_attach(GTK_GRID(contacts_window_grid), detail_grid, 1, 0, 1, 2);
}

static void search_entry_search_changed_cb(GtkSearchEntry *entry, gpointer user_data)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	contacts_clear(user_data);
	contacts_fill_list(user_data, text);
}

void contacts_fill_list(gpointer box, const gchar *text)
{
	GSList *list;
	GSList *contact_list = address_book_get_contacts();

	for (list = contact_list; list != NULL; list = list->next) {
		struct contact *contact = list->data;
		GtkWidget *child_box;
		GtkWidget *img;

		if (text && (!g_strcasestr(contact->name, text) && !g_strcasestr(contact->company, text))) {
			continue;
		}

		child_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		g_object_set_data(G_OBJECT(child_box), "contact", contact);

		if (contact->image) {
			img = gtk_image_new_from_pixbuf(image_get_scaled(contact->image, -1, -1));
		} else {
			img = gtk_image_new_from_icon_name(AVATAR_DEFAULT, GTK_ICON_SIZE_DIALOG);
		}

		gtk_box_pack_start(GTK_BOX(child_box), img, FALSE, FALSE, 5);

		GtkWidget *txt = gtk_label_new(contact->name);
		gtk_box_pack_start(GTK_BOX(child_box), txt, FALSE, FALSE, 5);
		gtk_widget_show_all(child_box);

		gtk_list_box_insert(GTK_LIST_BOX(box), child_box, -1);
	}
}

void contacts_list_row_selected_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	GList *childrens = gtk_container_get_children(GTK_CONTAINER(row));
	GtkWidget *child;
	struct contact *contact;

	if (!childrens || !childrens->data) {
		return;
	}

	child = childrens->data;

	contact = g_object_get_data(G_OBJECT(child), "contact");

	if (!contact) {
		return;
	}

	contacts_update_details(contact);
}

void button_add_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkWidget *view = user_data;
	struct contact *contact = g_slice_new0(struct contact);

	contact->name = g_strdup("");
	contact_editor(contact, contacts_window);

	contacts_clear(view);
	contacts_fill_list(view, NULL);
	contacts_clear_details();
}

void button_edit_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(user_data));
	GtkWidget *child;
	struct contact *contact;

	if (!row) {
		return;
	}

	child = gtk_container_get_children(GTK_CONTAINER(row))->data;
	if (!child) {
		return;
	}

	contact = g_object_get_data(G_OBJECT(child), "contact");
	if (!contact) {
		return;
	}

	contact_editor(contact, contacts_window);

	contacts_clear(user_data);

	contacts_fill_list(user_data, NULL);
	contacts_clear_details();

	contacts_update_details(contact);
}

void button_remove_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(user_data));
	GtkWidget *child = gtk_container_get_children(GTK_CONTAINER(row))->data;
	struct contact *contact = g_object_get_data(G_OBJECT(child), "contact");
	GtkWidget *dialog;
	gchar *tmp;

	dialog = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, NULL);

	gtk_window_set_title(GTK_WINDOW(dialog), _("Contact"));
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(contacts_window));
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
	GtkWidget *remove = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Delete"), GTK_RESPONSE_OK);
	gtk_style_context_add_class(gtk_widget_get_style_context(remove), GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);
	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	tmp = g_strdup_printf(_("Do you want to delete the entry '%s'?"), contact->name);
	GtkWidget *label = gtk_label_new(tmp);
	gtk_container_add(GTK_CONTAINER(content), label);

	gtk_widget_set_margin_start(label, 18);
	gtk_widget_set_margin_end(label, 18);
	gtk_widget_set_margin_top(label, 18);
	gtk_widget_set_margin_bottom(label, 18);
	gtk_widget_show_all(content);
	g_free(tmp);

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if (result != GTK_RESPONSE_OK) {
		return;
	}

	address_book_remove_contact(contact);

	contacts_clear(user_data);
	contacts_fill_list(user_data, NULL);
	contacts_update_details(NULL);
}

static gint contacts_window_delete_event_cb(GtkWidget *widget, GdkEvent event, gpointer data)
{
	contacts_window = NULL;
	contacts_window_grid = NULL;
	detail_grid = NULL;

	return FALSE;
}

void contacts(void)
{
	GtkWidget *entry;
	GtkWidget *scrolled;
	GtkWidget *button_add;
	GtkWidget *button_remove;
	GtkWidget *button_edit;
	GtkWidget *contacts_view;

	if (contacts_window) {
		gtk_window_present(GTK_WINDOW(contacts_window));
		return;
	}

	/**
	 * Contacts        |                         |
	 * -------------------------------------------
	 * PICTURE <NAME>  |  <PICTURE>  <NAME>      |
	 * PICTURE <NAME>  |  <PHONE>                |
	 * ...             |  <NUMBER> <TYPE> <DIAL> |
	 * ...             |  <NUMBER> <TYPE> <DIAL> |
	 * ...             |  <NUMBER> <TYPE> <DIAL> |
	 * ...             |                         |
	 * ...             |  <ADDRESS>              |
	 * ...             |  <STREET>     <PRIVATE> |
	 * ...             |  <CITY>                 |
	 * ...             |  <ZIP>                  |
	 * -------------------------------------------
	 */

	contacts_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	GtkWidget *header = gtk_header_bar_new();
	gtk_widget_set_hexpand(header, TRUE);
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR (header), _("Contacts"));
	gtk_window_set_titlebar((GtkWindow *)(contacts_window), header);

	/* Create button box as raised and linked */
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(header), box);
	gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_RAISED);
	gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_LINKED);

	button_add = gtk_button_new_from_icon_name("list-add-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text(button_add, _("Add new contact"));
	gtk_box_pack_start(GTK_BOX(box), button_add, FALSE, TRUE, 0);

	button_remove = gtk_button_new_from_icon_name("list-remove-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text(button_remove, _("Remove selected contact"));
	gtk_box_pack_start(GTK_BOX(box), button_remove, FALSE, TRUE, 0);

	button_edit = gtk_button_new_with_label(_("Edit"));
	gtk_widget_set_tooltip_text(button_edit, _("Edit selected contact"));
	gtk_box_pack_end(GTK_BOX(box), button_edit, TRUE, TRUE, 0);

	contacts_window_grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(contacts_window), contacts_window_grid);

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(contacts_window_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(contacts_window_grid), 5);

	entry = gtk_search_entry_new();
	gtk_widget_set_vexpand(entry, FALSE);
	gtk_widget_set_margin(entry, 5, 5, 5, 0);
	gtk_grid_attach(GTK_GRID(contacts_window_grid), entry, 0, 0, 1, 1);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_widget_set_vexpand(scrolled, TRUE);

	contacts_view = gtk_list_box_new();
	g_signal_connect(contacts_view, "row-selected", (GCallback) contacts_list_row_selected_cb, NULL);

	contacts_fill_list(contacts_view, NULL);

	gtk_container_add(GTK_CONTAINER(scrolled), contacts_view);
	gtk_widget_set_size_request(scrolled, 300, -1);
	gtk_grid_attach(GTK_GRID(contacts_window_grid), scrolled, 0, 1, 1, 1);

	if (!address_book_can_save()) {
		gtk_widget_set_sensitive(button_add, FALSE);
		gtk_widget_set_sensitive(button_remove, FALSE);
	}

	g_signal_connect(entry, "search-changed", G_CALLBACK(search_entry_search_changed_cb), contacts_view);
	g_signal_connect(button_add, "clicked", G_CALLBACK(button_add_clicked_cb), contacts_view);
	g_signal_connect(button_remove, "clicked", G_CALLBACK(button_remove_clicked_cb), contacts_view);
	g_signal_connect(button_edit, "clicked", G_CALLBACK(button_edit_clicked_cb), contacts_view);
	g_signal_connect(contacts_window, "delete_event", G_CALLBACK(contacts_window_delete_event_cb), NULL);

	gtk_window_set_position(GTK_WINDOW(contacts_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(contacts_window), _("Contacts"));
	gtk_widget_set_size_request(contacts_window, 800, 500);
	gtk_widget_show_all(contacts_window);
}
