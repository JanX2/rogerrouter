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
#include <libroutermanager/profile.h>
#include <libroutermanager/phone.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/action.h>
#include <libroutermanager/router.h>

#include <roger/main.h>
#include <roger/pref.h>
#include <roger/pref_action.h>
#include <roger/uitools.h>

static GtkWidget *incoming_call_rings_toggle;
static GtkWidget *incoming_call_begins_toggle;
static GtkWidget *incoming_call_ends_toggle;
static GtkWidget *incoming_call_missed_toggle;
static GtkWidget *outgoing_call_label;
static GtkWidget *outgoing_call_dial_toggle;
static GtkWidget *outgoing_call_begins_toggle;
static GtkWidget *outgoing_call_ends_toggle;

static struct action *selected_action = NULL;
static gchar **selected_numbers = NULL;

static void set_toggle_buttons(gboolean state)
{
	gtk_widget_set_sensitive(incoming_call_rings_toggle, state);
	gtk_widget_set_sensitive(incoming_call_begins_toggle, state);
	gtk_widget_set_sensitive(incoming_call_ends_toggle, state);
	gtk_widget_set_sensitive(incoming_call_missed_toggle, state);
	gtk_widget_set_sensitive(outgoing_call_dial_toggle, state);
	gtk_widget_set_sensitive(outgoing_call_begins_toggle, state);
	gtk_widget_set_sensitive(outgoing_call_ends_toggle, state);
}

void action_refresh_list(GtkListStore *list_store)
{
	GSList *list = action_get_list(profile_get_active());
	GtkTreeIter iter;

	while (list) {
		struct action *action = list->data;

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter, 0, action->name, -1);
		gtk_list_store_set(list_store, &iter, 1, action, -1);
		list = list->next;
	}

	if (!g_slist_length(list)) {
		set_toggle_buttons(FALSE);
	}
}

void settings_refresh_list(GtkListStore *list_store)
{
	gchar **numbers = router_get_numbers(profile_get_active());
	GtkTreeIter iter;
	gint count;
	gint index;

	selected_numbers = selected_action ? g_settings_get_strv(selected_action->settings, "numbers") : NULL;

	for (index = 0; index < g_strv_length(numbers); index++) {
		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter, 0, FALSE, -1);
		gtk_list_store_set(list_store, &iter, 1, numbers[index], -1);

		if (selected_numbers) {
			for (count = 0; count < g_strv_length(selected_numbers); count++) {
				if (!strcmp(numbers[index], selected_numbers[count])) {
					gtk_list_store_set(list_store, &iter, 0, TRUE, -1);
				}
			}
		}
	}
}

static void action_enable_toggle_cb(GtkCellRendererToggle *toggle, gchar *path_str, gpointer user_data)
{
	GtkTreeModel *model = user_data;
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
	GtkTreeIter iter;
	GValue iter_value = {0};
	GValue name_value = {0};
	gboolean dial;
	gint count = 0;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, 0, &dial, -1);

	dial ^= 1;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, dial, -1);

	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get_value(model, &iter, 0, &iter_value);
			gtk_tree_model_get_value(model, &iter, 1, &name_value);

			if (g_value_get_boolean(&iter_value)) {
				selected_numbers = g_realloc(selected_numbers, (count + 1) * sizeof(char *));
				selected_numbers[count] = g_strdup(g_value_get_string(&name_value));
				count++;
			}

			g_value_unset(&iter_value);
			g_value_unset(&name_value);
		} while (gtk_tree_model_iter_next(model, &iter));
	} else {
		g_debug("FAILED");
	}

	/* Terminate array */
	selected_numbers = g_realloc(selected_numbers, (count + 1) * sizeof(char *));
	selected_numbers[count] = NULL;

	gtk_tree_path_free(path);
}

