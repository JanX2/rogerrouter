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
#include <gtk/gtk.h>

#include <libroutermanager/address-book.h>
#include <libroutermanager/contact.h>
#include <libroutermanager/number.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/call.h>
#include <libroutermanager/appobject-emit.h>
#include <roger/contacts.h>
#include <roger/main.h>
#include <roger/uitools.h>
#include <roger/phone.h>

GdkPixbuf *image_get_scaled(GdkPixbuf *image, gint req_width, gint req_height);
void contacts_fill_list(GtkListStore *list_store, const gchar *text);

static GSList *contact_list = NULL;
static GtkWidget *detail_grid = NULL;
static GtkWidget *detail_photo_image = NULL;
static GtkWidget *detail_name_label = NULL;
static gint detail_row = 1;
static GtkWidget *contacts_window = NULL;
static GtkWidget *contacts_window_grid;
static GtkWidget *view = NULL;

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

void contacts_update_details(struct contact *contact)
{
	GSList *numbers;
	GtkWidget *grid = gtk_grid_new();
	gchar *markup;

	gtk_widget_set_margin_left(grid, 25);
	gtk_widget_set_margin_top(grid, 25);
	gtk_widget_set_margin_right(grid, 25);
	gtk_widget_set_margin_bottom(grid, 25);

	gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
	detail_photo_image = gtk_image_new();
	gtk_grid_attach(GTK_GRID(grid), detail_photo_image, 0, 0, 1, 1);

	if (contact) {
	detail_name_label = gtk_label_new("");
	gtk_label_set_ellipsize(GTK_LABEL(detail_name_label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment(GTK_MISC(detail_name_label), 0, 0.5);
	gtk_widget_set_hexpand(detail_name_label, TRUE);
	gtk_grid_attach(GTK_GRID(grid), detail_name_label, 1, 0, 1, 1);

	GdkPixbuf *buf = image_get_scaled(contact->image, 96, 96);
	gtk_image_set_from_pixbuf(GTK_IMAGE(detail_photo_image), buf);

	markup = g_markup_printf_escaped("<span size=\"x-large\">%s</span>", contact->name);
	gtk_label_set_markup(GTK_LABEL(detail_name_label), markup);
	g_free(markup);

	detail_row = 1;

	for (numbers = contact->numbers; numbers != NULL; numbers = numbers->next) {
		GtkWidget *type;
		GtkWidget *number;
		GtkWidget *dial;
		GtkWidget *phone_image;
		struct phone_number *phone_number = numbers->data;

		switch (phone_number->type) {
			case PHONE_NUMBER_HOME:
				type = ui_label_new(_("Home"));
				break;
			case PHONE_NUMBER_WORK:
				type = ui_label_new(_("Work"));
				break;
			case PHONE_NUMBER_MOBILE:
				type = ui_label_new(_("Mobile"));
				break;
			case PHONE_NUMBER_FAX:
				type = ui_label_new(_("Fax"));
				break;
			default:
				type = ui_label_new(_("Unknown"));
		}
		number = gtk_label_new(phone_number->number);
		gtk_label_set_selectable(GTK_LABEL(number), TRUE);
		gtk_misc_set_alignment(GTK_MISC(number), 0, 0.5);
		dial = gtk_button_new();
		gtk_widget_set_tooltip_text(dial, _("Dial number"));
		phone_image = gtk_image_new_from_icon_name("call-start-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON(dial), phone_image);
		gtk_button_set_relief(GTK_BUTTON(dial), GTK_RELIEF_NONE);
		g_signal_connect(dial, "clicked", G_CALLBACK(dial_clicked_cb), phone_number->number);
		gtk_grid_attach(GTK_GRID(grid), type, 0, detail_row, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), number, 1, detail_row, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), dial, 2, detail_row, 1, 1);
		detail_row++;
	}

	if (contact->street || contact->zip || contact->city) {
		GtkWidget *address = ui_label_new(_("Address"));
		GString *addr_str = g_string_new("");
		GtkWidget *label;

		gtk_grid_attach(GTK_GRID(grid), address, 0, detail_row, 1, 1);
		if (contact->street) {
			g_string_append_printf(addr_str, "%s\n", contact->street);
		}
		if (!EMPTY_STRING(contact->zip)) {
			g_string_append_printf(addr_str, "%s ", contact->zip);
		}
		if (contact->city) {
			g_string_append_printf(addr_str, "%s ", contact->city);
		}

		label = gtk_label_new(addr_str->str);
		gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_grid_attach(GTK_GRID(grid), label, 1, detail_row, 1, 1);

		g_string_free(addr_str, TRUE);
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
#ifdef JOURNAL_TOP_OLD_ICONS
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "edit-clear");
#else
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "edit-clear-symbolic");
#endif
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

static gint contacts_window_delete_event_cb(GtkWidget *widget, GdkEvent event, gpointer data)
{
	contacts_window = NULL;
	return FALSE;
}

void contacts_fill_list(GtkListStore *list_store, const gchar *text)
{
	GSList *list;
	GSList *contact_list = address_book_get_contacts();

	for (list = contact_list; list != NULL; list = list->next) {
		struct contact *contact = list->data;
		GtkTreeIter iter;

		if (text && !g_strcasestr(contact->name, text)) {
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

GtkWidget *details_view(void)
{
	GtkWidget *detail_grid = gtk_grid_new();

	gtk_widget_set_margin_left(detail_grid, 25);
	gtk_widget_set_margin_top(detail_grid, 25);
	gtk_widget_set_margin_right(detail_grid, 25);
	gtk_widget_set_margin_bottom(detail_grid, 25);

	gtk_grid_set_row_spacing(GTK_GRID(detail_grid), 15);
	gtk_grid_set_column_spacing(GTK_GRID(detail_grid), 15);
	detail_photo_image = gtk_image_new();
	gtk_grid_attach(GTK_GRID(detail_grid), detail_photo_image, 0, 0, 1, 1);

	detail_name_label = gtk_label_new("");
	gtk_label_set_ellipsize(GTK_LABEL(detail_name_label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment(GTK_MISC(detail_name_label), 0, 0.5);
	gtk_widget_set_hexpand(detail_name_label, TRUE);
	gtk_grid_attach(GTK_GRID(detail_grid), detail_name_label, 1, 0, 1, 1);

	return detail_grid;
}

static GtkWidget *edit_dialog;
static GtkWidget *edit_grid = NULL;
void add_detail_button_clicked_cb(GtkComboBox *box, gpointer user_data);

void refresh_edit_dialog(struct contact *contact)
{
	GSList *numbers;
	GtkWidget *photo_button;
	GtkWidget *grid = gtk_grid_new();

	gtk_widget_set_margin_left(grid, 25);
	gtk_widget_set_margin_top(grid, 15);
	gtk_widget_set_margin_right(grid, 25);
	gtk_widget_set_margin_bottom(grid, 15);

	gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
	photo_button = gtk_button_new();
	detail_photo_image = gtk_image_new();
	gtk_button_set_image(GTK_BUTTON(photo_button), detail_photo_image);
	gtk_grid_attach(GTK_GRID(grid), photo_button, 0, 0, 1, 1);

	detail_name_label = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(detail_name_label), contact ? contact->name : "");
	gtk_widget_set_hexpand(detail_name_label, TRUE);
	gtk_grid_attach(GTK_GRID(grid), detail_name_label, 1, 0, 1, 1);

	GdkPixbuf *buf = image_get_scaled(contact ? contact->image : NULL, 96, 96);
	gtk_image_set_from_pixbuf(GTK_IMAGE(detail_photo_image), buf);

	detail_row = 1;

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
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Fax"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Other"));

		gtk_combo_box_set_active(GTK_COMBO_BOX(type_box), phone_number->type);

		number = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(number), phone_number->number);

		remove = gtk_button_new();
		gtk_widget_set_tooltip_text(remove, _("Remove number"));
		phone_image = gtk_image_new_from_icon_name("user-trash-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON(remove), phone_image);
		//g_signal_connect(dial, "clicked", G_CALLBACK(dial_clicked_cb), phone_number->number);
		gtk_grid_attach(GTK_GRID(grid), type_box, 0, detail_row, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), number, 1, detail_row, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), remove, 2, detail_row, 1, 1);
		detail_row++;
	}

	if (contact && (contact->street || contact->zip || contact->city)) {
		GtkWidget *street = gtk_entry_new();
		GtkWidget *zip = gtk_entry_new();
		GtkWidget *city = gtk_entry_new();
		GtkWidget *remove;
		GtkWidget *phone_image;
		GtkWidget *type_box;

		type_box = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Private"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Business"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Other"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_box), 0);

		gtk_grid_attach(GTK_GRID(grid), type_box, 0, detail_row, 1, 1);
		remove = gtk_button_new();
		gtk_widget_set_tooltip_text(remove, _("Remove address"));
		phone_image = gtk_image_new_from_icon_name("user-trash-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON(remove), phone_image);
		gtk_grid_attach(GTK_GRID(grid), remove, 2, detail_row, 1, 1);

		gtk_grid_attach(GTK_GRID(grid), street, 1, detail_row, 1, 1);
		detail_row++;
		gtk_grid_attach(GTK_GRID(grid), zip, 1, detail_row, 1, 1);
		detail_row++;
		gtk_grid_attach(GTK_GRID(grid), city, 1, detail_row, 1, 1);
		detail_row++;

		if (contact->street) {
			gtk_entry_set_text(GTK_ENTRY(street), contact->street);
		}
		if (contact->zip) {
			gtk_entry_set_text(GTK_ENTRY(zip), contact->zip);
		}
		if (contact->city) {
			gtk_entry_set_text(GTK_ENTRY(city), contact->city);
		}
	}

	GtkWidget *add_detail_button = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(add_detail_button), _("Add detail"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(add_detail_button), _("Phone"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(add_detail_button), _("Address"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(add_detail_button), 0);
	g_signal_connect(add_detail_button, "changed", G_CALLBACK(add_detail_button_clicked_cb), contact);
	gtk_grid_attach(GTK_GRID(grid), add_detail_button, 0, detail_row, 1, 1);

	gtk_widget_show_all(grid);


	if (edit_grid) {
		gtk_container_remove(GTK_CONTAINER(edit_dialog), edit_grid);
		//gtk_widget_destroy(edit_grid);
	}

	edit_grid = grid;

	gtk_container_add(GTK_CONTAINER(edit_dialog), grid);
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
		contact->street = g_strdup("");
		contact->zip = g_strdup("");
		contact->city = g_strdup("");

		refresh_edit_dialog(contact);
	}
}

void contact_edit_dialog(struct contact *contact)
{
	edit_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	refresh_edit_dialog(contact);

	gtk_window_set_position(GTK_WINDOW(edit_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(edit_dialog), contact ? _("Modify contact") : _("Add contact"));
	gtk_widget_set_size_request(edit_dialog, 500, 500);
	gtk_widget_show_all(edit_dialog);
}

void button_add_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct contact *contact = g_slice_new0(struct contact);
	contact_edit_dialog(contact);
}

void button_edit_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		struct contact *contact;
		GtkListStore *list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view)));

		gtk_tree_model_get(model, &iter, 2, &contact, -1);
		contact_edit_dialog(contact);

		gtk_list_store_clear(list_store);
		contacts_fill_list(list_store, NULL);
		contacts_update_details(NULL);
	}
}

