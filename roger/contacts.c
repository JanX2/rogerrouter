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
#include <roger/journal.h>

static GtkWidget *detail_grid = NULL;
static GtkWidget *contacts_window = NULL;
static GtkWidget *contacts_window_grid = NULL;
static GtkWidget *contacts_search_entry = NULL;

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

void contacts_update_details(struct contact *contact)
{
	GtkWidget *scrolled_window;
	GtkWidget *detail_photo_image = NULL;
	GtkWidget *detail_name_label = NULL;
	GtkWidget *frame;
	GSList *numbers;
	GSList *addresses;
	GtkWidget *grid;
	gchar *markup;
	gint detail_row = 1;

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);

	if (address_book_available()) {
		if (contact) {
			grid = gtk_grid_new();
			gtk_widget_set_margin(grid, 20, 25, 20, 25);
			gtk_container_add(GTK_CONTAINER(scrolled_window), grid);

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
				phone_image = gtk_image_new_from_icon_name(APP_ICON_CALL, GTK_ICON_SIZE_BUTTON);
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
	} else {
		GtkWidget *placeholder;

		placeholder = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(placeholder), _("<b>No address book plugin selected\nPlease activate one in the settings</b>"));
		gtk_widget_set_halign(placeholder, GTK_ALIGN_CENTER);
		gtk_widget_set_hexpand(placeholder, TRUE);
		gtk_widget_set_valign(placeholder, GTK_ALIGN_CENTER);
		gtk_widget_set_vexpand(placeholder, TRUE);

		scrolled_window = placeholder;
	}

	gtk_widget_show_all(scrolled_window);

	if (detail_grid) {
		gtk_container_remove(GTK_CONTAINER(contacts_window_grid), detail_grid);
	}

	detail_grid = scrolled_window;
	if (roger_uses_headerbar()) {
		gtk_grid_attach(GTK_GRID(contacts_window_grid), scrolled_window, 1, 0, 1, 2);
	} else {
		gtk_grid_attach(GTK_GRID(contacts_window_grid), scrolled_window, 1, 0, 1, 3);
	}
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

/**
 * \brief Update contact list (clears previous and add all matching contacts)
 * \param box listbox widget
 * \param selected_contact selected contact
 */
static void contacts_update_list(gpointer box, struct contact *selected_contact)
{
	GSList *list;
	GSList *contact_list = address_book_get_contacts();
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(contacts_search_entry));
	gint pos = 0;

	/* For all children of list box, call contacts_destory_child() */
	gtk_container_foreach(GTK_CONTAINER(box), contacts_destroy_child, NULL);

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
		gtk_list_box_insert(GTK_LIST_BOX(box), child_box, pos);

		if (selected_contact && !strcmp(selected_contact->name, contact->name)) {
			GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(box), pos);

			gtk_list_box_select_row(GTK_LIST_BOX(box), row);
		}
	}

	/* Update contact details */
	contacts_update_details(NULL);
}

/**
 * \brief Search entry changed callback
 * \param entry search entry widget
 * \param user_data pointer to list box
 */
static void search_entry_search_changed_cb(GtkSearchEntry *entry, gpointer user_data)
{
	GtkWidget *listbox = user_data;

	/* Update contact list */
	contacts_update_list(listbox, NULL);
}

/**
 * \brief Row selected callback
 * \param box list box
 * \param row list bx row
 * \param user_data UNSUED
 */
static void contacts_list_box_row_selected_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	GList *childrens;
	struct contact *contact;

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

/**
 * \brief Add button clicked callback
 * \param button add button widget
 * \param user_data listbox widget
 */
static void add_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkWidget *list_box = user_data;
	struct contact *contact;

	/* Create a new contact */
	contact = g_slice_new0(struct contact);
	contact->name = g_strdup("");

	/* Open contact editor with new contact */
	contact_editor(contact, contacts_window);

	/* Update contact list */
	contacts_update_list(list_box, contact);
}

/**
 * \brief Edit button clicked callback
 * \param button edit button widget
 * \param user_data list box widget
 */
static void edit_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(user_data));
	GtkWidget *child;
	struct contact *contact;

	/* Sanity check #1 */
	if (!row) {
		return;
	}

	/* Get first child */
	child = gtk_container_get_children(GTK_CONTAINER(row))->data;

	/* Sanity check #2 */
	if (!child) {
		return;
	}

	/* Get contact */
	contact = g_object_get_data(G_OBJECT(child), "contact");

	/* Sanity check #3 */
	if (!contact) {
		return;
	}

	/* Open contact editor */
	contact_editor(contact, contacts_window);

	/* Update contact list */
	contacts_update_list(user_data, contact);

	/* Update contact details */
	//contacts_update_details(contact);
}

/**
 * \brief Remove button clicked callback
 * \param button remove button widget
 * \param user_data list box widget
 */
static void remove_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(user_data));
	GtkWidget *child;
	GtkWidget *dialog;
	struct contact *contact;
	gint result;

	/* Sanity check #1 */
	if (!row) {
		return;
	}

	/* Get first child */
	child = gtk_container_get_children(GTK_CONTAINER(row))->data;

	/* Sanity check #2 */
	if (!child) {
		return;
	}

	/* Get contact */
	contact = g_object_get_data(G_OBJECT(child), "contact");

	/* Sanity check #3 */
	if (!contact) {
		return;
	}

	/* Create dialog */
	dialog = gtk_message_dialog_new(GTK_WINDOW(contacts_window), GTK_DIALOG_USE_HEADER_BAR | GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("Do you want to delete the entry '%s'?"), contact->name);

	/* Add cancel button */
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);

	/* Add remove button */
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Delete"), GTK_RESPONSE_OK);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if (result != GTK_RESPONSE_OK) {
		return;
	}

	/* Remove selected contact */
	address_book_remove_contact(contact);

	/* Update contact list */
	contacts_update_list(user_data, NULL);
}

