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

#include <gtk/gtk.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/filter.h>

#include <roger/main.h>
#include <roger/journal.h>
#include <roger/pref.h>
#include <roger/pref_filters.h>

static gint table_y;
static GSList *pref_filters_current_rules;

/**
 * \brief Update option combobox according filter type
 * \param widget combobox widget
 * \param next option combobox pointer
 */
static void type_box_changed_cb(GtkWidget *widget, gpointer next)
{
	GtkWidget *combo_box = GTK_WIDGET(next);
	GtkListStore *store;
	gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	GtkWidget *entry = g_object_get_data(G_OBJECT(next), "entry");

	store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box)));
	gtk_list_store_clear(store);

	if (entry != NULL) {
		gtk_widget_set_sensitive(entry, TRUE);
	}

	switch (active) {
	case 0:
		/* Type */
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("All"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Incoming"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Missed"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Outgoing"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Voice box"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Fax box"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
		if (entry != NULL) {
			gtk_widget_set_sensitive(entry, FALSE);
		}
		break;
	case 1:
		/* Date */
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is not"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is after"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is before"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
		break;
	case 2:
		/* Name */
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is not"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Starts with"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Contains"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
		break;
	case 3:
		/* Number */
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is not"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Starts with"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Contains"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
		break;
	case 4:
		/* Local number */
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is not"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Starts with"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Contains"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
		break;
	}
}

/**
 * \brief Type box changed (2)
 * \param widget box widget
 * \param rule_ptr pointer to filter rule
 */
static void type_box_changed_cb2(GtkWidget *widget, gpointer rule_ptr)
{
	struct filter_rule *rule = rule_ptr;

	rule->type = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
}

/**
 * \brief Sub type box changed
 * \param widget box widget
 * \param rule_ptr pointer to filter rule
 */
static void sub_type_box_changed_cb(GtkWidget *widget, gpointer rule_ptr)
{
	struct filter_rule *rule = rule_ptr;

	rule->sub_type = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
}

/**
 * \brief Entry changed
 * \param widget entry widget
 * \param data filter rule pointer
 */
static void entry_changed_cb(GtkWidget *widget, gpointer data)
{
	struct filter_rule *rule = data;

	if (rule->entry != NULL) {
		g_free(rule->entry);
		rule->entry = NULL;
	}

	rule->entry = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
}

/* Forward declaration */
static void pref_filters_add_rule(gpointer grid_ptr, struct filter_rule *rule);

/**
 * \brief Add button callback
 * \param widget add button widget
 * \param grid current grid widget
 */
static void add_button_clicked_cb(GtkWidget *widget, gpointer grid)
{
	pref_filters_add_rule(grid, NULL);
}

/**
 * \brief Remove button callback
 * \param eidget button widget
 * \param grid grid widget pointer
 */
static void remove_button_clicked_cb(GtkWidget *widget, gpointer grid_ptr)
{
	GtkWidget *grid = grid_ptr;
	struct filter_rule *rule = g_object_get_data(G_OBJECT(grid), "rule");

	if (g_slist_length(pref_filters_current_rules) <= 1) {
		return;
	}

	pref_filters_current_rules = g_slist_remove(pref_filters_current_rules, rule);
	g_slice_free(struct filter_rule, rule);

	gtk_widget_destroy(grid);
}

/**
 * \brief Add new rule to grid
 * \param grid grid widget pointer
 * \param rule filter rule pointer
 */
