/**
 * Roger Router
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/router.h>
#include <libroutermanager/net_monitor.h>
#include <libroutermanager/ftp.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/password.h>
#include <libroutermanager/action.h>
#include <libroutermanager/audio.h>
#include <libroutermanager/address-book.h>

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
	GtkWidget *international_call_prefix_entry;
	GtkWidget *national_call_prefix_entry;
	GtkWidget *line_access_code_entry;
	GtkWidget *area_code_entry;
	GtkWidget *country_code_entry;
	GtkWidget *softphone_msn_combobox;
	GtkWidget *softphone_controller_combobox;
	GtkWidget *fax_header_entry;
	GtkWidget *fax_ident_entry;
	GtkWidget *fax_msn_combobox;
	GtkWidget *fax_controller_combobox;
	GtkWidget *fax_service_combobox;
	GtkWidget *fax_resolution_combobox;
	GtkWidget *fax_report_switch;
	GtkWidget *fax_report_directory_chooser;
	GtkWidget *fax_ecm_switch;
	GtkWidget *view_call_type_icons_combobox;
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
	GtkWidget *audio_input_combobox;
	GtkWidget *security_plugin_combobox;
	GtkWidget *address_book_plugin_combobox;
};

static struct settings *settings = NULL;


/**
 * \brief Verify that router password is valid
 * \param button verify button widget
 * \param user_data user data pointer (NULL)
 */
void login_check_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog;
	struct profile *profile = profile_get_active();
	gboolean locked = router_is_locked();
	gboolean log_in = FALSE;

	router_logout(profile);

	router_release_lock();
	log_in = router_login(profile);
	if (log_in == TRUE) {
		dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, _("Login password is valid"));

		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		if (locked) {
			net_monitor_reconnect();
		}
	} else {
		//dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Login password is invalid"));
		// This case is handled by libroutermanager emitting a new message
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
	struct ftp *client;
	struct profile *profile = profile_get_active();

	client = ftp_init(router_get_host(profile));
	if (!client) {
		dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not connect to FTP. Missing USB storage?"));
	} else {
		if (ftp_login(client, router_get_ftp_user(profile), router_get_ftp_password(profile)) == TRUE) {
			dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, _("FTP password is valid"));
		} else {
			emit_message(_("FTP password is invalid"), _("Please check your FTP credentials"));
		}
		ftp_shutdown(client);
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

	router_get_settings(profile_get_active());
}

/**
 * \brief Router login password changed callback - set password in password manager
 * \param entry password entry widget
 * \param user_data UNUSED
 */
void login_password_entry_changed_cb(GtkEntry *entry, gpointer user_data)
{
	struct profile *profile = profile_get_active();

	password_manager_set_password(profile, "login-password", gtk_entry_get_text(GTK_ENTRY(entry)));
}

/**
 * \brief FTP login password changed callback - set password in password manager
 * \param entry password entry widget
 * \param user_data UNUSED
 */
void ftp_password_entry_changed_cb(GtkEntry *entry, gpointer user_data)
{
	struct profile *profile = profile_get_active();

	password_manager_set_password(profile, "ftp-password", gtk_entry_get_text(GTK_ENTRY(entry)));
}

/**
 * \brief Report dir set callback - store it in the settings
 * \param chooser file chooser widget
 * \param user_data user data pointer (NULL)
 */
