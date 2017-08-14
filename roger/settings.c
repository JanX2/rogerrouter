/*
 * Roger Router
 * Copyright (c) 2012-2017 Jan-Michael Brummer
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

#include <rm/rm.h>

#include <roger/application.h>
#include <roger/settings.h>
#include <roger/journal.h>
#include <roger/main.h>
#include <roger/uitools.h>

struct settings {
	GtkWidget *window;
	GtkWidget *headerbar;
	GtkWidget *host_entry;
	GtkWidget *login_user_entry;
	GtkWidget *login_password_entry;
	GtkWidget *ftp_user_entry;
	GtkWidget *ftp_password_entry;
	GtkWidget *international_access_code_entry;
	GtkWidget *national_access_code_entry;
	GtkWidget *line_access_code_entry;
	GtkWidget *area_code_entry;
	GtkWidget *country_code_entry;

	GtkWidget *phone_plugin_combobox;
	GtkWidget *fax_plugin_combobox;
	GtkWidget *numbers_assign_listbox;

	GtkWidget *phone_active_switch;
	GtkWidget *softphone_msn_combobox;
	GtkWidget *softphone_controller_combobox;
	GtkWidget *fax_active_switch;
	GtkWidget *fax_header_entry;
	GtkWidget *fax_ident_entry;
	GtkWidget *fax_msn_combobox;
	GtkWidget *fax_controller_combobox;
	GtkWidget *fax_service_combobox;
	GtkWidget *fax_resolution_combobox;
	GtkWidget *fax_report_switch;
	GtkWidget *fax_report_directory_chooser;
	GtkWidget *fax_ecm_switch;
	GtkWidget *fax_ghostscript_label;
	GtkWidget *fax_ghostscript_file_chooser_button;

	GtkListStore *filter_liststore;
	GtkListStore *actions_liststore;
	GtkWidget *incoming_call_rings_checkbutton;
	GtkWidget *incoming_call_begins_checkbutton;
	GtkWidget *incoming_call_ends_checkbutton;
	GtkWidget *incoming_call_missed_checkbutton;
	GtkWidget *outgoing_call_dial_checkbutton;
	GtkWidget *outgoing_call_begins_checkbutton;
	GtkWidget *outgoing_call_ends_checkbutton;
	GtkWidget *audio_plugin_combobox;
	GtkWidget *audio_output_combobox;
	GtkWidget *audio_output_ringtone_combobox;
	GtkWidget *audio_input_combobox;
	GtkWidget *security_plugin_combobox;
	GtkWidget *address_book_plugin_combobox;
	GtkWidget *notification_plugin_combobox;
	GtkWidget *notification_ringtone_switch;
	//GtkListStore *notification_liststore;
	GtkWidget *notification_listbox;

	GtkWidget *phone_settings;
	GtkWidget *phone_notification_plugin_combobox;
};

static struct settings *settings = NULL;


/**
 * \brief Verify that router password is valid
 * \param button verify button widget
 * \param user_data user data pointer (NULL)
 */
void login_check_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	RmProfile *profile = rm_profile_get_active();
	gboolean locked = rm_router_is_locked();
	gboolean log_in = FALSE;

	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(settings->login_password_entry), GTK_ENTRY_ICON_SECONDARY, NULL);
	rm_router_logout(profile);

	rm_router_release_lock();
	log_in = rm_router_login(profile);
	if (log_in == TRUE) {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(settings->login_password_entry), GTK_ENTRY_ICON_SECONDARY, "emblem-ok-symbolic");

		if (locked) {
			rm_netmonitor_reconnect();
		}
	}
}

/**
 * \brief Verify that ftp password is valid
 * \param button verify button widget
 * \param user_data preference window widget
 */
void ftp_login_check_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = NULL;
	RmFtp *client;
	RmProfile *profile = rm_profile_get_active();

	client = rm_ftp_init(rm_router_get_host(profile));
	if (!client) {
		dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not connect to FTP. Missing USB storage?"));
	} else {
		if (rm_ftp_login(client, rm_router_get_ftp_user(profile), rm_router_get_ftp_password(profile)) == TRUE) {
			dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, _("FTP password is valid"));
		} else {
			rm_object_emit_message(_("FTP password is invalid"), _("Please check your FTP credentials"));
		}
		rm_ftp_shutdown(client);
	}

	if (dialog) {
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

/**
 * \brief Update preferences button clicked callback
 * \param button update button widget
 * \param user_data preference window widget
 */
void reload_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *update_dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Do you want to reload the router settings and replace your preferences?"));
	gtk_window_set_title(GTK_WINDOW(update_dialog), _("Update settings"));
	gtk_window_set_position(GTK_WINDOW(update_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gint result = gtk_dialog_run(GTK_DIALOG(update_dialog));
	gtk_widget_destroy(update_dialog);

	if (result != GTK_RESPONSE_OK) {
		return;
	}

	rm_router_get_settings(rm_profile_get_active());
}

/**
 * \brief Router login password changed callback - set password in password manager
 * \param entry password entry widget
 * \param user_data UNUSED
 */
void login_password_entry_changed_cb(GtkEntry *entry, gpointer user_data)
{
	RmProfile *profile = rm_profile_get_active();

	rm_password_set(profile, "login-password", gtk_entry_get_text(GTK_ENTRY(entry)));
}

/**
 * \brief FTP login password changed callback - set password in password manager
 * \param entry password entry widget
 * \param user_data UNUSED
 */
void ftp_password_entry_changed_cb(GtkEntry *entry, gpointer user_data)
{
	RmProfile *profile = rm_profile_get_active();

	rm_password_set(profile, "ftp-password", gtk_entry_get_text(GTK_ENTRY(entry)));
}

/**
 * \brief Report dir set callback - store it in the settings
 * \param chooser file chooser widget
 * \param user_data user data pointer (NULL)
 */
void fax_report_directory_chooser_file_set_cb(GtkFileChooserButton *chooser, gpointer user_data)
{
	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));

	g_settings_set_string(rm_profile_get_active()->settings, "fax-report-dir", filename);

	g_free(filename);
}

void fax_report_switch_activate_cb(GtkSwitch *report_switch, gboolean state, gpointer user_data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(user_data), gtk_switch_get_active(report_switch));
}

/**
 * \brief Refresh filter list - rebuild
 * \param list_store list store widget
 */
void filter_refresh_list(GtkListStore *list_store)
{
	GSList *list;

	for (list = rm_filter_get_list(rm_profile_get_active()); list != NULL; list = list->next) {
		GtkTreeIter iter;
		RmFilter *filter = list->data;

		gtk_list_store_insert_with_values(list_store, &iter, -1, 0, filter->name, 1, filter, -1);
	}
}

static gint table_y;
static GSList *pref_filters_current_rules;

GtkWidget *pref_group_create(GtkWidget *box, gchar *title_str, gboolean hexpand, gboolean vexpand)
{
	GtkWidget *grid;
	GtkWidget *title;
	gchar *title_markup = ui_bold_text(title_str);

	grid = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);

	/* Configure plugins label */
	title = gtk_label_new("");
	gtk_widget_set_halign(title, GTK_ALIGN_START);
	gtk_widget_set_margin(title, 10, 5, 10, 5);
	gtk_label_set_markup(GTK_LABEL(title), title_markup);
	gtk_grid_attach(GTK_GRID(grid), title, 0, 0, 1, 1);

	gtk_widget_set_margin(box, 20, 0, 20, 10);
	gtk_grid_attach(GTK_GRID(grid), box, 0, 1, 1, 1);

	g_free(title_markup);

	return grid;
}

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
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Fax Report"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Record"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Blocked"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);

		if (entry != NULL) {
			gtk_widget_set_sensitive(entry, FALSE);
		}
		break;
	case 1:
		/* Date/Time */
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is not"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is after"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), _("Is before"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
		break;
	case 2:
	/* Name */
	case 3:
	/* Number */
	case 4:
	/* Extension */
	case 5:
		/* Line */
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
	RmFilterRule *rule = rule_ptr;

	rule->type = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
}