gboolean action_edit(struct action *action)
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *grid;
	GtkWidget *general_grid;
	GtkWidget *settings_grid;
	GtkWidget *name_label;
	GtkWidget *name_entry;
	GtkWidget *description_label;
	GtkWidget *description_entry;
	GtkWidget *exec_label;
	GtkWidget *exec_entry;
	GtkWidget *scroll_window;
	GtkWidget *view;
	GtkListStore *list_store;
	GtkTreeModel *tree_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *enable_column;
	GtkTreeViewColumn *number_column;
	gint result;
	gboolean changed = FALSE;

	dialog = gtk_dialog_new_with_buttons(_("Action"), pref_get_window(), GTK_DIALOG_MODAL, _("_Apply"), GTK_RESPONSE_APPLY, _("_Cancel"), GTK_RESPONSE_CANCEL, NULL);

	/**
	 * General:
	 *   Name: <ENTRY>
	 *   Description: <ENTRY>
	 *   Exec: <ENTRY>
	 *
	 * MSN Settings:
	 *   |-Enabled----Number-------|
	 *   |    +       NUMBER       |
	 *   |-------------------------|
	 */
	grid = gtk_grid_new();
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_add(GTK_CONTAINER(content_area), grid);

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	/* General grid */
	general_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(general_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(general_grid), 5);

	/* Name */
	name_label = ui_label_new(_("Name"));
	gtk_grid_attach(GTK_GRID(general_grid), name_label, 0, 0, 1, 1);

	name_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(general_grid), name_entry, 1, 0, 1, 1);

	/* Description */
	description_label = ui_label_new(_("Description"));
	gtk_grid_attach(GTK_GRID(general_grid), description_label, 0, 1, 1, 1);

	description_entry = gtk_entry_new();
	gtk_widget_set_hexpand(description_entry, TRUE);
	gtk_grid_attach(GTK_GRID(general_grid), description_entry, 1, 1, 1, 1);

	/* Exec */
	exec_label = ui_label_new(_("Execute"));
	gtk_grid_attach(GTK_GRID(general_grid), exec_label, 0, 2, 1, 1);

	exec_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(exec_entry, _("Regex:\n%LINE% - Local line\n%NUMBER% - Caller number\n%NAME% - Caller name\n%COMPANY% - Caller company"));
	gtk_grid_attach(GTK_GRID(general_grid), exec_entry, 1, 2, 1, 1);

	if (action) {
		gtk_entry_set_text(GTK_ENTRY(name_entry), action->name);
		gtk_entry_set_text(GTK_ENTRY(description_entry), action->description);
		gtk_entry_set_text(GTK_ENTRY(exec_entry), action->exec);
	}

	gtk_grid_attach(GTK_GRID(grid), pref_group_create(general_grid, _("General"), TRUE, FALSE), 0, 0, 1, 1);

	/* Settings grid */
	settings_grid = gtk_grid_new();
	gtk_widget_set_hexpand(settings_grid, TRUE);
	gtk_widget_set_vexpand(settings_grid, TRUE);

	/* Scrolled window */
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_window), GTK_SHADOW_IN);
	gtk_widget_set_hexpand(scroll_window, TRUE);
	gtk_widget_set_vexpand(scroll_window, TRUE);

	/* Treeview */
	view = gtk_tree_view_new();

	gtk_widget_set_hexpand(view, TRUE);
	gtk_widget_set_vexpand(view, TRUE);
	gtk_container_add(GTK_CONTAINER(scroll_window), view);
	gtk_grid_attach(GTK_GRID(settings_grid), scroll_window, 0, 0, 1, 1);

	list_store = gtk_list_store_new(2, G_TYPE_BOOLEAN, G_TYPE_STRING);

	settings_refresh_list(list_store);

	tree_model = GTK_TREE_MODEL(list_store);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(tree_model));

	renderer = gtk_cell_renderer_toggle_new();
	enable_column = gtk_tree_view_column_new_with_attributes(_("Enable"), renderer, "active", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), enable_column);
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(action_enable_toggle_cb), tree_model);

	renderer = gtk_cell_renderer_text_new();
	number_column = gtk_tree_view_column_new_with_attributes(_("Number"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), number_column);

	gtk_grid_attach(GTK_GRID(grid), pref_group_create(settings_grid, _("MSN Settings"), TRUE, TRUE), 1, 0, 1, 1);

	gtk_widget_show_all(grid);
	result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result == GTK_RESPONSE_APPLY) {
		if (!action) {
			action = action_create();
			action_add(profile_get_active(), action);
			selected_action = action;
		}

		action = action_modify(action, gtk_entry_get_text(GTK_ENTRY(name_entry)), gtk_entry_get_text(GTK_ENTRY(description_entry)), gtk_entry_get_text(GTK_ENTRY(exec_entry)), selected_numbers);
		action_commit(profile_get_active());

		changed = TRUE;
	}

	gtk_widget_destroy(dialog);

	return changed;
}

static void pref_action_add_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	if (action_edit(NULL)) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(data));
		GtkListStore *list_store;

		list_store = GTK_LIST_STORE(model);
		gtk_list_store_clear(list_store);
		action_refresh_list(list_store);
	}
}