void button_remove_clicked_cb(GtkWidget *button, gpointer user_data)
{
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
		//gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW( psAddressBook ));
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

void contacts(void)
{
	GtkWidget *entry;
	GtkWidget *scrolled;
	GtkWidget *action_grid;
	GtkWidget *button_add;
	GtkWidget *button_remove;
	GtkWidget *button_edit;
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

	contact_list = address_book_get_contacts();

	contacts_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	contacts_window_grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER (contacts_window), contacts_window_grid);

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(contacts_window_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(contacts_window_grid), 5);

	entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(entry), "icon-release", G_CALLBACK(entry_icon_released), NULL);
	gtk_widget_set_margin_left(entry, 5);
	gtk_widget_set_margin_top(entry, 5);
	gtk_widget_set_margin_right(entry, 5);
	gtk_widget_set_margin_bottom(entry, 5);
	gtk_widget_set_vexpand(entry, FALSE);
#ifdef JOURNAL_TOP_OLD_ICONS
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, "edit-find");
#else
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, "edit-find-symbolic");
#endif
	gtk_grid_attach(GTK_GRID(contacts_window_grid), entry, 0, 0, 1, 1);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_widget_set_vexpand(scrolled, TRUE);

	contacts_view = contacts_list_view(entry);

	gtk_container_add(GTK_CONTAINER(scrolled), contacts_view);
	gtk_widget_set_size_request(scrolled, 300, -1);
	gtk_grid_attach(GTK_GRID(contacts_window_grid), scrolled, 0, 1, 1, 1);

	action_grid = gtk_grid_new();
	button_add = gtk_button_new();
	image = gtk_image_new_from_icon_name("list-add-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button_add), image);
	gtk_button_set_relief(GTK_BUTTON(button_add), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(button_add, _("Add new contact"));
	g_signal_connect(button_add, "clicked", G_CALLBACK(button_add_clicked_cb), NULL);
	gtk_grid_attach(GTK_GRID(action_grid), button_add, 0, 0, 1, 1);

	button_edit = gtk_button_new();
	image = gtk_image_new_from_icon_name("document-properties-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button_edit), image);
	gtk_button_set_relief(GTK_BUTTON(button_edit), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(button_edit, _("Edit selected contact"));
	g_signal_connect(button_edit, "clicked", G_CALLBACK(button_edit_clicked_cb), NULL);
	gtk_grid_attach(GTK_GRID(action_grid), button_edit, 1, 0, 1, 1);

	button_remove = gtk_button_new();
	image = gtk_image_new_from_icon_name("list-remove-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button_remove), image);
	gtk_button_set_relief(GTK_BUTTON(button_remove), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(button_remove, _("Remove selected contact"));
	g_signal_connect(button_remove, "clicked", G_CALLBACK(button_remove_clicked_cb), NULL);
	gtk_grid_attach(GTK_GRID(action_grid), button_remove, 2, 0, 1, 1);

	gtk_grid_attach(GTK_GRID(contacts_window_grid), action_grid, 0, 2, 1, 1);

	details_view();

	g_signal_connect(contacts_window, "delete_event", G_CALLBACK(contacts_window_delete_event_cb), NULL);

	gtk_window_set_position(GTK_WINDOW(contacts_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(contacts_window), _("Contacts"));
	gtk_widget_set_size_request(contacts_window, 800, 500);
	gtk_widget_show_all(contacts_window);
}