/**
 * \brief Sub type box changed
 * \param widget box widget
 * \param rule_ptr pointer to filter rule
 */
static void sub_type_box_changed_cb(GtkWidget *widget, gpointer rule_ptr)
{
	RmFilterRule *rule = rule_ptr;

	rule->sub_type = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
}

/**
 * \brief Entry changed
 * \param widget entry widget
 * \param data filter rule pointer
 */
static void entry_changed_cb(GtkWidget *widget, gpointer data)
{
	RmFilterRule *rule = data;

	if (rule->entry != NULL) {
		g_free(rule->entry);
		rule->entry = NULL;
	}

	rule->entry = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
}

/* Forward declaration */
static void pref_filters_add_rule(gpointer grid_ptr, RmFilterRule *rule);

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
	RmFilterRule *rule = g_object_get_data(G_OBJECT(grid), "rule");

	if (g_slist_length(pref_filters_current_rules) <= 1) {
		return;
	}

	pref_filters_current_rules = g_slist_remove(pref_filters_current_rules, rule);
	g_slice_free(RmFilterRule, rule);

	gtk_widget_destroy(grid);
}

/**
 * \brief Add new rule to grid
 * \param grid grid widget pointer
 * \param rule filter rule pointer
 */
static void pref_filters_add_rule(gpointer grid_ptr, RmFilterRule *rule)
{
	GtkWidget *grid = grid_ptr;
	GtkWidget *own_grid;
	GtkWidget *type_box;
	GtkWidget *sub_type_box;
	GtkWidget *entry;
	GtkWidget *remove_button;
	GtkWidget *add_button;
	GtkWidget *image;

	if (!rule) {
		rule = g_slice_new0(RmFilterRule);
	}

	own_grid = gtk_grid_new();

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(own_grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(own_grid), 12);
	//gtk_grid_set_column_homogeneous(GTK_GRID(own_grid), TRUE);

	g_object_set_data(G_OBJECT(own_grid), "rule", rule);

	pref_filters_current_rules = g_slist_append(pref_filters_current_rules, rule);

	type_box = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Type"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Date/Time"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Name"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Number"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Extension"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_box), _("Line"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(type_box), rule->type);

	g_signal_connect(G_OBJECT(type_box), "changed", G_CALLBACK(type_box_changed_cb2), rule);
	gtk_grid_attach(GTK_GRID(own_grid), type_box, 0, 0, 1, 1);

	sub_type_box = gtk_combo_box_text_new();
	type_box_changed_cb(type_box, sub_type_box);
	gtk_combo_box_set_active(GTK_COMBO_BOX(sub_type_box), rule->sub_type);

	g_signal_connect(G_OBJECT(sub_type_box), "changed", G_CALLBACK(sub_type_box_changed_cb), rule);
	gtk_grid_attach(GTK_GRID(own_grid), sub_type_box, 1, 0, 1, 1);

	entry = gtk_entry_new();
	gtk_widget_set_hexpand(entry, TRUE);
	if (rule->entry) {
		gtk_entry_set_text(GTK_ENTRY(entry), rule->entry);
	}
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), rule);
	gtk_grid_attach(GTK_GRID(own_grid), entry, 2, 0, 1, 1);

	add_button = gtk_button_new();
	image = gtk_image_new_from_icon_name("list-add-symbolic", GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(add_button), image);

	g_signal_connect(G_OBJECT(add_button), "clicked", G_CALLBACK(add_button_clicked_cb), grid);
	gtk_grid_attach(GTK_GRID(own_grid), add_button, 3, 0, 1, 1);

	remove_button = gtk_button_new();
	image = gtk_image_new_from_icon_name("list-remove-symbolic", GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(remove_button), image);
	g_signal_connect(remove_button, "clicked", G_CALLBACK(remove_button_clicked_cb), own_grid);
	gtk_grid_attach(GTK_GRID(own_grid), remove_button, 4, 0, 1, 1);

	g_signal_connect(G_OBJECT(type_box), "changed", G_CALLBACK(type_box_changed_cb), sub_type_box);

	gtk_widget_show_all(own_grid);

	gtk_grid_attach(GTK_GRID(grid), own_grid, 0, table_y, 2, 1);

	table_y++;
}

/**
 * \brief Filter edit callback
 * \param widget button widget
 * \param data gtk tree view widget
 */
void filter_edit_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeIter selected_iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));
	RmFilter *filter;
	GtkWidget *dialog;
	GtkWidget *grid;
	GtkWidget *content;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *save;
	GSList *list;
	RmFilterRule *rule;
	GValue ptr = { 0 };
	GtkListStore *list_store;
	gint result;
	gboolean use_header = TRUE;

	if (!gtk_tree_selection_get_selected(selection, &model, &selected_iter)) {
		return;
	}

	gtk_tree_model_get_value(model, &selected_iter, 1, &ptr);

	filter = g_value_get_pointer(&ptr);
	g_value_unset(&ptr);

	//dialog = gtk_dialog_new_with_buttons(_("Edit filter"), settings->window, GTK_DIALOG_DESTROY_WITH_PARENT, _("_Close"), GTK_RESPONSE_CLOSE, NULL);

	dialog = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", use_header, NULL);

	gtk_window_set_title(GTK_WINDOW(dialog), _("Edit filter"));
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(settings->window));
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	save = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Save"), GTK_RESPONSE_OK);
	ui_set_suggested_style(save);

	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(content), grid);
	gtk_widget_set_margin(grid, 18, 18, 18, 18);

	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	label = ui_label_new(_("Name:"));
	gtk_widget_set_margin(label, 0, 0, 0, 6);
	gtk_grid_attach(GTK_GRID(grid), label, 0, table_y, 1, 1);

	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), filter->name);
	gtk_widget_set_margin(entry, 0, 0, 0, 6);
	gtk_widget_set_hexpand(entry, TRUE);
	gtk_grid_attach(GTK_GRID(grid), entry, 1, table_y, 1, 1);
	table_y++;

	GtkWidget *type_in_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(type_in_grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(type_in_grid), 12);

	GtkWidget *type_grid = pref_group_create(type_in_grid, _("Rules"), TRUE, FALSE);
	gtk_grid_attach(GTK_GRID(grid), type_grid, 0, table_y, 2, 1);
	table_y++;

	//pref_filters_add_rule(type_in_grid, NULL);

	pref_filters_current_rules = NULL;
	for (list = filter->rules; list != NULL; list = list->next) {
		rule = list->data;

		pref_filters_add_rule(type_in_grid, rule);
	}

	pref_filters_current_rules = filter->rules;

	gtk_widget_show_all(grid);

	result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result != GTK_RESPONSE_OK) {
		gtk_widget_destroy(dialog);
		return;
	}

	if (filter->name) {
		g_free(filter->name);
	}
	filter->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

	filter->rules = pref_filters_current_rules;

	gtk_widget_destroy(dialog);

	model = gtk_tree_view_get_model(data);
	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	filter_refresh_list(list_store);

	journal_update_filter();
}

/**
 * \brief Remove filter dialog
 * \param widget add filter button
 * \param data filter list store
 */