void fax_report_directory_chooser_file_set_cb(GtkFileChooserButton *chooser, gpointer user_data)
{
	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));

	g_settings_set_string(profile_get_active()->settings, "fax-report-dir", filename);

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

	for (list = filter_get_list(); list != NULL; list = list->next) {
		GtkTreeIter iter;
		struct filter *filter = list->data;

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
	GtkWidget *image;

	if (!rule) {
		rule = g_slice_new0(struct filter_rule);
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
	struct filter *filter;
	GtkWidget *dialog;
	GtkWidget *grid;
	GtkWidget *content;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *save;
	GSList *list;
	struct filter_rule *rule;
	GValue ptr = {0};
	GtkListStore *list_store;
	gint result;
	gboolean use_header = roger_uses_headerbar();

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
	filter_save();
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
	struct filter *filter = NULL;
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

	filter_remove(filter);

	g_value_unset(&ptr);

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
	struct filter *filter;
	gint result;
	gboolean use_header = roger_uses_headerbar();

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

	filter = filter_new(name);
	filter->rules = pref_filters_current_rules;
	filter_add(filter);

	gtk_widget_destroy(dialog);

	model = gtk_tree_view_get_model(data);
	list_store = GTK_LIST_STORE(model);
	gtk_list_store_clear(list_store);
	filter_refresh_list(list_store);

	journal_update_filter();
	filter_save();
}

static struct action *selected_action = NULL;
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
	GSList *list = action_get_list(profile_get_active());
	GtkTreeIter iter;

	while (list) {
		struct action *action = list->data;

		gtk_list_store_insert_with_values(list_store, &iter, -1,
			0, action->name,
			1, action, -1);
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

void action_enable_renderer_toggled_cb(GtkCellRendererToggle *toggle, gchar *path_str, gpointer user_data)
{
	GtkTreeModel *model = gtk_tree_view_get_model(user_data);
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
	GtkTreeIter iter;
	GValue iter_value = {0};
	GValue name_value = {0};
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

gboolean action_edit(struct action *action)
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
		gtk_entry_set_text(GTK_ENTRY(name_entry), action->name);
		gtk_entry_set_text(GTK_ENTRY(exec_entry), action->exec);
	}
	settings_refresh_list(list_store);

	result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result == 1) {
		if (!action) {
			action = action_create();
			action_add(profile_get_active(), action);
			selected_action = action;
		}

		action = action_modify(action, gtk_entry_get_text(GTK_ENTRY(name_entry)), "", gtk_entry_get_text(GTK_ENTRY(exec_entry)), selected_numbers);
		action_commit(profile_get_active());

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
			action_add(profile_get_active(), action);
			selected_action = action;
		}

		action = action_modify(action, gtk_entry_get_text(GTK_ENTRY(name_entry)), "", gtk_entry_get_text(GTK_ENTRY(exec_entry)), selected_numbers);
		action_commit(profile_get_active());

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
	struct action *action = NULL;
	GValue ptr = { 0 };

	if (!gtk_tree_selection_get_selected(selection, &model, &selected_iter)) {
		return;
	}

	gtk_tree_model_get_value(model, &selected_iter, 1, &ptr);

	action = g_value_get_pointer(&ptr);
	GtkWidget *remove_dialog = gtk_message_dialog_new(GTK_WINDOW(settings->window), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Do you want to delete the action '%s'?"), action->name);
	gtk_window_set_title(GTK_WINDOW(remove_dialog), _("Delete action"));
	gtk_window_set_position(GTK_WINDOW(remove_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gint result = gtk_dialog_run(GTK_DIALOG(remove_dialog));
	gtk_widget_destroy(remove_dialog);

	if (result != GTK_RESPONSE_OK) {
		g_value_unset(&ptr);
		return;
	}

	selected_action = NULL;

	g_value_unset(&ptr);

	action_remove(profile_get_active(), action);
	action_commit(profile_get_active());
	action_free(action);

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

gboolean actions_treeview_cursor_changed_cb(GtkTreeView *view, gpointer user_data)
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

		selected_action = action;
		if (action) {
			/* Store flags as otherwise we would mix up the settings */
			flags = selected_action->flags;

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_rings_checkbutton), flags & ACTION_INCOMING_RING);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_begins_checkbutton), flags & ACTION_INCOMING_BEGIN);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_ends_checkbutton), flags & ACTION_INCOMING_END);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_rings_checkbutton), flags & ACTION_INCOMING_RING);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_begins_checkbutton), flags & ACTION_INCOMING_BEGIN);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_ends_checkbutton), flags & ACTION_INCOMING_END);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->incoming_call_missed_checkbutton), flags & ACTION_INCOMING_MISSED);

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_begins_checkbutton), flags & ACTION_OUTGOING_BEGIN);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_ends_checkbutton), flags & ACTION_OUTGOING_END);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_dial_checkbutton), flags & ACTION_OUTGOING_DIAL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_begins_checkbutton), flags & ACTION_OUTGOING_BEGIN);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_ends_checkbutton), flags & ACTION_OUTGOING_END);

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
		flags |= ACTION_INCOMING_RING;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->incoming_call_begins_checkbutton))) {
		flags |= ACTION_INCOMING_BEGIN;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->incoming_call_ends_checkbutton))) {
		flags |= ACTION_INCOMING_END;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->incoming_call_missed_checkbutton))) {
		flags |= ACTION_INCOMING_MISSED;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_dial_checkbutton))) {
		flags |= ACTION_OUTGOING_DIAL;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_begins_checkbutton))) {
		flags |= ACTION_OUTGOING_BEGIN;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->outgoing_call_ends_checkbutton))) {
		flags |= ACTION_OUTGOING_END;
	}

	selected_action->flags = flags;
	g_settings_set_int(selected_action->settings, "flags", flags);
}

