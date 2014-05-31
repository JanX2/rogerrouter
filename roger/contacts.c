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
#include <libroutermanager/number.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/call.h>
#include <libroutermanager/appobject-emit.h>
#include <roger/contacts.h>
#include <roger/contact_edit.h>
#include <roger/main.h>
#include <roger/uitools.h>
#include <roger/phone.h>
#include <roger/icons.h>

GdkPixbuf *image_get_scaled(GdkPixbuf *image, gint req_width, gint req_height);
void contacts_fill_list(GtkListStore *list_store, const gchar *text);

static GtkWidget *detail_grid = NULL;
static GtkWidget *contacts_window = NULL;
static GtkWidget *contacts_window_grid = NULL;

void dial_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct contact contact_s;
	struct contact *contact = &contact_s;
	gchar *full_number;
	gchar *number = user_data;

	full_number = call_full_number(number, FALSE);

	/** Ask for contact information */
	memset(contact, 0, sizeof(struct contact));
	contact_s.number = full_number;
	emit_contact_process(contact);

	app_show_phone_window(contact);
	g_free(full_number);
}

void contacts_clear_details(void)
{
	if (detail_grid) {
		gtk_container_remove(GTK_CONTAINER(contacts_window_grid), detail_grid);
		detail_grid = NULL;
	}
}

void contacts_update_details(struct contact *contact)
{
	GtkWidget *detail_photo_image = NULL;
	GtkWidget *detail_name_label = NULL;
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
		detail_photo_image = gtk_image_new();
		gtk_grid_attach(GTK_GRID(grid), detail_photo_image, 0, 0, 1, 1);

		detail_name_label = gtk_label_new("");
		gtk_label_set_ellipsize(GTK_LABEL(detail_name_label), PANGO_ELLIPSIZE_END);
		gtk_widget_set_halign(detail_name_label, GTK_ALIGN_START);
		gtk_widget_set_hexpand(detail_name_label, TRUE);
		gtk_grid_attach(GTK_GRID(grid), detail_name_label, 1, 0, 1, 1);

		GdkPixbuf *buf = image_get_scaled(contact->image, 96, 96);
		gtk_image_set_from_pixbuf(GTK_IMAGE(detail_photo_image), buf);

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
	gtk_grid_attach(GTK_GRID(contacts_window_grid), detail_grid, 1, 0, 1, 3);
}

static void search_entry_changed(GtkEditable *entry, gpointer user_data)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	if (!EMPTY_STRING(text)) {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "edit-clear-symbolic");
	} else {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, NULL);
	}

	GtkTreeView *view = user_data;
	GtkListStore *list_store = GTK_LIST_STORE(gtk_tree_view_get_model(view));

	gtk_list_store_clear(list_store);
	contacts_fill_list(list_store, text);
}

static void entry_icon_released(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
	switch (icon_pos) {
	case GTK_ENTRY_ICON_PRIMARY:
		break;
	case GTK_ENTRY_ICON_SECONDARY:
		gtk_entry_set_text(entry, "");
		break;
	}
}

void contacts_fill_list(GtkListStore *list_store, const gchar *text)
{
	GSList *list;
	GSList *contact_list = address_book_get_contacts();

	for (list = contact_list; list != NULL; list = list->next) {
		struct contact *contact = list->data;
		GtkTreeIter iter;

		if (text && (!g_strcasestr(contact->name, text) && !g_strcasestr(contact->company, text))) {
			continue;
		}

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter, 0, image_get_scaled(contact->image, -1, -1), -1);
		gtk_list_store_set(list_store, &iter, 1, contact->name, -1);
		gtk_list_store_set(list_store, &iter, 2, contact, -1);
	}
}