void filter_remove_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeIter selected_iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));
	GtkListStore *list_store;
	RmFilter *filter = NULL;
	GValue ptr = { 0 };

	if (!gtk_tree_selection_get_selected(selection, &model, &selected_iter)) {
		return;
	}

	gtk_tree_model_get_value(model, &selected_iter, 1, &ptr);

	filter = g_value_get_pointer(&ptr);
	GtkWidget *remove_dialog = gtk_message_dialog_new(GTK_WINDOW(settings->window), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Do you want to delete the filter '%s'?"), filter->name);
	gtk_window_set_title(GTK_WINDOW(remove_dialog), _("Delete filter"));
	gtk_window_set_position(GTK_WINDOW(remove_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gint result = gtk_dialog_run(GTK_DIALOG(remove_dialog));
	gtk_widget_destroy(remove_dialog);

	if (result != GTK_RESPONSE_OK) {
		g_value_unset(&ptr);
		return;
	}

	rm_filter_remove(rm_profile_get_active(), filter);

	g_value_unset(&ptr);

	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	filter_refresh_list(list_store);

	journal_update_filter();
}


/**
 * \brief Add new filter dialog
 * \param widget add filter button
 * \param data filter list store
 */
void filter_add_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	GtkWidget *grid;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *add;
	GtkWidget *content;
	GtkListStore *list_store;
	GtkTreeModel *model;
	RmFilter *filter;
	gint result;
	gboolean use_header = TRUE;

	pref_filters_current_rules = NULL;
	//dialog = gtk_dialog_new_with_buttons(_("Add new filter"), settings->window, GTK_DIALOG_DESTROY_WITH_PARENT, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);

	dialog = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", use_header, NULL);

	gtk_window_set_title(GTK_WINDOW(dialog), _("New filter"));
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(settings->window));
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	add = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Add"), GTK_RESPONSE_OK);
	ui_set_suggested_style(add);

	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(content), grid);
	gtk_widget_set_margin(grid, 18, 18, 18, 18);

	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	label = ui_label_new(_("Name:"));
	gtk_widget_set_margin(label, 0, 0, 0, 6);
	gtk_grid_attach(GTK_GRID(grid), label, 0, table_y, 1, 1);

	entry = gtk_entry_new();
	gtk_widget_set_margin(entry, 0, 0, 0, 6);
	gtk_widget_set_hexpand(entry, TRUE);
	gtk_grid_attach(GTK_GRID(grid), entry, 1, table_y, 1, 1);
	table_y++;

	GtkWidget *type_in_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(type_in_grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(type_in_grid), 12);

	GtkWidget *type_grid = pref_group_create(type_in_grid, _("Rules"), TRUE, FALSE);
	gtk_grid_attach(GTK_GRID(grid), type_grid, 0, table_y, 2, 1);
	table_y++;

	pref_filters_add_rule(type_in_grid, NULL);

	gtk_widget_show_all(grid);

	result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result != GTK_RESPONSE_OK) {
		gtk_widget_destroy(dialog);
		return;
	}

	const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));

	filter = rm_filter_new(rm_profile_get_active(), name);
	filter->rules = pref_filters_current_rules;

	gtk_widget_destroy(dialog);

	model = gtk_tree_view_get_model(data);
	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	filter_refresh_list(list_store);

	journal_update_filter();
}

static RmAction *selected_action = NULL;
static gchar **selected_numbers = NULL;


static void set_checkbutton_buttons(gboolean state)
{
	gtk_widget_set_sensitive(settings->incoming_call_rings_checkbutton, state);
	gtk_widget_set_sensitive(settings->incoming_call_begins_checkbutton, state);
	gtk_widget_set_sensitive(settings->incoming_call_ends_checkbutton, state);
	gtk_widget_set_sensitive(settings->incoming_call_missed_checkbutton, state);
	gtk_widget_set_sensitive(settings->outgoing_call_dial_checkbutton, state);
	gtk_widget_set_sensitive(settings->outgoing_call_begins_checkbutton, state);
	gtk_widget_set_sensitive(settings->outgoing_call_ends_checkbutton, state);
}

void action_refresh_list(GtkListStore *list_store)
{
	GSList *list = rm_action_get_list(rm_profile_get_active());
	GtkTreeIter iter;

	while (list) {
		RmAction *action = list->data;
		gchar *name = rm_action_get_name(action);

		gtk_list_store_insert_with_values(list_store, &iter, -1,
						  0, name,
						  1, action, -1);

		g_free(name);

		list = list->next;
	}

	if (!g_slist_length(list)) {
		set_checkbutton_buttons(FALSE);
	}
}

void settings_destroy_cb(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
	g_slice_free(struct settings, settings);
	settings = NULL;
}

void settings_refresh_list(GtkListStore *list_store)
{
	gchar **numbers = rm_router_get_numbers(rm_profile_get_active());
	GtkTreeIter iter;
	gint count;
	gint index;

	selected_numbers = selected_action ? rm_action_get_numbers(selected_action) : NULL;

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

	/*if (selected_numbers) {
	        g_strfreev(selected_numbers);
	   }*/
}

void action_enable_renderer_toggled_cb(GtkCellRendererToggle *toggle, gchar *path_str, gpointer user_data)
{
	GtkTreeModel *model = gtk_tree_view_get_model(user_data);
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
	GtkTreeIter iter;
	GValue iter_value = { 0 };
	GValue name_value = { 0 };
	gboolean dial;
	gint count = 0;

	g_debug("%s(): Called", __FUNCTION__);
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

void action_apply_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
}

gboolean action_edit(RmAction *action)
{
#if 1
	GtkWidget *dialog;
	GtkWidget *name_entry;
	GtkWidget *exec_entry;
	GtkListStore *list_store;
	GtkBuilder *builder;
	gint result;
	gboolean changed = FALSE;

	builder = gtk_builder_new_from_resource("/org/tabos/roger/settings.glade");
	if (!builder) {
		g_warning("Could not load settings ui");
		return changed;
	}

	dialog = GTK_WIDGET(gtk_builder_get_object(builder, "action_edit_dialog"));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(settings->window));

	name_entry = GTK_WIDGET(gtk_builder_get_object(builder, "action_name_entry"));
	exec_entry = GTK_WIDGET(gtk_builder_get_object(builder, "action_execute_entry"));
	list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "edit_action_liststore"));

	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	if (action) {
		gchar *name = rm_action_get_name(action);
		gchar *exec = rm_action_get_exec(action);

		gtk_entry_set_text(GTK_ENTRY(name_entry), name);
		gtk_entry_set_text(GTK_ENTRY(exec_entry), exec);

		g_free(exec);
		g_free(name);
	}
	settings_refresh_list(list_store);

	result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result == 1) {
		if (!action) {
			action = rm_action_new(rm_profile_get_active());
			rm_action_set_name(action, gtk_entry_get_text(GTK_ENTRY(name_entry)));
		}

		rm_action_set_name(action, gtk_entry_get_text(GTK_ENTRY(name_entry)));
		rm_action_set_description(action, "");
		rm_action_set_exec(action, gtk_entry_get_text(GTK_ENTRY(exec_entry)));
		rm_action_set_numbers(action, (const gchar**)selected_numbers);
		selected_action = action;

		changed = TRUE;
	}

	gtk_widget_destroy(dialog);

	return changed;
#else
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *grid;
	GtkWidget *general_grid;
	GtkWidget *settings_grid;
	GtkWidget *name_label;
	GtkWidget *name_entry;
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

	dialog = gtk_dialog_new_with_buttons(_("Action"), GTK_WINDOW(settings->window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR, _("_Apply"), GTK_RESPONSE_APPLY, _("_Cancel"), GTK_RESPONSE_CANCEL, NULL);

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

	/* Exec */
	exec_label = ui_label_new(_("Execute"));
	gtk_grid_attach(GTK_GRID(general_grid), exec_label, 0, 2, 1, 1);

	exec_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(exec_entry, _("Regex:\n%LINE% - Local line\n%NUMBER% - Caller number\n%NAME% - Caller name\n%COMPANY% - Caller company"));
	gtk_grid_attach(GTK_GRID(general_grid), exec_entry, 1, 2, 1, 1);

	if (action) {
		gtk_entry_set_text(GTK_ENTRY(name_entry), action->name);
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
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(action_enable_renderer_toggled_cb), view);

	renderer = gtk_cell_renderer_text_new();
	number_column = gtk_tree_view_column_new_with_attributes(_("Number"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), number_column);

	gtk_grid_attach(GTK_GRID(grid), pref_group_create(settings_grid, _("MSN Settings"), TRUE, TRUE), 1, 0, 1, 1);

	gtk_widget_show_all(grid);
	result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result == GTK_RESPONSE_APPLY) {
		if (!action) {
			action = action_create();
			action_add(rm_profile_get_active(), action);
			selected_action = action;
		}

		action = action_modify(action, gtk_entry_get_text(GTK_ENTRY(name_entry)), "", gtk_entry_get_text(GTK_ENTRY(exec_entry)), selected_numbers);
		action_commit(rm_profile_get_active());

		changed = TRUE;
	}

	gtk_widget_destroy(dialog);

	return changed;