/**
 * \brief Update device comboboxes depending on plugin combobox
 * \param box plugin combobox
 * \param user_data UNUSED
 */
void audio_plugin_combobox_changed_cb(GtkComboBox *box, gpointer user_data) {
	GSList *devices;
	gchar *active;
	struct audio *audio = NULL;
	GSList *audio_plugins;
	GSList *list;
	gchar *input_name;
	gchar *output_name;
	gint input_cnt = 0;
	gint output_cnt = 0;

	input_name = g_settings_get_string(profile_get_active()->settings, "audio-input");
	output_name = g_settings_get_string(profile_get_active()->settings, "audio-output");

	/* Clear device comboboxes */
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(settings->audio_output_combobox));
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(settings->audio_input_combobox));

	/* Read active audio plugin */
	active = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(settings->audio_plugin_combobox));

	/* Find active audio plugin structure */
	audio_plugins = audio_get_plugins();
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
		struct audio_device *device = list->data;

		if (device->type == AUDIO_INPUT) {
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->audio_input_combobox), device->internal_name, device->name);

			if (!strcmp(device->internal_name, input_name)) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_input_combobox), input_cnt);
			}

			input_cnt++;
		} else {
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->audio_output_combobox), device->internal_name, device->name);

			if (!strcmp(device->internal_name, output_name)) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_output_combobox), output_cnt);
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
	}

	audio_set_default(active);
}

void view_call_type_icons_combobox_changed_cb(GtkComboBox *widget, gpointer user_data)
{
	g_settings_set_uint(app_settings, "icon-type", gtk_combo_box_get_active(widget));
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

	plugins = address_book_get_plugins();
	for (list = plugins; list; list = list->next) {
		struct address_book *ab = list->data;

		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->address_book_plugin_combobox), ab->name, ab->name);
	}
}

void settings_notebook_switch_page_cb(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data)
{
	struct profile *profile;

	if (page_num != 5) {
		return;
	}

	profile = profile_get_active();
	g_settings_unbind(profile->settings, "address-book");

	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(settings->address_book_plugin_combobox));
	settings_fill_address_book_plugin_combobox(settings);
	g_settings_bind(profile->settings, "address-book", settings->address_book_plugin_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(settings->address_book_plugin_combobox))) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(settings->address_book_plugin_combobox), 0);
	}
}