static gboolean contacts_list_button_press_event_cb(GtkTreeView *view, GdkEventButton *event, gpointer user_data)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;

	if (!((event->type == GDK_BUTTON_PRESS && event->button == 1) || event->type == GDK_KEY_PRESS)) {
		return FALSE;
	}

	if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
		GtkTreePath *path;

		/* Get tree path for row that was clicked */
		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(view), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all(selection);
			gtk_tree_selection_select_path(selection, path);

			model = gtk_tree_view_get_model(view);
			if (gtk_tree_model_get_iter(model, &iter, path)) {
				struct contact *contact;

				gtk_tree_model_get(model, &iter, 2, &contact, -1);
				contacts_update_details(contact);
			}
			gtk_tree_path_free(path);
		}
	}

	return FALSE;
}

static gboolean contacts_list_move_cursor_cb(GtkTreeView *view, GtkMovementStep step, gint direction, gpointer user_data)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;

	if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
		GtkTreePath *path;

		if (step == GTK_MOVEMENT_DISPLAY_LINES) {
			gtk_tree_view_get_cursor(view, &path, NULL);

			if (path == NULL) {
				return FALSE;
			}

			if (direction == 1) {
				gtk_tree_path_next(path);
			} else {
				gtk_tree_path_prev(path);
			}

			model = gtk_tree_view_get_model(view);
			if (gtk_tree_model_get_iter(model, &iter, path)) {
				struct contact *contact;

				gtk_tree_model_get(model, &iter, 2, &contact, -1);
				contacts_update_details(contact);
			}
			gtk_tree_path_free(path);
		}
	}

	return FALSE;
}

GtkWidget *contacts_list_view(GtkWidget *entry)
{
	GtkListStore *list_store;
	GtkTreeModel *tree_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *photo_column;
	GtkTreeViewColumn *name_column;
	GtkWidget *view;

	view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

	list_store = gtk_list_store_new(3,
	                                GDK_TYPE_PIXBUF,
	                                G_TYPE_STRING,
	                                G_TYPE_POINTER);

	tree_model = GTK_TREE_MODEL(list_store);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(tree_model));

	g_signal_connect(entry, "changed", G_CALLBACK(search_entry_changed), view);

	renderer = gtk_cell_renderer_pixbuf_new();
	photo_column = gtk_tree_view_column_new_with_attributes(_("Photo"), renderer, "pixbuf", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), photo_column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
	name_column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), name_column);

	g_signal_connect(view, "button-press-event", G_CALLBACK(contacts_list_button_press_event_cb), list_store);
	g_signal_connect(view, "move-cursor", (GCallback) contacts_list_move_cursor_cb, list_store);
	contacts_fill_list(list_store, NULL);

	return view;
}

void contacts_select(GtkListStore *store, struct contact *contact)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
	struct contact *iter_contact;

	while (valid) {
		gtk_tree_model_get(model, &iter, 2, &iter_contact, -1);
		if (iter_contact == contact) {
			contacts_update_details(contact);
			break;
		}
		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

void button_add_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkWidget *view = user_data;
	GtkListStore *list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view)));
	struct contact *contact = g_slice_new0(struct contact);

	contact->name = g_strdup("");
	contact_editor(contact);

	gtk_list_store_clear(list_store);
	contacts_fill_list(list_store, NULL);
	contacts_select(list_store, contact);
	contacts_clear_details();
}

void button_edit_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *view = user_data;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		struct contact *contact;
		GtkListStore *list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view)));

		gtk_tree_model_get(model, &iter, 2, &contact, -1);
		contact_editor(contact);

		gtk_list_store_clear(list_store);
		contacts_fill_list(list_store, NULL);
		contacts_clear_details();
		contacts_select(list_store, contact);
	}
}

void button_remove_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkWidget *view = user_data;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GValue name = {0};
		struct contact *contact;
		GtkListStore *list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view)));

		gtk_tree_model_get_value(model, &iter, 1, &name);
		gtk_tree_model_get(model, &iter, 2, &contact, -1);

		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Do you want to delete the entry '%s'?"), g_value_get_string(&name));
		gtk_window_set_title(GTK_WINDOW(dialog), _("Delete entry"));
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
		gint result = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		if (result != GTK_RESPONSE_OK) {
			return;
		}

		address_book_remove_contact(contact);
		gtk_list_store_clear(list_store);
		contacts_fill_list(list_store, NULL);
		contacts_update_details(NULL);

		/* Remove contact */
		g_value_unset(&name);
	}
}