#endif
}

void actions_add_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	if (action_edit(NULL)) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(data));
		GtkListStore *list_store;

		list_store = GTK_LIST_STORE(model);
		gtk_list_store_clear(list_store);
		action_refresh_list(list_store);
	}
}

void actions_remove_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeIter selected_iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));
	GtkListStore *list_store;
	RmAction *action = NULL;
	GValue ptr = { 0 };
	gchar *name;

	if (!gtk_tree_selection_get_selected(selection, &model, &selected_iter)) {
		return;
	}

	gtk_tree_model_get_value(model, &selected_iter, 1, &ptr);

	action = g_value_get_pointer(&ptr);
	name = rm_action_get_name(action);
	GtkWidget *remove_dialog = gtk_message_dialog_new(GTK_WINDOW(settings->window), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Do you want to delete the action '%s'?"), name);
	gtk_window_set_title(GTK_WINDOW(remove_dialog), _("Delete action"));
	gtk_window_set_position(GTK_WINDOW(remove_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gint result = gtk_dialog_run(GTK_DIALOG(remove_dialog));
	gtk_widget_destroy(remove_dialog);
	g_free(name);

	if (result != GTK_RESPONSE_OK) {
		g_value_unset(&ptr);
		return;
	}

	selected_action = NULL;

	g_value_unset(&ptr);

	rm_action_remove(rm_profile_get_active(), action);
	//action_commit(rm_profile_get_active());
	//action_free(action);

	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	action_refresh_list(list_store);
}

void actions_edit_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeIter selected_iter;
	GtkTreeModel *model;
	GtkListStore *list_store;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));
	RmAction *action = NULL;
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

gboolean actions_treeview_cursor_changed_cb(GtkTreeView *view, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	RmAction *action;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	if (!selection) {
		return FALSE;
	}

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		guint flags;
		gtk_tree_model_get(model, &iter, 1, &action, -1);

		selected_action = action;
		if (action) {
			/* Store flags as otherwise we would mix up the settings */
			flags = rm_action_get_flags(selected_action);

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_rings_checkbutton), flags & RM_ACTION_INCOMING_RING);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_begins_checkbutton), flags & RM_ACTION_INCOMING_BEGIN);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_ends_checkbutton), flags & RM_ACTION_INCOMING_END);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_rings_checkbutton), flags & RM_ACTION_INCOMING_RING);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_begins_checkbutton), flags & RM_ACTION_INCOMING_BEGIN);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_ends_checkbutton), flags & RM_ACTION_INCOMING_END);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_missed_checkbutton), flags & RM_ACTION_INCOMING_MISSED);

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_begins_checkbutton), flags & RM_ACTION_OUTGOING_BEGIN);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_ends_checkbutton), flags & RM_ACTION_OUTGOING_END);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_dial_checkbutton), flags & RM_ACTION_OUTGOING_DIAL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_begins_checkbutton), flags & RM_ACTION_OUTGOING_BEGIN);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_ends_checkbutton), flags & RM_ACTION_OUTGOING_END);

			set_checkbutton_buttons(TRUE);
		} else {
			set_checkbutton_buttons(FALSE);
		}
	}

	return FALSE;
}

gboolean actions_treeview_button_press_event_cb(GtkTreeView *view, GdkEventButton *event, gpointer user_data)
{
	if ((event->type != GDK_BUTTON_PRESS) || (event->button != 1)) {
		return FALSE;
	}

	return actions_treeview_cursor_changed_cb(view, user_data);
}

void action_checkbutton_toggled_cb(GtkToggleButton *button, gpointer user_data)
{
	guint flags = 0;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->incoming_call_rings_checkbutton))) {
		flags |= RM_ACTION_INCOMING_RING;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->incoming_call_begins_checkbutton))) {
		flags |= RM_ACTION_INCOMING_BEGIN;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->incoming_call_ends_checkbutton))) {
		flags |= RM_ACTION_INCOMING_END;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->incoming_call_missed_checkbutton))) {
		flags |= RM_ACTION_INCOMING_MISSED;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_dial_checkbutton))) {
		flags |= RM_ACTION_OUTGOING_DIAL;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_begins_checkbutton))) {
		flags |= RM_ACTION_OUTGOING_BEGIN;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_ends_checkbutton))) {
		flags |= RM_ACTION_OUTGOING_END;
	}

	rm_action_set_flags(selected_action, flags);
}

/**
 * \brief Update device comboboxes depending on plugin combobox
 * \param box plugin combobox
 * \param user_data UNUSED
 */
void audio_plugin_combobox_changed_cb(GtkComboBox *box, gpointer user_data)
{
	GSList *devices;
	gchar *active;
	RmAudio *audio = NULL;
	GSList *audio_plugins;
	GSList *list;
	gchar *input_name;
	gchar *output_name;
	gchar *output_ringtone_name;
	gint input_cnt = 0;
	gint output_cnt = 0;

	input_name = g_settings_get_string(rm_profile_get_active()->settings, "audio-input");
	output_name = g_settings_get_string(rm_profile_get_active()->settings, "audio-output");
	output_ringtone_name = g_settings_get_string(rm_profile_get_active()->settings, "audio-output-ringtone");

	/* Clear device comboboxes */
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(settings->audio_output_combobox));
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(settings->audio_output_ringtone_combobox));
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(settings->audio_input_combobox));

	/* Read active audio plugin */
	active = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(settings->audio_plugin_combobox));

	/* Find active audio plugin structure */
	audio_plugins = rm_audio_get_plugins();
	for (list = audio_plugins; list != NULL; list = list->next) {
		audio = list->data;

		if (!strcmp(audio->name, active)) {
			break;
		}

		audio = NULL;
	}

	/* Fill device comboboxes */
	devices = audio->get_devices();
	for (list = devices; list; list = list->next) {
		RmAudioDevice *device = list->data;

		if (device->type == RM_AUDIO_INPUT) {
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->audio_input_combobox), device->internal_name, device->name);

			if (!strcmp(device->internal_name, input_name)) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_input_combobox), input_cnt);
			}

			input_cnt++;
		} else {
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->audio_output_combobox), device->internal_name, device->name);
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->audio_output_ringtone_combobox), device->internal_name, device->name);

			if (!strcmp(device->internal_name, output_name)) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_output_combobox), output_cnt);
			}

			if (!strcmp(device->internal_name, output_ringtone_name)) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_output_ringtone_combobox), output_cnt);
			}
			output_cnt++;
		}
	}

	/* In case no device is preselected, preselect first device */
	if (devices) {
		if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(settings->audio_input_combobox))) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_input_combobox), 0);
		}

		if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(settings->audio_output_combobox))) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_output_combobox), 0);
		}

		if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(settings->audio_output_ringtone_combobox))) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_output_ringtone_combobox), 0);
		}
	}
}

void view_call_type_icons_combobox_changed_cb(GtkComboBoxText *widget, gpointer user_data)
{
	g_settings_set_string(app_settings, "icon-type", gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget)));
	journal_init_call_icon();
	journal_clear();
	journal_redraw();
	//journal_button_refresh_clicked_cb(NULL, NULL);
}

GtkWindow *settings_get_window(void)
{
	return settings ? GTK_WINDOW(settings->window) : NULL;
}