void app_show_settings(void)
{
	GtkBuilder *builder;
	const gchar *box_name;
	const gchar *firmware_name;
	gchar *box_string;
	struct profile *profile;
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

	settings = g_slice_new(struct settings);
	profile = profile_get_active();

	/* Connect to builder objects */
	settings->window = GTK_WIDGET(gtk_builder_get_object(builder, "settings"));
	gtk_window_set_transient_for(GTK_WINDOW(settings->window), GTK_WINDOW(journal_get_window()));

	settings->headerbar = GTK_WIDGET(gtk_builder_get_object(builder, "headerbar"));

	if (roger_uses_headerbar()) {
		gtk_window_set_titlebar(GTK_WINDOW(settings->window), settings->headerbar);
		gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(settings->headerbar), TRUE);
	} else {
		gtk_window_set_title(GTK_WINDOW(settings->window), _("Settings"));
	}

	/* Router group */
	if (!roger_uses_headerbar()) {
		GtkWidget *reload_button_ssd = GTK_WIDGET(gtk_builder_get_object(builder, "reload_button_ssd"));
		gtk_widget_show(reload_button_ssd);
	}

	settings->host_entry = GTK_WIDGET(gtk_builder_get_object(builder, "host_entry"));
	g_settings_bind(profile->settings, "host", settings->host_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->login_user_entry = GTK_WIDGET(gtk_builder_get_object(builder, "login_user_entry"));
	g_settings_bind(profile->settings, "login-user", settings->login_user_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->login_password_entry = GTK_WIDGET(gtk_builder_get_object(builder, "login_password_entry"));
	gtk_entry_set_text(GTK_ENTRY(settings->login_password_entry), router_get_login_password(profile));

	settings->ftp_user_entry = GTK_WIDGET(gtk_builder_get_object(builder, "ftp_user_entry"));
	g_settings_bind(profile->settings, "ftp-user", settings->ftp_user_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->ftp_password_entry = GTK_WIDGET(gtk_builder_get_object(builder, "ftp_password_entry"));
	gtk_entry_set_text(GTK_ENTRY(settings->ftp_password_entry), router_get_ftp_password(profile));

	/* Prefix group */
	is_cable = router_is_cable(profile);

	settings->line_access_code_entry = GTK_WIDGET(gtk_builder_get_object(builder, "line_access_code_entry"));
	settings->international_call_prefix_entry = GTK_WIDGET(gtk_builder_get_object(builder, "international_call_prefix_entry"));
	settings->national_call_prefix_entry = GTK_WIDGET(gtk_builder_get_object(builder, "national_call_prefix_entry"));
	settings->country_code_entry = GTK_WIDGET(gtk_builder_get_object(builder, "country_code_entry"));
	settings->area_code_entry = GTK_WIDGET(gtk_builder_get_object(builder, "area_code_entry"));

	g_settings_bind(profile->settings, "line-access-code", settings->line_access_code_entry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile->settings, "international-call-prefix", settings->international_call_prefix_entry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile->settings, "country-code", settings->country_code_entry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile->settings, "national-call-prefix", settings->national_call_prefix_entry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile->settings, "area-code", settings->area_code_entry, "text", G_SETTINGS_BIND_DEFAULT);

	gtk_widget_set_sensitive(settings->international_call_prefix_entry, is_cable);
	gtk_widget_set_sensitive(settings->country_code_entry, is_cable);
	gtk_widget_set_sensitive(settings->national_call_prefix_entry, is_cable);
	gtk_widget_set_sensitive(settings->area_code_entry, is_cable);

	gtk_editable_set_editable(GTK_EDITABLE(settings->international_call_prefix_entry), is_cable);
	gtk_editable_set_editable(GTK_EDITABLE(settings->country_code_entry), is_cable);
	gtk_editable_set_editable(GTK_EDITABLE(settings->national_call_prefix_entry), is_cable);
	gtk_editable_set_editable(GTK_EDITABLE(settings->area_code_entry), is_cable);

	/* Devices group */
	settings->softphone_msn_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "softphone_msn_combobox"));
	numbers = router_get_numbers(profile);
	for (idx = 0; idx < g_strv_length(numbers); idx++) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->softphone_msn_combobox), numbers[idx], numbers[idx]);
	}
	g_settings_bind(profile->settings, "phone-number", settings->softphone_msn_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	settings->softphone_controller_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "softphone_controller_combobox"));
	g_settings_bind(profile->settings, "phone-controller", settings->softphone_controller_combobox, "active", G_SETTINGS_BIND_DEFAULT);
	
	settings->fax_header_entry = GTK_WIDGET(gtk_builder_get_object(builder, "fax_header_entry"));
	g_settings_bind(profile->settings, "fax-header", settings->fax_header_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->fax_ident_entry = GTK_WIDGET(gtk_builder_get_object(builder, "fax_ident_entry"));
	g_settings_bind(profile->settings, "fax-ident", settings->fax_ident_entry, "text", G_SETTINGS_BIND_DEFAULT);

	settings->fax_msn_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "fax_msn_combobox"));
	numbers = router_get_numbers(profile);
	for (idx = 0; idx < g_strv_length(numbers); idx++) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->fax_msn_combobox), numbers[idx], numbers[idx]);
	}
	g_settings_bind(profile->settings, "fax-number", settings->fax_msn_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

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

	/* Journal group */
	settings->view_call_type_icons_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "view_call_type_icons_combobox"));
	g_settings_bind(app_settings, "icon-type", settings->view_call_type_icons_combobox, "active", G_SETTINGS_BIND_DEFAULT);

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

	/* Plugins group - Workaround for Ubuntu 16.04 */
	GtkWidget *tmp = GTK_WIDGET(gtk_builder_get_object(builder, "plugins_box"));
	PeasEngine *peas = peas_engine_get_default();
	GtkWidget *manager = peas_gtk_plugin_manager_new(peas);
	gtk_box_pack_start(GTK_BOX(tmp), GTK_WIDGET(manager), TRUE, TRUE, 0);

	/* Extended group */
	settings->audio_plugin_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "audio_plugin_combobox"));
	settings->audio_output_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "audio_output_combobox"));
	settings->audio_input_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "audio_input_combobox"));

	audio_plugins = audio_get_plugins();
	for (list = audio_plugins; list != NULL; list = list->next) {
		struct audio *audio = list->data;

		g_assert(audio != NULL);

		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->audio_plugin_combobox), audio->name, audio->name);
	}

	g_signal_connect(settings->audio_plugin_combobox, "changed", G_CALLBACK(audio_plugin_combobox_changed_cb), NULL);

	g_settings_bind(profile_get_active()->settings, "audio-output", settings->audio_output_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile_get_active()->settings, "audio-input", settings->audio_input_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile_get_active()->settings, "audio-plugin", settings->audio_plugin_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	if (audio_plugins) {
		if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(settings->audio_plugin_combobox))) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(settings->audio_plugin_combobox), 0);
		} else {
				audio_plugin_combobox_changed_cb(NULL, NULL);
		}
	}

	/* Security group */
	GSList *plugins = password_manager_get_plugins();
	settings->security_plugin_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "security_plugin_combobox"));

	for (list = plugins; list; list = list->next) {
		struct password_manager *pm = list->data;

		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(settings->security_plugin_combobox), pm->name, pm->name);
	}

	if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(settings->security_plugin_combobox))) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(settings->security_plugin_combobox), 0);
	}

	g_settings_bind(profile->settings, "password-manager", settings->security_plugin_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	/* Address book group */
	settings->address_book_plugin_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "address_book_plugin_combobox"));
	//settings_fill_address_book_plugin_combobox(settings);
	//g_settings_bind(profile->settings, "address-book", settings->address_book_plugin_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	/* Header bar information */
	box_name = router_get_name(profile);
	firmware_name = router_get_version(profile);
	box_string = g_strdup_printf(_("%s (Firmware: %s)"), box_name, firmware_name);

	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(settings->headerbar), box_string);
	g_free(box_string);

	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	gtk_widget_show_all(settings->window);
}