void button_reload_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkWidget *view = user_data;
	GtkListStore *list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view)));

	address_book_reload_contacts();

	gtk_list_store_clear(list_store);
	contacts_fill_list(list_store, NULL);
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
	GtkWidget *action_grid;
	GtkWidget *button_add;
	GtkWidget *button_remove;
	GtkWidget *button_edit;
	GtkWidget *button_reload;
	GtkWidget *contacts_view;
	GtkWidget *image;

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

	contacts_window_grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(contacts_window), contacts_window_grid);

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(contacts_window_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(contacts_window_grid), 5);

	entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(entry), "icon-release", G_CALLBACK(entry_icon_released), NULL);
	gtk_widget_set_vexpand(entry, FALSE);
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, "edit-find-symbolic");
	gtk_widget_set_margin(entry, 0, 5, 0, 5);
	gtk_grid_attach(GTK_GRID(contacts_window_grid), entry, 0, 0, 1, 1);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_widget_set_vexpand(scrolled, TRUE);

	contacts_view = contacts_list_view(entry);

	gtk_container_add(GTK_CONTAINER(scrolled), contacts_view);
	gtk_widget_set_size_request(scrolled, 300, -1);
	gtk_grid_attach(GTK_GRID(contacts_window_grid), scrolled, 0, 1, 1, 1);

	action_grid = gtk_grid_new();
	button_add = gtk_button_new();
	image = get_icon(APP_ICON_ADD, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(button_add), image);
	gtk_button_set_relief(GTK_BUTTON(button_add), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(button_add, _("Add new contact"));
	g_signal_connect(button_add, "clicked", G_CALLBACK(button_add_clicked_cb), contacts_view);
	gtk_grid_attach(GTK_GRID(action_grid), button_add, 0, 0, 1, 1);

	button_remove = gtk_button_new();
	image = get_icon(APP_ICON_REMOVE, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(button_remove), image);
	gtk_button_set_relief(GTK_BUTTON(button_remove), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(button_remove, _("Remove selected contact"));
	g_signal_connect(button_remove, "clicked", G_CALLBACK(button_remove_clicked_cb), contacts_view);
	gtk_grid_attach(GTK_GRID(action_grid), button_remove, 1, 0, 1, 1);

	button_edit = gtk_button_new();
	image = get_icon(APP_ICON_EDIT, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(button_edit), image);
	gtk_button_set_relief(GTK_BUTTON(button_edit), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(button_edit, _("Edit selected contact"));
	g_signal_connect(button_edit, "clicked", G_CALLBACK(button_edit_clicked_cb), contacts_view);
	gtk_grid_attach(GTK_GRID(action_grid), button_edit, 2, 0, 1, 1);


	button_reload = gtk_button_new();
	image = get_icon(APP_ICON_REFRESH, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(button_reload), image);
	gtk_button_set_relief(GTK_BUTTON(button_reload), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(button_reload, _("Reload address book"));
	g_signal_connect(button_reload, "clicked", G_CALLBACK(button_reload_clicked_cb), contacts_view);
	gtk_grid_attach(GTK_GRID(action_grid), button_reload, 3, 0, 1, 1);

	gtk_grid_attach(GTK_GRID(contacts_window_grid), action_grid, 0, 2, 1, 1);

	if (!address_book_can_save()) {
		gtk_widget_set_sensitive(button_add, FALSE);
		gtk_widget_set_sensitive(button_edit, FALSE);
		gtk_widget_set_sensitive(button_remove, FALSE);
	}

	g_signal_connect(contacts_window, "delete_event", G_CALLBACK(contacts_window_delete_event_cb), NULL);

	gtk_window_set_position(GTK_WINDOW(contacts_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(contacts_window), _("Contacts"));
	gtk_widget_set_size_request(contacts_window, 800, 500);
	gtk_widget_show_all(contacts_window);
}