void settings_fill_address_book_plugin_combobox(struct settings *settings)
{
	GSList *plugins;
	GSList *list;

	plugins = rm_addressbook_get_plugins();
	for (list = plugins; list; list = list->next) {
		RmAddressBook *ab = list->data;

		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->address_book_plugin_combobox), ab->name, ab->name);
	}
}

void settings_notification_outgoing_toggled_cb(GtkCellRendererToggle *toggle, gchar *path_str, gpointer user_data)
{
	GtkListStore *liststore = user_data;
	GtkTreeModel *model = GTK_TREE_MODEL(liststore);
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
	GtkTreeIter iter;
	GValue iter_value = { 0 };
	GValue name_value = { 0 };
	gboolean dial;
	gint count = 0;
	gchar **selected_outgoing_numbers = NULL;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, 1, &dial, -1);

	dial ^= 1;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, dial, -1);

	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get_value(model, &iter, 1, &iter_value);
			gtk_tree_model_get_value(model, &iter, 0, &name_value);

			if (g_value_get_boolean(&iter_value)) {
				selected_outgoing_numbers = g_realloc(selected_outgoing_numbers, (count + 1) * sizeof(char *));
				selected_outgoing_numbers[count] = g_strdup(g_value_get_string(&name_value));
				count++;
			}

			g_value_unset(&iter_value);
			g_value_unset(&name_value);
		} while (gtk_tree_model_iter_next(model, &iter));
	} else {
		g_debug("FAILED");
	}

	/* Terminate array */
	selected_outgoing_numbers = g_realloc(selected_outgoing_numbers, (count + 1) * sizeof(char *));
	selected_outgoing_numbers[count] = NULL;

	gtk_tree_path_free(path);

	//g_settings_set_strv(notification_gtk_settings, "outgoing-numbers", (const gchar * const *) selected_outgoing_numbers);
	rm_profile_set_notification_outgoing_numbers(rm_profile_get_active(), (const gchar*const*)selected_outgoing_numbers);
}

void settings_notification_incoming_toggled_cb(GtkCellRendererToggle *toggle, gchar *path_str, gpointer user_data)
{
	GtkListStore *liststore = user_data;
	GtkTreeModel *model = GTK_TREE_MODEL(liststore);
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
	GtkTreeIter iter;
	GValue iter_value = { 0 };
	GValue name_value = { 0 };
	gboolean dial;
	gint count = 0;
	gchar **selected_incoming_numbers = NULL;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, 2, &dial, -1);

	dial ^= 1;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 2, dial, -1);

	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get_value(model, &iter, 2, &iter_value);
			gtk_tree_model_get_value(model, &iter, 0, &name_value);

			if (g_value_get_boolean(&iter_value)) {
				selected_incoming_numbers = g_realloc(selected_incoming_numbers, (count + 1) * sizeof(char *));
				selected_incoming_numbers[count] = g_strdup(g_value_get_string(&name_value));
				count++;
			}

			g_value_unset(&iter_value);
			g_value_unset(&name_value);
		} while (gtk_tree_model_iter_next(model, &iter));
	} else {
		g_debug("FAILED");
	}

	/* Terminate array */
	selected_incoming_numbers = g_realloc(selected_incoming_numbers, (count + 1) * sizeof(char *));
	selected_incoming_numbers[count] = NULL;

	gtk_tree_path_free(path);

	//g_settings_set_strv(notification_gtk_settings, "incoming-numbers", (const gchar * const *) selected_incoming_numbers);
	rm_profile_set_notification_incoming_numbers(rm_profile_get_active(), (const gchar*const*)selected_incoming_numbers);
}

void phone_settings_close_button_cb(GtkWidget *button, gpointer user_data)
{
	gtk_widget_destroy(settings->phone_settings);
}

void phone_plugin_settings_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	const gchar *name = gtk_combo_box_get_active_id(GTK_COMBO_BOX(settings->phone_plugin_combobox));

	if (name && !strcmp(name, _("CAPI Phone"))) {
		GtkBuilder *builder;

		builder = gtk_builder_new_from_resource("/org/tabos/roger/settings.glade");
		if (!builder) {
			g_warning("Could not load settings ui");
			return;
		}
		settings->phone_settings = GTK_WIDGET(gtk_builder_get_object(builder, "phone_settings"));

		GtkWidget *number = GTK_WIDGET(gtk_builder_get_object(builder, "phone_settings_number_combobox"));
		GtkWidget *controller = GTK_WIDGET(gtk_builder_get_object(builder, "phone_settings_controller_combobox"));
		gchar **numbers;
		gint idx;

		RmProfile *profile = rm_profile_get_active();
		numbers = rm_router_get_numbers(profile);
		for (idx = 0; idx < g_strv_length(numbers); idx++) {
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(number), numbers[idx], numbers[idx]);
		}
		g_settings_bind(profile->settings, "phone-number", number, "active-id", G_SETTINGS_BIND_DEFAULT);

		g_settings_bind(profile->settings, "phone-controller", controller, "active", G_SETTINGS_BIND_DEFAULT);
		gtk_builder_connect_signals(builder, NULL);

		g_object_unref(G_OBJECT(builder));

		gtk_window_set_transient_for(GTK_WINDOW(settings->phone_settings), GTK_WINDOW(settings->window));
		gtk_dialog_run(GTK_DIALOG(settings->phone_settings));
	}
}

void settings_numbers_device_combobox_changed_cb(GtkComboBox *box, gpointer user_data)
{
	GSList *devices = rm_device_get_plugins();
	GSList *list;
	RmDevice *new_device = rm_device_get(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(box)));
	RmDevice *old_device = NULL;
	gchar *number = user_data;
	gchar **numbers;

	for (list = devices; list != NULL; list = list->next) {
		RmDevice *device = list->data;

		if (rm_device_handles_number(device, number)) {
			old_device = device;
			break;
		}
	}

	//Remove number
	if (old_device) {
		numbers = rm_strv_remove(rm_device_get_numbers(old_device), number);

		rm_device_set_numbers(old_device, numbers);
	}

	//Add number
	if (new_device) {
		numbers = rm_strv_add(rm_device_get_numbers(new_device), number);

		rm_device_set_numbers(new_device, numbers);
	}
}

void settings_notification_combobox_changed_cb(GtkComboBox *box, gpointer user_data)
{
	RmProfile *profile = rm_profile_get_active();
	gchar *number = user_data;
	gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(box));
	gchar **incoming_numbers = rm_profile_get_notification_incoming_numbers(profile);
	gchar **outgoing_numbers = rm_profile_get_notification_outgoing_numbers(profile);

	switch (idx) {
	case 0:
		incoming_numbers = rm_strv_remove(incoming_numbers, number);
		outgoing_numbers = rm_strv_remove(outgoing_numbers, number);
		break;
	case 1:
		incoming_numbers = rm_strv_add(incoming_numbers, number);
		outgoing_numbers = rm_strv_remove(outgoing_numbers, number);
		break;
	case 2:
		incoming_numbers = rm_strv_remove(incoming_numbers, number);
		outgoing_numbers = rm_strv_add(outgoing_numbers, number);
		break;
	case 3:
		incoming_numbers = rm_strv_add(incoming_numbers, number);
		outgoing_numbers = rm_strv_add(outgoing_numbers, number);
		break;
	default:
		return;
	}

	rm_profile_set_notification_incoming_numbers(profile, (const gchar*const*)incoming_numbers);
	rm_profile_set_notification_outgoing_numbers(profile, (const gchar*const*)outgoing_numbers);
}