static void pref_filters_add_rule(gpointer grid_ptr, struct filter_rule *rule)
{
	GtkWidget *grid = grid_ptr;
	GtkWidget *own_grid;
	GtkWidget *type_box;
	GtkWidget *sub_type_box;
	GtkWidget *entry;
	GtkWidget *remove_button;
	GtkWidget *add_button;

	if (!rule) {
		rule = g_slice_new0(struct filter_rule);
	}

	own_grid = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(own_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(own_grid), 15);
	gtk_grid_set_column_homogeneous(GTK_GRID(own_grid), TRUE);

	g_object_set_data(G_OBJECT(own_grid), "rule", rule);

	pref_filters_current_rules = g_slist_append(pref_filters_current_rules, rule);

	type_box = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Call type"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Date"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Name"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Number"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Local number"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(type_box), rule->type);

	g_signal_connect(G_OBJECT(type_box), "changed", G_CALLBACK(type_box_changed_cb2), rule);
	gtk_grid_attach(GTK_GRID(own_grid), type_box, 0, 0, 1, 1);

	sub_type_box = gtk_combo_box_text_new();
	type_box_changed_cb(type_box, sub_type_box);
	gtk_combo_box_set_active(GTK_COMBO_BOX(sub_type_box), rule->sub_type);

	g_signal_connect(G_OBJECT(sub_type_box), "changed", G_CALLBACK(sub_type_box_changed_cb), rule);
	gtk_grid_attach(GTK_GRID(own_grid), sub_type_box, 1, 0, 1, 1);

	entry = gtk_entry_new();
	if (rule->entry) {
		gtk_entry_set_text(GTK_ENTRY(entry), rule->entry);
	}
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), rule);
	gtk_grid_attach(GTK_GRID(own_grid), entry, 2, 0, 1, 1);

	add_button = gtk_button_new_with_mnemonic(_("Add"));
	g_signal_connect(G_OBJECT(add_button), "clicked", G_CALLBACK(add_button_clicked_cb), grid);
	gtk_grid_attach(GTK_GRID(own_grid), add_button, 3, 0, 1, 1);

	remove_button = gtk_button_new_with_mnemonic(_("_Remove"));
	g_signal_connect(remove_button, "clicked", G_CALLBACK(remove_button_clicked_cb), own_grid);
	gtk_grid_attach(GTK_GRID(own_grid), remove_button, 4, 0, 1, 1);

	g_signal_connect(G_OBJECT(type_box), "changed", G_CALLBACK(type_box_changed_cb), sub_type_box);

	gtk_widget_show_all(own_grid);

	gtk_grid_attach(GTK_GRID(grid), own_grid, 0, table_y, 2, 1);

	table_y++;
}

/**
 * \brief Refresh filter list - rebuild
 * \param list_store list store widget
 */
void filter_refresh_list(GtkListStore *list_store)
{
	GSList *list = filter_get_list();
	GtkTreeIter iter;

	while (list) {
		struct filter *filter = list->data;

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter, 0, filter->name, -1);
		gtk_list_store_set(list_store, &iter, 1, filter, -1);
		list = list->next;
	}
}

/**
 * \brief Filter edit callback
 * \param widget button widget
 * \param data gtk tree view widget
 */
void filter_edit_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeIter selected_iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));
	struct filter *filter;
	GtkWidget *edit_dialog;
	GtkWidget *grid;
	GtkWidget *label;
	GtkWidget *entry;
	GSList *list;
	struct filter_rule *rule;
	GtkWidget *box;
	GValue ptr = {0};
	GtkListStore *list_store;

	if (!gtk_tree_selection_get_selected(selection, &model, &selected_iter)) {
		return;
	}

	gtk_tree_model_get_value(model, &selected_iter, 1, &ptr);

	filter = g_value_get_pointer(&ptr);
	g_value_unset(&ptr);

	edit_dialog = gtk_dialog_new_with_buttons(_("Edit filter"), pref_get_window(), GTK_DIALOG_DESTROY_WITH_PARENT, _("_Close"), GTK_RESPONSE_CLOSE, NULL);

	grid = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

	label = gtk_label_new(_("Enter filter name"));

	table_y = 0;
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(label), 0, table_y, 1, 1);
	entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(entry), 1, table_y, 1, 1);
	gtk_entry_set_text(GTK_ENTRY(entry), filter->name);
	table_y++;

	pref_filters_current_rules = NULL;
	for (list = filter->rules; list != NULL; list = list->next) {
		rule = list->data;

		pref_filters_add_rule(grid, rule);
	}

	pref_filters_current_rules = filter->rules;

	gtk_widget_show_all(grid);

	box = gtk_dialog_get_content_area(GTK_DIALOG(edit_dialog));
	gtk_box_pack_start(GTK_BOX(box), grid, TRUE, TRUE, 5);
	gtk_window_set_position(GTK_WINDOW(edit_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gtk_dialog_run(GTK_DIALOG(edit_dialog));

	if (filter->name) {
		g_free(filter->name);
	}
	filter->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

	filter->rules = pref_filters_current_rules;

	gtk_widget_destroy(edit_dialog);

	model = gtk_tree_view_get_model(data);
	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	filter_refresh_list(list_store);

	journal_update_filter();
	filter_save();
}

/**
 * \brief Add new filter dialog
 * \param widget add filter button
 * \param data filter list store
 */
void filter_add_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *add_dialog;
	GtkWidget *grid;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *box;
	GtkListStore *list_store;
	GtkTreeModel *model;
	struct filter *filter;
	gint result;

	pref_filters_current_rules = NULL;
	add_dialog = gtk_dialog_new_with_buttons(_("Add new filter"), pref_get_window(), GTK_DIALOG_DESTROY_WITH_PARENT, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);

	grid = gtk_grid_new();
	label = gtk_label_new(_("Enter filter name"));
	gtk_grid_attach(GTK_GRID(grid), label, 0, table_y, 1, 1);

	entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(grid), entry, 1, table_y, 1, 1);
	table_y++;

	pref_filters_add_rule(grid, NULL);

	gtk_widget_show_all(grid);

	box = gtk_dialog_get_content_area(GTK_DIALOG(add_dialog));
	gtk_container_add(GTK_CONTAINER(box), grid);

	gtk_window_set_position(GTK_WINDOW(add_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	result = gtk_dialog_run(GTK_DIALOG(add_dialog));

	if (result != GTK_RESPONSE_OK) {
		gtk_widget_destroy(add_dialog);
		return;
	}

	const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));

	filter = filter_new(name);
	filter->rules = pref_filters_current_rules;
	filter_add(filter);

	gtk_widget_destroy(add_dialog);

	model = gtk_tree_view_get_model(data);
	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	filter_refresh_list(list_store);

	journal_update_filter();
	filter_save();
}