static void pref_action_remove_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeIter selected_iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));
	GtkListStore *list_store;
	struct action *action = NULL;
	GValue ptr = { 0 };

	if (!gtk_tree_selection_get_selected(selection, &model, &selected_iter)) {
		return;
	}

	gtk_tree_model_get_value(model, &selected_iter, 1, &ptr);

	action = g_value_get_pointer(&ptr);
	GtkWidget *remove_dialog = gtk_message_dialog_new(pref_get_window(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Do you want to delete the action '%s'?"), action->name);
	gtk_window_set_title(GTK_WINDOW(remove_dialog), _("Delete action"));
	gtk_window_set_position(GTK_WINDOW(remove_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gint result = gtk_dialog_run(GTK_DIALOG(remove_dialog));
	gtk_widget_destroy(remove_dialog);

	if (result != GTK_RESPONSE_OK) {
		g_value_unset(&ptr);
		return;
	}

	selected_action = NULL;

	action_remove(profile_get_active(), action);
	action_commit(profile_get_active());
	action_free(action);

	g_value_unset(&ptr);

	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	action_refresh_list(list_store);
}

static void pref_action_edit_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeIter selected_iter;
	GtkTreeModel *model;
	GtkListStore *list_store;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));
	struct action *action = NULL;
	GValue ptr = { 0 };

	if (!gtk_tree_selection_get_selected(selection, &model, &selected_iter)) {
		return;
	}

	gtk_tree_model_get_value(model, &selected_iter, 1, &ptr);

	action = g_value_get_pointer(&ptr);

	if (action_edit(action)) {
		list_store = GTK_LIST_STORE(model);
		gtk_list_store_clear(list_store);
		action_refresh_list(list_store);
	}

	g_value_unset(&ptr);
}

static gboolean view_cursor_changed_cb(GtkTreeView *view, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	struct action *action;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	if (!selection) {
		return FALSE;
	}

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		guint flags;
		gtk_tree_model_get(model, &iter, 1, &action, -1);

		gtk_label_set_text(GTK_LABEL(user_data), action->description);

		selected_action = action;

		/* Store flags as otherwise we would mix up the settings */
		flags = selected_action->flags;

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(incoming_call_rings_toggle), flags & ACTION_INCOMING_RING);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(incoming_call_begins_toggle), flags & ACTION_INCOMING_BEGIN);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(incoming_call_ends_toggle), flags & ACTION_INCOMING_END);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(outgoing_call_begins_toggle), flags & ACTION_OUTGOING_BEGIN);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(outgoing_call_ends_toggle), flags & ACTION_OUTGOING_END);

		set_toggle_buttons(TRUE);

		selected_action = action;
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

void action_toggle_cb(GtkToggleButton *button, gpointer user_data)
{
	guint flags = 0;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(incoming_call_rings_toggle))) {
		flags |= ACTION_INCOMING_RING;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(incoming_call_begins_toggle))) {
		flags |= ACTION_INCOMING_BEGIN;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(incoming_call_ends_toggle))) {
		flags |= ACTION_INCOMING_END;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(incoming_call_missed_toggle))) {
		flags |= ACTION_INCOMING_MISSED;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(outgoing_call_dial_toggle))) {
		flags |= ACTION_OUTGOING_DIAL;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(outgoing_call_begins_toggle))) {
		flags |= ACTION_OUTGOING_BEGIN;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(outgoing_call_ends_toggle))) {
		flags |= ACTION_OUTGOING_END;
	}

	selected_action->flags = flags;
	g_settings_set_int(selected_action->settings, "flags", flags);
}