void settings_numbers_populate(GtkWidget *listbox)
{
	RmProfile *profile;
	gchar **numbers;
	gint idx;

	profile = rm_profile_get_active();

	numbers = rm_router_get_numbers(profile);
	for (idx = 0; idx < g_strv_length(numbers); idx++) {
		gint dev_idx = 0;
		GtkWidget *child = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
		GtkWidget *label = gtk_label_new(numbers[idx]);
		GtkWidget *sub_label = gtk_label_new(_("assigned to"));

		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_widget_set_halign(sub_label, GTK_ALIGN_START);
		gtk_widget_set_sensitive(sub_label, FALSE);

		GtkWidget *l_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_margin(l_box, 6, 6, 6, 6);
		gtk_widget_set_halign(l_box, GTK_ALIGN_START);
		gtk_widget_set_hexpand(l_box, TRUE);

		gtk_box_pack_start(GTK_BOX(l_box), label, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(l_box), sub_label, TRUE, TRUE, 0);

		gtk_box_pack_start(GTK_BOX(child), l_box, TRUE, TRUE, 0);

		//gchar *markup = g_markup_printf_escaped("<b>%s</b>\n<span style=\"italic\">assigned to</span>", numbers[idx]);
		//gtk_label_set_markup(GTK_LABEL(label), markup);
		//gtk_box_pack_start(GTK_BOX(l_box), label, TRUE, TRUE, 12);

		GtkWidget *combobox = gtk_combo_box_text_new();
		gtk_widget_set_halign(combobox, GTK_ALIGN_END);
		gtk_widget_set_margin(combobox, 6, 6, 6, 6);

		GSList *devices = rm_device_get_plugins();
		GSList *list;

		for (list = devices; list != NULL; list = list->next) {
			RmDevice *device = list->data;
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), rm_device_get_name(device));
			if (rm_device_handles_number(device, numbers[idx])) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), dev_idx);
			}
			dev_idx++;
		}

		if (devices && gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)) == -1) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
		}

		g_signal_connect(G_OBJECT(combobox), "changed", G_CALLBACK(settings_numbers_device_combobox_changed_cb), numbers[idx]);

		gtk_box_pack_end(GTK_BOX(child), combobox, FALSE, FALSE, 0);

		/*GtkWidget *notification_label = gtk_label_new("with notification for");
		   gtk_widget_set_sensitive(notification_label, FALSE);
		   gtk_box_pack_start(GTK_BOX(child), notification_label, FALSE, FALSE, 0);

		   GtkWidget *notification_combobox = gtk_combo_box_text_new();
		   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(notification_combobox), _("Incoming"));
		   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(notification_combobox), _("Outgoing"));
		   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(notification_combobox), _("Both"));
		   //g_signal_connect(G_OBJECT(combobox), "changed", G_CALLBACK(settings_numbers_device_combobox_changed_cb), numbers[idx]);

		   gtk_box_pack_start(GTK_BOX(child), notification_combobox, TRUE, TRUE, 0);*/

		//gtk_widget_set_size_request(child, 100, 80);

		gtk_list_box_insert(GTK_LIST_BOX(listbox), child, -1);
	}
}

void settings_notification_populate(GtkWidget *list_box)
{
	RmProfile *profile = rm_profile_get_active();
	gchar **numbers = rm_router_get_numbers(profile);
	gint index;
	gchar **selected_outgoing_numbers;
	gchar **selected_incoming_numbers;

	selected_outgoing_numbers = rm_profile_get_notification_outgoing_numbers(profile);
	selected_incoming_numbers = rm_profile_get_notification_incoming_numbers(profile);

	for (index = 0; index < g_strv_length(numbers); index++) {
		GtkWidget *child = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
		GtkWidget *label = gtk_label_new(numbers[index]);
		GtkWidget *sub_label = gtk_label_new(_("notified on"));
		gint set = 0;

		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_widget_set_halign(sub_label, GTK_ALIGN_START);
		gtk_widget_set_sensitive(sub_label, FALSE);

		GtkWidget *l_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_margin(l_box, 6, 6, 6, 6);
		gtk_widget_set_halign(l_box, GTK_ALIGN_START);
		gtk_widget_set_hexpand(l_box, TRUE);

		gtk_box_pack_start(GTK_BOX(l_box), label, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(l_box), sub_label, TRUE, TRUE, 0);

		gtk_box_pack_start(GTK_BOX(child), l_box, TRUE, TRUE, 0);

		GtkWidget *combobox = gtk_combo_box_text_new();
		gtk_widget_set_halign(combobox, GTK_ALIGN_END);
		gtk_widget_set_vexpand(combobox, FALSE);
		gtk_widget_set_valign(combobox, GTK_ALIGN_CENTER);
		gtk_widget_set_margin(combobox, 6, 6, 6, 6);

		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), _("None"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), _("Incoming"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), _("Outgoing"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), _("All"));

		g_signal_connect(G_OBJECT(combobox), "changed", G_CALLBACK(settings_notification_combobox_changed_cb), numbers[index]);

		gtk_box_pack_end(GTK_BOX(child), combobox, FALSE, FALSE, 0);

		gtk_list_box_insert(GTK_LIST_BOX(list_box), child, -1);

		if (rm_strv_contains((const gchar*const*)selected_incoming_numbers, numbers[index])) {
			set += 1;
		}

		if (rm_strv_contains((const gchar*const*)selected_outgoing_numbers, numbers[index])) {
			set += 2;
		}

		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), set);
	}
}

void fax_ghostscript_file_chooser_button_file_set_cb(GtkWidget *button, gpointer user_data)
{
	RmProfile *profile = rm_profile_get_active();
	gchar *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));;

	g_settings_set_string(profile->settings, "ghostscript", file);
}

void update_numbers(gboolean active, const gchar *number)
{
	GSList *devices = rm_device_get_plugins();
	GSList *list;
	/* This one is hard-coded as we are currently supporting CAPI and Call Monitor only, once
	 * SIP is implemented we need to break this one up */
	RmDevice *new_device = active ? rm_device_get("CAPI") : rm_device_get("Call Monitor");
	RmDevice *old_device = NULL;
	gchar **numbers;

	for (list = devices; list != NULL; list = list->next) {
		RmDevice *device = list->data;

		if (rm_device_handles_number(device, (gchar*)number)) {
			old_device = device;
			break;
		}
	}

	//Remove number
	if (old_device) {
		numbers = rm_strv_remove(rm_device_get_numbers(old_device), number);

		rm_device_set_numbers(old_device, numbers);
	}

	//Add number
	if (new_device) {
		numbers = rm_strv_add(rm_device_get_numbers(new_device), number);

		rm_device_set_numbers(new_device, numbers);
	}
}

void phone_active_switch_activate_cb(GtkSwitch *widget, gpointer user_data)
{
	const gchar *number = gtk_combo_box_get_active_id(GTK_COMBO_BOX(settings->softphone_msn_combobox));
	gboolean active = gtk_switch_get_active(widget);

	g_debug("%s(): Updating numbers", __FUNCTION__);
	update_numbers(active, number);
}

void fax_active_switch_activate_cb(GtkSwitch *widget, gpointer user_data)
{
	const gchar *number = gtk_combo_box_get_active_id(GTK_COMBO_BOX(settings->fax_msn_combobox));
	gboolean active = gtk_switch_get_active(widget);

	g_debug("%s(): Updating numbers", __FUNCTION__);
	update_numbers(active, number);
}

void numbers_header_func(GtkListBoxRow *row,
			 GtkListBoxRow *before,
			 gpointer user_data)
{
	GtkWidget *current;

	if (!before) {
		gtk_list_box_row_set_header(row, NULL);
		return;
	}

	current = gtk_list_box_row_get_header(row);
	if (!current) {
		current = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_widget_show(current);
		gtk_list_box_row_set_header(row, current);
	}
}

void fax_msn_changed_cb(GtkWidget *widget,
			gpointer user_data)
{
	const gchar *number = gtk_combo_box_get_active_id(GTK_COMBO_BOX(settings->fax_msn_combobox));
	gboolean active = gtk_switch_get_active(GTK_SWITCH(settings->fax_active_switch));

	g_debug("%s(): Updating numbers", __FUNCTION__);
	update_numbers(active, number);
}

