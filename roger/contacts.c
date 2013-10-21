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
	gchar *markup;
	int index;

	GdkPixbuf *buf = image_get_scaled(contact->image, 96, 96);
	gtk_image_set_from_pixbuf(GTK_IMAGE(detail_photo_image), buf);

	markup = g_markup_printf_escaped("<span size=\"x-large\">%s</span>", contact->name);
	gtk_label_set_markup(GTK_LABEL(detail_name_label), markup);
	g_free(markup);

	for (index = detail_row; index > 0; index--) {
		gtk_grid_remove_row(GTK_GRID(detail_grid), index);
	}
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
		gtk_misc_set_alignment(GTK_MISC(number), 0, 0.5);
		dial = gtk_button_new();
		phone_image = gtk_image_new_from_icon_name("call-start-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON(dial), phone_image);
		gtk_button_set_relief(GTK_BUTTON(dial), GTK_RELIEF_NONE);
		g_signal_connect(dial, "clicked", G_CALLBACK(dial_clicked_cb), phone_number->number);
		gtk_grid_attach(GTK_GRID(detail_grid), type, 0, detail_row, 1, 1);
		gtk_grid_attach(GTK_GRID(detail_grid), number, 1, detail_row, 1, 1);
		gtk_grid_attach(GTK_GRID(detail_grid), dial, 2, detail_row, 1, 1);
		detail_row++;
	}

	if (contact->street || contact->zip || contact->city) {
		GtkWidget *address = ui_label_new(_("Address"));
		GString *addr_str = g_string_new("");
		GtkWidget *label;

		gtk_grid_attach(GTK_GRID(detail_grid), address, 0, detail_row, 1, 1);
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
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_grid_attach(GTK_GRID(detail_grid), label, 1, detail_row, 1, 1);

		g_string_free(addr_str, TRUE);
	}

	gtk_widget_show_all(detail_grid);
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

#if GTK_CHECK_VERSION(3,10,0)
	GtkListBox *listbox = user_data;
	gtk_list_box_invalidate_filter(listbox);
#else
	GtkTreeView *view = user_data;
	GtkListStore *list_store = GTK_LIST_STORE(gtk_tree_view_get_model(view));

	gtk_list_store_clear(list_store);
	contacts_fill_list(list_store, text);
#endif
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

#if GTK_CHECK_VERSION(3,10,0)
void listbox_row_selected_cb(GtkListBox *listbox, GtkListBoxRow *row, gpointer user_data)
{
	struct contact *contact;
	gint index;

	if (!row) {
		return;
	}

	g_assert(contact_list != NULL);
	index = gtk_list_box_row_get_index(row);
	contact = g_slist_nth_data(contact_list, index);
	g_assert(contact != NULL);

	contacts_update_details(contact);
}

gboolean listbox_filter_func(GtkListBoxRow *row, gpointer user_data)
{
	GtkEntry *entry = user_data;
	const gchar *text = gtk_entry_get_text(entry);
	struct contact *contact;
	gint index;

	g_return_if_fail(row != NULL);
	g_assert(contact_list != NULL);
	index = gtk_list_box_row_get_index(row);
	contact = g_slist_nth_data(contact_list, index);
	g_assert(contact != NULL);

	return (g_strcasestr(contact->name, text) || g_strcasestr(contact->company, text)) ? TRUE : FALSE;
}

GtkWidget *contacts_list_view(GtkWidget *entry)
{
	GSList *list;
	GtkWidget *listbox = gtk_list_box_new();

	gtk_list_box_set_filter_func(GTK_LIST_BOX(listbox), listbox_filter_func, entry, NULL);

	g_signal_connect(entry, "changed", G_CALLBACK(search_entry_changed), listbox);

	for (list = contact_list; list != NULL; list = list->next) {
		struct contact *contact = list->data;
		GtkWidget *label;

		if (!EMPTY_STRING(contact->name)) {
			GtkWidget *box = gtk_grid_new();
			GtkWidget *photo_image = gtk_image_new();

			gtk_grid_set_column_spacing(GTK_GRID(box), 5);

			GdkPixbuf *buf = image_get_scaled(contact->image, -1, -1);
			gtk_image_set_from_pixbuf(GTK_IMAGE(photo_image), buf);

			gtk_grid_attach(GTK_GRID(box), photo_image, 0, 0, 1, 1);
			label = gtk_label_new(contact->name);
			gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);

			gtk_grid_attach(GTK_GRID(box), label, 1, 0, 1, 1);
			gtk_list_box_insert(GTK_LIST_BOX(listbox), box, -1);
		}
	}
	g_signal_connect(listbox, "row-selected", G_CALLBACK(listbox_row_selected_cb), NULL);

	return listbox;
}
#else


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
	GtkWidget *view = NULL;
	GtkListStore *list_store;
	GtkTreeModel *tree_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *photo_column;
	GtkTreeViewColumn *name_column;

	view = gtk_tree_view_new();

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
	name_column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), name_column);

	g_signal_connect(view, "button-press-event", G_CALLBACK(contacts_list_button_press_event_cb), list_store);
	g_signal_connect(view, "move-cursor", (GCallback) contacts_list_move_cursor_cb, list_store);
	contacts_fill_list(list_store, NULL);

	return view;
}
#endif

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

void contacts(void)
{
	GtkWidget *grid;
	GtkWidget *entry;
	GtkWidget *scrolled;
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

	contact_list = address_book_get_contacts();

	contacts_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER (contacts_window), grid);

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

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
	gtk_grid_attach(GTK_GRID(grid), entry, 0, 0, 1, 1);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_vexpand(scrolled, TRUE);

	contacts_view = contacts_list_view(entry);

	gtk_container_add(GTK_CONTAINER(scrolled), contacts_view);
	gtk_widget_set_size_request(scrolled, 300, -1);
	gtk_grid_attach(GTK_GRID(grid), scrolled, 0, 1, 1, 1);

	detail_grid = details_view();
	gtk_grid_attach(GTK_GRID(grid), detail_grid, 1, 0, 1, 2);


	g_signal_connect(contacts_window, "delete_event", G_CALLBACK(contacts_window_delete_event_cb), NULL);

	gtk_window_set_position(GTK_WINDOW(contacts_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(contacts_window), _("Contacts"));
	gtk_widget_set_size_request(contacts_window, 800, 500);
	gtk_widget_show_all(contacts_window);
}