static gint contacts_window_delete_event_cb(GtkWidget *widget, GdkEvent event, gpointer data)
{
	contacts_window = NULL;
	contacts_window_grid = NULL;
	detail_grid = NULL;

	return FALSE;
}

static void contacts_contacts_changed_cb(AppObject *object, gpointer user_data)
{
	GtkWidget *list_box = user_data;

	/* Update contact list */
	contacts_update_list(list_box, NULL);
}

/**
 * \brief Contacts window
 */
void contacts(void)
{
	GtkWidget *header_bar;
	GtkWidget *box;
	GtkWidget *entry;
	GtkWidget *scrolled;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *edit_button;
	GtkWidget *contacts_list_box;
	gint y = 0;

	/* Only allow one contact window at a time */
	if (contacts_window) {
		gtk_window_present(GTK_WINDOW(contacts_window));
		return;
	}

	/* Create window */
	contacts_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(contacts_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(contacts_window), _("Contacts"));
	gtk_widget_set_size_request(contacts_window, 850, 550);
	g_signal_connect(contacts_window, "delete_event", G_CALLBACK(contacts_window_delete_event_cb), NULL);

	/* Create and add global window grid */
	contacts_window_grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(contacts_window), contacts_window_grid);

	if (roger_uses_headerbar()) {
		/* Create header bar and add it to the window */
		header_bar = gtk_header_bar_new();
		gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
		gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), _("Contacts"));
		gtk_window_set_titlebar(GTK_WINDOW(contacts_window), header_bar);
	} else {
		header_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
		gtk_grid_attach(GTK_GRID(contacts_window_grid), header_bar, 0, y++, 1, 1);
	}

	/* Create button box as raised and linked */
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(header_bar), box);
	gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_RAISED);
	gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_LINKED);

	/* Add button */
	add_button = gtk_button_new_from_icon_name("list-add-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text(add_button, _("Add new contact"));
	gtk_box_pack_start(GTK_BOX(box), add_button, FALSE, FALSE, 0);

	/* Remove button */
	remove_button = gtk_button_new_from_icon_name("list-remove-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text(remove_button, _("Remove selected contact"));
	gtk_box_pack_start(GTK_BOX(box), remove_button, FALSE, FALSE, 0);

	/* Edit button */
	edit_button = gtk_button_new_with_label(_("Edit"));
	gtk_widget_set_tooltip_text(edit_button, _("Edit selected contact"));
	gtk_box_pack_end(GTK_BOX(box), edit_button, TRUE, TRUE, 0);

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(contacts_window_grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(contacts_window_grid), 6);

	/* Create search entry */
	entry = gtk_search_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(entry), _("Type to search"));
	gtk_widget_set_vexpand(entry, FALSE);
	gtk_widget_set_margin(entry, 6, 6, 6, 0);
	gtk_grid_attach(GTK_GRID(contacts_window_grid), entry, 0, y++, 1, 1);
	contacts_search_entry = entry;

	/* Create scrolled window for contact list */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_widget_set_vexpand(scrolled, TRUE);
	gtk_grid_attach(GTK_GRID(contacts_window_grid), scrolled, 0, y++, 1, 1);

	/* Request a width of 300px */
	gtk_widget_set_size_request(scrolled, 300, -1);

	/* Create contacts list box which holds the contacts */
	contacts_list_box = gtk_list_box_new();
	GtkWidget *placeholder;
	placeholder = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(placeholder), _("<b>No contacts found</b>"));
	gtk_widget_show(placeholder);
	gtk_list_box_set_placeholder(GTK_LIST_BOX(contacts_list_box), placeholder);
	g_signal_connect(contacts_list_box, "row-selected", G_CALLBACK(contacts_list_box_row_selected_cb), NULL);
	gtk_container_add(GTK_CONTAINER(scrolled), contacts_list_box);

	/* Update contact list */
	contacts_update_list(contacts_list_box, NULL);

	/* Only set buttons to sensitive if we can write to the selected book */
	if (!address_book_can_save()) {
		gtk_widget_set_sensitive(add_button, FALSE);
		gtk_widget_set_sensitive(remove_button, FALSE);
		gtk_widget_set_sensitive(edit_button, FALSE);
	}

	/* Connect signals */
	g_signal_connect(entry, "search-changed", G_CALLBACK(search_entry_search_changed_cb), contacts_list_box);
	g_signal_connect(add_button, "clicked", G_CALLBACK(add_button_clicked_cb), contacts_list_box);
	g_signal_connect(remove_button, "clicked", G_CALLBACK(remove_button_clicked_cb), contacts_list_box);
	g_signal_connect(edit_button, "clicked", G_CALLBACK(edit_button_clicked_cb), contacts_list_box);

	g_signal_connect(app_object, "contacts-changed", G_CALLBACK(contacts_contacts_changed_cb), contacts_list_box);

	/* Show window */
	gtk_widget_show_all(contacts_window);
}