void app_show_settings(void)
{
	GtkBuilder *builder;
	const gchar *box_name;
	const gchar *firmware_name;
	gchar *box_string;
	RmProfile *profile;
	gboolean is_cable;
	gchar **numbers;
	gint idx;
	GSList *audio_plugins;
	GSList *list;

	if (settings) {
		gtk_window_present(GTK_WINDOW(settings->window));
		return;
	}

	builder = gtk_builder_new_from_resource("/org/tabos/roger/settings.glade");
	if (!builder) {
		g_warning("Could not load settings ui");
		return;
	}

	profile = rm_profile_get_active();
	if (!profile) {
		g_warning("No active profile, exiting settings");
		return;
	}

	settings = g_slice_new0(struct settings);

	/* Connect to builder objects */
	settings->window = GTK_WIDGET(gtk_builder_get_object(builder, "settings"));
	gtk_window_set_transient_for(GTK_WINDOW(settings->window), GTK_WINDOW(journal_get_window()));

	settings->headerbar = GTK_WIDGET(gtk_builder_get_object(builder, "headerbar"));

	/* Router group */
	settings->host_entry = GTK_WIDGET(gtk_builder_get_object(builder, "host_entry"));
	g_settings_bind(profile->settings, "host", settings->host_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->login_user_entry = GTK_WIDGET(gtk_builder_get_object(builder, "login_user_entry"));
	g_settings_bind(profile->settings, "login-user", settings->login_user_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->login_password_entry = GTK_WIDGET(gtk_builder_get_object(builder, "login_password_entry"));
	gtk_entry_set_text(GTK_ENTRY(settings->login_password_entry), rm_router_get_login_password(profile));

	settings->ftp_user_entry = GTK_WIDGET(gtk_builder_get_object(builder, "ftp_user_entry"));
	g_settings_bind(profile->settings, "ftp-user", settings->ftp_user_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->ftp_password_entry = GTK_WIDGET(gtk_builder_get_object(builder, "ftp_password_entry"));
	gtk_entry_set_text(GTK_ENTRY(settings->ftp_password_entry), rm_router_get_ftp_password(profile));

	if (!rm_router_need_ftp(profile)) {
		GtkWidget *tmp = GTK_WIDGET(gtk_builder_get_object(builder, "ftp_login_check_button"));

		g_debug("%s(): Disable ftp data", __FUNCTION__);
		gtk_widget_set_sensitive(settings->ftp_user_entry, FALSE);
		gtk_widget_set_sensitive(settings->ftp_password_entry, FALSE);
		gtk_widget_set_sensitive(tmp, FALSE);
	}

	/* Prefix group */
	is_cable = rm_router_is_cable(profile);

	settings->line_access_code_entry = GTK_WIDGET(gtk_builder_get_object(builder, "external_access_code_entry"));
	settings->international_access_code_entry = GTK_WIDGET(gtk_builder_get_object(builder, "international_access_code_entry"));
	settings->national_access_code_entry = GTK_WIDGET(gtk_builder_get_object(builder, "national_access_code_entry"));
	settings->country_code_entry = GTK_WIDGET(gtk_builder_get_object(builder, "country_code_entry"));
	settings->area_code_entry = GTK_WIDGET(gtk_builder_get_object(builder, "area_code_entry"));

	g_settings_bind(profile->settings, "external-access-code", settings->line_access_code_entry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile->settings, "international-access-code", settings->international_access_code_entry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile->settings, "country-code", settings->country_code_entry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile->settings, "national-access-code", settings->national_access_code_entry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile->settings, "area-code", settings->area_code_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/*gtk_widget_set_sensitive(settings->international_access_code_entry, is_cable);
	   gtk_widget_set_sensitive(settings->country_code_entry, is_cable);
	   gtk_widget_set_sensitive(settings->national_access_code_entry, is_cable);
	   gtk_widget_set_sensitive(settings->area_code_entry, is_cable);*/

	is_cable = TRUE;
	gtk_editable_set_editable(GTK_EDITABLE(settings->international_access_code_entry), is_cable);
	gtk_editable_set_editable(GTK_EDITABLE(settings->country_code_entry), is_cable);
	gtk_editable_set_editable(GTK_EDITABLE(settings->national_access_code_entry), is_cable);
	gtk_editable_set_editable(GTK_EDITABLE(settings->area_code_entry), is_cable);

	/* Numbers group */
	settings->phone_active_switch = GTK_WIDGET(gtk_builder_get_object(builder, "phone_active_switch"));
	g_settings_bind(profile->settings, "phone-active", settings->phone_active_switch, "active", G_SETTINGS_BIND_DEFAULT);
	g_signal_connect(G_OBJECT(settings->phone_active_switch), "notify::active", G_CALLBACK(phone_active_switch_activate_cb), NULL);

	settings->softphone_msn_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "softphone_number_combobox"));
	numbers = rm_router_get_numbers(profile);
	for (idx = 0; idx < g_strv_length(numbers); idx++) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->softphone_msn_combobox), numbers[idx], numbers[idx]);
	}
	g_settings_bind(profile->settings, "phone-number", settings->softphone_msn_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	settings->softphone_controller_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "softphone_controller_combobox"));
	g_settings_bind(profile->settings, "phone-controller", settings->softphone_controller_combobox, "active", G_SETTINGS_BIND_DEFAULT);


	//settings->numbers_assign_listbox = GTK_WIDGET(gtk_builder_get_object(builder, "numbers_assign_listbox"));
	//settings_numbers_populate(settings->numbers_assign_listbox);

	/* Fax group */
	settings->fax_active_switch = GTK_WIDGET(gtk_builder_get_object(builder, "fax_active_switch"));
	g_settings_bind(profile->settings, "fax-active", settings->fax_active_switch, "active", G_SETTINGS_BIND_DEFAULT);
	g_signal_connect(G_OBJECT(settings->fax_active_switch), "notify::active", G_CALLBACK(fax_active_switch_activate_cb), NULL);

	/*settings->fax_plugin_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "fax_plugin_combobox"));
	   fax_plugins = rm_fax_get_plugins();
	   for (list = fax_plugins; list != NULL; list = list->next) {
	   RmFax *fax = list->data;
	   gchar *name;

	   g_assert(fax != NULL);

	   name = rm_fax_get_name(fax);
	   g_debug("Adding fax '%s'", name);

	   gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->fax_plugin_combobox), name, name);
	   g_free(name);
	   }
	   g_settings_bind(rm_profile_get_active()->settings, "fax-plugin", settings->fax_plugin_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);*/

	/* Devices group */

	settings->fax_header_entry = GTK_WIDGET(gtk_builder_get_object(builder, "fax_header_entry"));
	g_settings_bind(profile->settings, "fax-header", settings->fax_header_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->fax_ident_entry = GTK_WIDGET(gtk_builder_get_object(builder, "fax_ident_entry"));
	g_settings_bind(profile->settings, "fax-ident", settings->fax_ident_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->fax_msn_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "fax_number_combobox"));
	numbers = rm_router_get_numbers(profile);
	for (idx = 0; idx < g_strv_length(numbers); idx++) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->fax_msn_combobox), numbers[idx], numbers[idx]);
	}
	g_settings_bind(profile->settings, "fax-number", settings->fax_msn_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_signal_connect(settings->fax_msn_combobox, "changed", G_CALLBACK(fax_msn_changed_cb), NULL);

	settings->fax_controller_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "fax_controller_combobox"));
	g_settings_bind(profile->settings, "fax-controller", settings->fax_controller_combobox, "active", G_SETTINGS_BIND_DEFAULT);

	settings->fax_service_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "fax_service_combobox"));
	g_settings_bind(profile->settings, "fax-cip", settings->fax_service_combobox, "active", G_SETTINGS_BIND_DEFAULT);

	settings->fax_resolution_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "fax_resolution_combobox"));
	g_settings_bind(profile->settings, "fax-resolution", settings->fax_resolution_combobox, "active", G_SETTINGS_BIND_DEFAULT);

	settings->fax_report_switch = GTK_WIDGET(gtk_builder_get_object(builder, "fax_report_switch"));
	g_settings_bind(profile->settings, "fax-report", settings->fax_report_switch, "active", G_SETTINGS_BIND_DEFAULT);

	settings->fax_report_directory_chooser = GTK_WIDGET(gtk_builder_get_object(builder, "fax_report_directory_chooser"));
	if (!gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(settings->fax_report_directory_chooser), g_settings_get_string(profile->settings, "fax-report-dir"))) {
		g_debug("%s(): Setting fallback directory", __FUNCTION__);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(settings->fax_report_directory_chooser), g_get_home_dir());
	}

	//g_settings_bind(profile->settings, "fax-cip", settings->fax_cip_combobox, "active", G_SETTINGS_BIND_DEFAULT);

	settings->fax_ecm_switch = GTK_WIDGET(gtk_builder_get_object(builder, "fax_ecm_switch"));
	g_settings_bind(profile->settings, "fax-ecm", settings->fax_ecm_switch, "active", G_SETTINGS_BIND_DEFAULT);

