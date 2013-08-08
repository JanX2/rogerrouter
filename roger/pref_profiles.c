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

#include <libcallibre/profile.h>
#include <libcallibre/plugins.h>
#include <libcallibre/filter.h>

#include <assistant.h>
#include <main.h>
#include <pref.h>
#include <pref_profiles.h>

static GtkWidget *add_button = NULL;
static GtkWidget *remove_button = NULL;

void profile_refresh_list(GtkListStore *list_store)
{
	GSList *list = profile_get_list();
	GtkTreeIter iter;

	while (list) {
		struct profile *profile = list->data;

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter, 0, profile->name, -1);
		gtk_list_store_set(list_store, &iter, 1, profile, -1);
		list = list->next;
	}
}

/**
 * \brief Add button callback
 * \param widget add button widget
 * \param user_data user data pointer
 */
static void pref_profiles_add_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	assistant();
}

/**
 * \brief Remove button callback
 * \param widget button widget
 * \param data treeview widget
 */
static void pref_profiles_remove_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeIter selected_iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));
	GtkListStore *list_store;
	struct profile *profile = NULL;
	GValue ptr = { 0 };

	if (!gtk_tree_selection_get_selected(selection, &model, &selected_iter)) {
		return;
	}

	gtk_tree_model_get_value(model, &selected_iter, 1, &ptr);

	profile = g_value_get_pointer(&ptr);
	GtkWidget *remove_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Do you want to delete the profile '%s'?"), profile->name);
	gtk_window_set_title(GTK_WINDOW(remove_dialog), _("Delete profile"));
	gtk_window_set_position(GTK_WINDOW(remove_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gint result = gtk_dialog_run(GTK_DIALOG(remove_dialog));
	gtk_widget_destroy(remove_dialog);

	if (result != GTK_RESPONSE_OK) {
		g_value_unset(&ptr);
		return;
	}

	profile_remove(profile);

	g_value_unset(&ptr);

	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	profile_refresh_list(list_store);

	//profile_save();
}


static gboolean view_cursor_changed_cb(GtkTreeView *view, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *name;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	if (!selection) {
		return FALSE;
	}

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 0, &name, -1);

		if (!strcmp(name, profile_get_active()->name)) {
			gtk_widget_set_sensitive(remove_button, FALSE);
		} else {
			gtk_widget_set_sensitive(remove_button, TRUE);
		}
	}

	return FALSE;
}

static gboolean view_button_press_event_cb(GtkTreeView *view, GdkEventButton *event, gpointer user_data)
{
	if ((event->type != GDK_BUTTON_PRESS) || (event->button != 1)) {
		return FALSE;
	}

	return view_cursor_changed_cb(view, user_data);
}

GtkWidget *pref_page_profiles(void)
{
	GtkWidget *group;
	GtkWidget *scroll_window;
	GtkWidget *view;
	GtkListStore *list_store;
	GtkTreeModel *tree_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *name_column;

	/**
	 * Group settings:
	 * <SCROLLEDWINDOW> with <TREEVIEW>
	 * <ADD-BUTTON> <DELETE-BUTTON>
	 */
	group = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(group), 5);
	gtk_grid_set_column_spacing(GTK_GRID(group), 15);

	/* Scroll window */
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_window), GTK_SHADOW_IN);
	gtk_widget_set_vexpand(scroll_window, TRUE);
	gtk_grid_attach(GTK_GRID(group), scroll_window, 0, 1, 2, 1);

	/* View */
	view = gtk_tree_view_new();
	gtk_widget_set_hexpand(view, TRUE);
	gtk_widget_set_vexpand(view, TRUE);
	g_signal_connect(view, "cursor-changed", G_CALLBACK(view_cursor_changed_cb), NULL);
	g_signal_connect(view, "button-press-event", G_CALLBACK(view_button_press_event_cb), NULL);

	gtk_container_add(GTK_CONTAINER(scroll_window), view);

	list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);

	profile_refresh_list(list_store);

	tree_model = GTK_TREE_MODEL(list_store);
 
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(tree_model));

	renderer = gtk_cell_renderer_text_new();
	name_column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), name_column);

	/* Buttons */
	add_button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(add_button, "clicked", G_CALLBACK(pref_profiles_add_button_clicked_cb), NULL);
	gtk_grid_attach(GTK_GRID(group), add_button, 0, 2, 1, 1);

	remove_button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	g_signal_connect(remove_button, "clicked", G_CALLBACK(pref_profiles_remove_button_clicked_cb), view);
	gtk_grid_attach(GTK_GRID(group), remove_button, 1, 2, 1, 1);

	return pref_group_create(group, _("Manage profiles"), TRUE, FALSE);
}