/**
 * \brief Remove filter dialog
 * \param widget add filter button
 * \param data filter list store
 */
void filter_remove_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeIter selected_iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));
	GtkListStore *list_store;
	struct filter *filter = NULL;
	GValue ptr = { 0 };

	if (!gtk_tree_selection_get_selected(selection, &model, &selected_iter)) {
		return;
	}

	gtk_tree_model_get_value(model, &selected_iter, 1, &ptr);

	filter = g_value_get_pointer(&ptr);
	GtkWidget *remove_dialog = gtk_message_dialog_new(pref_get_window(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Do you want to delete the filter '%s'?"), filter->name);
	gtk_window_set_title(GTK_WINDOW(remove_dialog), _("Delete filter"));
	gtk_window_set_position(GTK_WINDOW(remove_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gint result = gtk_dialog_run(GTK_DIALOG(remove_dialog));
	gtk_widget_destroy(remove_dialog);

	if (result != GTK_RESPONSE_OK) {
		g_value_unset(&ptr);
		return;
	}

	filter_remove(filter);

	g_value_unset(&ptr);

	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	filter_refresh_list(list_store);

	journal_update_filter();
	filter_save();
}

/**
 * \brief Create filter preferences page
 * \retrun filters widget
 */
GtkWidget *pref_page_filters(void)
{
	GtkWidget *group;
	GtkWidget *scroll_window;
	GtkWidget *view;
	GtkListStore *list_store;
	GtkTreeModel *tree_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *name_column;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *edit_button;

	/**
	 * Group settings:
	 * <SCROLLEDWINDOW> with <TREEVIEW>
	 * <ADD-BUTTON> <DELETE-BUTTON> <EDIT-BUTTON>
	 */
	group = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(group), 5);
	gtk_grid_set_column_spacing(GTK_GRID(group), 15);

	/* Scroll window */
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_window), GTK_SHADOW_IN);
	gtk_widget_set_vexpand(scroll_window, TRUE);
	gtk_grid_attach(GTK_GRID(group), scroll_window, 0, 1, 3, 1);

	/* View */
	view = gtk_tree_view_new();
	gtk_widget_set_hexpand(view, TRUE);
	gtk_widget_set_vexpand(view, TRUE);

	gtk_container_add(GTK_CONTAINER(scroll_window), view);

	list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);

	filter_refresh_list(list_store);

	tree_model = GTK_TREE_MODEL(list_store);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(tree_model));

	renderer = gtk_cell_renderer_text_new();
	name_column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), name_column);

	/* Buttons */
	add_button = gtk_button_new_with_mnemonic(_("_Add"));
	g_signal_connect(add_button, "clicked", G_CALLBACK(filter_add_cb), view);
	gtk_grid_attach(GTK_GRID(group), add_button, 0, 2, 1, 1);

	remove_button = gtk_button_new_with_mnemonic(_("_Remove"));
	g_signal_connect(remove_button, "clicked", G_CALLBACK(filter_remove_cb), view);
	gtk_grid_attach(GTK_GRID(group), remove_button, 1, 2, 1, 1);

	edit_button = gtk_button_new_with_mnemonic(_("_Edit"));
	g_signal_connect(edit_button, "clicked", G_CALLBACK(filter_edit_cb), view);
	gtk_grid_attach(GTK_GRID(group), edit_button, 2, 2, 1, 1);

	return pref_group_create(group, _("Manage filters"), TRUE, FALSE);
}