#ifdef G_OS_WIN32
	settings->fax_ghostscript_label = GTK_WIDGET(gtk_builder_get_object(builder, "fax_ghostscript_label"));
	gtk_widget_set_no_show_all(settings->fax_ghostscript_label, FALSE);
	settings->fax_ghostscript_file_chooser_button = GTK_WIDGET(gtk_builder_get_object(builder, "fax_ghostscript_file_chooser_button"));
	gtk_widget_set_no_show_all(settings->fax_ghostscript_file_chooser_button, FALSE);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(settings->fax_ghostscript_file_chooser_button), g_settings_get_string(profile->settings, "ghostscript"));
	g_signal_connect(G_OBJECT(settings->fax_ghostscript_file_chooser_button), "file-set", G_CALLBACK(fax_ghostscript_file_chooser_button_file_set_cb), NULL);
#endif

	/* Journal group */
	settings->filter_liststore = GTK_LIST_STORE(gtk_builder_get_object(builder, "filter_liststore"));
	filter_refresh_list(settings->filter_liststore);

	/* Action group */
	settings->actions_liststore = GTK_LIST_STORE(gtk_builder_get_object(builder, "actions_liststore"));
	settings->incoming_call_rings_checkbutton = GTK_WIDGET(gtk_builder_get_object(builder, "incoming_call_rings_checkbutton"));
	settings->incoming_call_begins_checkbutton = GTK_WIDGET(gtk_builder_get_object(builder, "incoming_call_begins_checkbutton"));
	settings->incoming_call_ends_checkbutton = GTK_WIDGET(gtk_builder_get_object(builder, "incoming_call_ends_checkbutton"));
	settings->incoming_call_missed_checkbutton = GTK_WIDGET(gtk_builder_get_object(builder, "incoming_call_missed_checkbutton"));
	settings->outgoing_call_dial_checkbutton = GTK_WIDGET(gtk_builder_get_object(builder, "outgoing_call_dial_checkbutton"));
	settings->outgoing_call_begins_checkbutton = GTK_WIDGET(gtk_builder_get_object(builder, "outgoing_call_begins_checkbutton"));
	settings->outgoing_call_ends_checkbutton = GTK_WIDGET(gtk_builder_get_object(builder, "outgoing_call_ends_checkbutton"));
	action_refresh_list(settings->actions_liststore);

	/* Extended group */
	settings->audio_plugin_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "audio_plugin_combobox"));
	settings->audio_output_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "audio_output_combobox"));
	settings->audio_output_ringtone_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "audio_output_ringtone_combobox"));
	settings->audio_input_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "audio_input_combobox"));

	audio_plugins = rm_audio_get_plugins();
	for (list = audio_plugins; list != NULL; list = list->next) {
		RmAudio *audio = list->data;
		gchar *name;

		g_assert(audio != NULL);

		name = rm_audio_get_name(audio);

		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->audio_plugin_combobox), name, name);
		g_free(name);
	}

	g_signal_connect(settings->audio_plugin_combobox, "changed", G_CALLBACK(audio_plugin_combobox_changed_cb), NULL);

	g_settings_bind(rm_profile_get_active()->settings, "audio-output", settings->audio_output_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(rm_profile_get_active()->settings, "audio-output-ringtone", settings->audio_output_ringtone_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(rm_profile_get_active()->settings, "audio-input", settings->audio_input_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(rm_profile_get_active()->settings, "audio-plugin", settings->audio_plugin_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	if (audio_plugins) {
		if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(settings->audio_plugin_combobox))) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_plugin_combobox), 0);
		} else {
			audio_plugin_combobox_changed_cb(NULL, NULL);
		}
	}

	settings->notification_ringtone_switch = GTK_WIDGET(gtk_builder_get_object(builder, "notification_ringtone_switch"));
	g_settings_bind(profile->settings, "notification-play-ringtone", settings->notification_ringtone_switch, "active", G_SETTINGS_BIND_DEFAULT);

	settings->notification_listbox = GTK_WIDGET(gtk_builder_get_object(builder, "notification_listbox"));
	gtk_list_box_set_header_func(GTK_LIST_BOX(settings->notification_listbox), numbers_header_func, NULL, NULL);
	settings_notification_populate(settings->notification_listbox);

	/* Header bar information */
	box_name = rm_router_get_name(profile);
	firmware_name = rm_router_get_version(profile);
	box_string = g_strdup_printf(_("%s (Firmware: %s)"), box_name, firmware_name);

	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(settings->headerbar), box_string);
	g_free(box_string);

	gtk_builder_add_callback_symbols(builder,
					 "settings_destroy_cb", G_CALLBACK(settings_destroy_cb),
					 "reload_button_clicked_cb", G_CALLBACK(reload_button_clicked_cb),
					 "audio_plugin_combobox_changed_cb", G_CALLBACK(audio_plugin_combobox_changed_cb),
					 "actions_edit_button_clicked_cb", G_CALLBACK(actions_edit_button_clicked_cb),
					 "actions_remove_button_clicked_cb", G_CALLBACK(actions_remove_button_clicked_cb),
					 "actions_add_button_clicked_cb", G_CALLBACK(actions_add_button_clicked_cb),
					 "actions_treeview_button_press_event_cb", G_CALLBACK(actions_treeview_button_press_event_cb),
					 "actions_treeview_cursor_changed_cb", G_CALLBACK(actions_treeview_cursor_changed_cb),
					 "action_checkbutton_toggled_cb", G_CALLBACK(action_checkbutton_toggled_cb),
					 "filter_edit_button_clicked_cb", G_CALLBACK(filter_edit_button_clicked_cb),
					 "filter_remove_button_clicked_cb", G_CALLBACK(filter_remove_button_clicked_cb),
					 "filter_add_button_clicked_cb", G_CALLBACK(filter_add_button_clicked_cb),
					 "fax_report_directory_chooser_file_set_cb", G_CALLBACK(fax_report_directory_chooser_file_set_cb),
					 "fax_report_switch_activate_cb", G_CALLBACK(fax_report_switch_activate_cb),
					 "ftp_login_check_button_clicked_cb", G_CALLBACK(ftp_login_check_button_clicked_cb),
					 "login_check_button_clicked_cb", G_CALLBACK(login_check_button_clicked_cb),
					 "ftp_password_entry_changed_cb", G_CALLBACK(ftp_password_entry_changed_cb),
					 "login_password_entry_changed_cb", G_CALLBACK(login_password_entry_changed_cb),
					 "action_enable_renderer_toggled_cb", G_CALLBACK(action_enable_renderer_toggled_cb),
					 "phone_active_switch_activate_cb", G_CALLBACK(phone_active_switch_activate_cb),
					 "fax_active_switch_activate_cb", G_CALLBACK(fax_active_switch_activate_cb),
					 NULL);
	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	gtk_widget_show_all(settings->window);
}