GtkWidget *pref_page_action(void)
{
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *scroll_window;
	GtkWidget *view;
	GtkListStore *list_store;
	GtkTreeModel *tree_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *name_column;
	GtkWidget *execute_label;
	GtkWidget *incoming_call_label;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *edit_button;
	GtkWidget *description_label;
	GtkWidget *description_text_label;
	gchar *tmp;

	/* Define actions:
	 * |-Name-------------| Execute actions
	 * | J.A.N.           |  Incoming call:
	 * |                  |  [] Call rings
	 * |                  |  [] Call begins
	 * |                  |  [] Call ends
	 * |                  |  [] Call missed
	 * |                  |  Outgoing call:
	 * |                  |  [] Call dial
	 * |                  |  [] Call begins
	 * |                  |  [] Call ends
	 * --------------------
	 * Add Remove Edit
	 *
	 * Description:
	 * Just another narrator
	 */

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	/* Scrolled window */
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_window), GTK_SHADOW_IN);
	gtk_widget_set_vexpand(scroll_window, TRUE);

	/* Treeview */
	view = gtk_tree_view_new();

	gtk_widget_set_hexpand(view, TRUE);
	gtk_widget_set_vexpand(view, TRUE);
	gtk_container_add(GTK_CONTAINER(scroll_window), view);
	gtk_grid_attach(GTK_GRID(grid), scroll_window, 0, 0, 3, 10);

	list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);

	tree_model = GTK_TREE_MODEL(list_store);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(tree_model));

	renderer = gtk_cell_renderer_text_new();
	name_column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), name_column);

	/* Execute actions label */
	execute_label = gtk_label_new("");
	tmp = ui_bold_text(_("Execute actions"));
	gtk_label_set_markup(GTK_LABEL(execute_label), tmp);
	g_free(tmp);
	gtk_grid_attach(GTK_GRID(grid), execute_label, 3, 0, 1, 1);

	/* Incoming call label */
	incoming_call_label = gtk_label_new(_("Incoming call"));
	gtk_grid_attach(GTK_GRID(grid), incoming_call_label, 3, 1, 1, 1);

	/* Incoming call rings toggle */
	incoming_call_rings_toggle = gtk_check_button_new_with_label(_("Call rings"));
	g_signal_connect(incoming_call_rings_toggle, "toggled", G_CALLBACK(action_toggle_cb), NULL);
	gtk_grid_attach(GTK_GRID(grid), incoming_call_rings_toggle, 3, 2, 1, 1);

	/* Incoming call begins toggle */
	incoming_call_begins_toggle = gtk_check_button_new_with_label(_("Call begins"));
	g_signal_connect(incoming_call_begins_toggle, "toggled", G_CALLBACK(action_toggle_cb), NULL);
	gtk_grid_attach(GTK_GRID(grid), incoming_call_begins_toggle, 3, 3, 1, 1);

	/* Incoming call ends toggle */
	incoming_call_ends_toggle = gtk_check_button_new_with_label(_("Call ends"));
	g_signal_connect(incoming_call_ends_toggle, "toggled", G_CALLBACK(action_toggle_cb), NULL);
	gtk_grid_attach(GTK_GRID(grid), incoming_call_ends_toggle, 3, 4, 1, 1);

	/* Incoming call missed toggle */
	incoming_call_missed_toggle = gtk_check_button_new_with_label(_("Call missed"));
	g_signal_connect(incoming_call_missed_toggle, "toggled", G_CALLBACK(action_toggle_cb), NULL);
	gtk_grid_attach(GTK_GRID(grid), incoming_call_missed_toggle, 3, 5, 1, 1);

	/* Outgoing call label */
	outgoing_call_label = gtk_label_new(_("Outgoing call"));
	gtk_grid_attach(GTK_GRID(grid), outgoing_call_label, 3, 6, 1, 1);

	/* Outgoing call dial toggle */
	outgoing_call_dial_toggle = gtk_check_button_new_with_label(_("Call dial"));
	g_signal_connect(outgoing_call_dial_toggle, "toggled", G_CALLBACK(action_toggle_cb), NULL);
	gtk_grid_attach(GTK_GRID(grid), outgoing_call_dial_toggle, 3, 7, 1, 1);

	/* Outgoing call begins toggle */
	outgoing_call_begins_toggle = gtk_check_button_new_with_label(_("Call begins"));
	g_signal_connect(outgoing_call_begins_toggle, "toggled", G_CALLBACK(action_toggle_cb), NULL);
	gtk_grid_attach(GTK_GRID(grid), outgoing_call_begins_toggle, 3, 8, 1, 1);

	/* Outgoing call ends toggle */
	outgoing_call_ends_toggle = gtk_check_button_new_with_label(_("Call ends"));
	g_signal_connect(outgoing_call_ends_toggle, "toggled", G_CALLBACK(action_toggle_cb), NULL);
	gtk_grid_attach(GTK_GRID(grid), outgoing_call_ends_toggle, 3, 9, 1, 1);

	/* Buttons */
	add_button = gtk_button_new_with_mnemonic(_("_Add"));
	g_signal_connect(add_button, "clicked", G_CALLBACK(pref_action_add_button_clicked_cb), view);
	gtk_grid_attach(GTK_GRID(grid), add_button, 0, 11, 1, 1);

	remove_button = gtk_button_new_with_mnemonic(_("_Remove"));
	g_signal_connect(remove_button, "clicked", G_CALLBACK(pref_action_remove_button_clicked_cb), view);
	gtk_grid_attach(GTK_GRID(grid), remove_button, 1, 11, 1, 1);

	edit_button = gtk_button_new_with_mnemonic(_("_Edit"));
	g_signal_connect(edit_button, "clicked", G_CALLBACK(pref_action_edit_button_clicked_cb), view);
	gtk_grid_attach(GTK_GRID(grid), edit_button, 2, 11, 1, 1);

	set_toggle_buttons(FALSE);

	/* Description */
	description_label = ui_label_new("");
	tmp = ui_bold_text(_("Description"));
	gtk_label_set_markup(GTK_LABEL(description_label), tmp);
	g_free(tmp);
	gtk_widget_set_halign(description_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), description_label, 0, 12, 3, 1);

	description_text_label = ui_label_new("");
	gtk_widget_set_halign(description_text_label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), description_text_label, 0, 13, 3, 1);

	g_signal_connect(view, "cursor-changed", G_CALLBACK(view_cursor_changed_cb), description_text_label);
	g_signal_connect(view, "button-press-event", G_CALLBACK(view_button_press_event_cb), description_text_label);

	action_refresh_list(list_store);

	return pref_group_create(grid, _("Define actions"), TRUE, TRUE);
}
