/*
 * Roger Router - Plugin Application Indicator
 * Copyright (c) 2013-2017 Dieter Schärf, Jan-Michael Brummer
 *
 * Roger Router
 * Copyright (c) 2012-2015 Jan-Michael Brummer
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

#include <libappindicator/app-indicator.h>

#include <rm/rm.h>

#include <roger/about.h>
#include <roger/application.h>
#include <roger/journal.h>
#include <roger/main.h>
#include <roger/phone.h>
#include <roger/uitools.h>
#include <roger/settings.h>

typedef struct {
	AppIndicator *indicator;
	GSettings *settings;
	guint signal_id;
	GAction *preferences;
	GAction *contacts;
	GAction *about;
	GAction *phone;
	GAction *plugins;
	GAction *quit;
	GAction *copy_ip;
	GAction *reconnect;
	GAction *journal;
	GAction *hideonquit;
	GAction *hideonstart;
} RmIndicatorPlugin;

void indicator_copy_ip(GtkWidget *widget,
                       gpointer   user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	g_action_activate(indicator_plugin->copy_ip, NULL);
}

void indicator_reconnect(GtkWidget *widget,
                       gpointer   user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	g_action_activate(indicator_plugin->reconnect, NULL);
}
/**
 * indicator_menu_functions:
 *
 * Create "Functions" submenu items
 *
 * Returns: new menu widget
 */
static GtkWidget *indicator_menu_functions(RmIndicatorPlugin *indicator_plugin)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	/* Functions - Copy IP adress */
	item = gtk_menu_item_new_with_label(_("Copy IP address"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_copy_ip), indicator_plugin);

	/* Functions - Reconnect */
	item = gtk_menu_item_new_with_label(_("Reconnect"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_reconnect), indicator_plugin);

	return menu;
}

/**
 * \brief "journal" callback function
 * \param none
 */
void indicator_journal_cb(GtkWidget *widget, gpointer user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	app_indicator_set_status(indicator_plugin->indicator, APP_INDICATOR_STATUS_ACTIVE);
	g_action_activate(indicator_plugin->journal, NULL);
}

void indicator_phone_cb(GtkWidget *widget, gpointer user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	g_action_activate(indicator_plugin->phone, NULL);
}

void indicator_show_contacts(GtkWidget *widget,
                             gpointer   user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	g_action_activate(indicator_plugin->contacts, NULL);
}

void indicator_show_plugins(GtkWidget *widget,
                             gpointer   user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	g_action_activate(indicator_plugin->plugins, NULL);
}

void indicator_show_about(GtkWidget *widget,
                             gpointer   user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	g_action_activate(indicator_plugin->about, NULL);
}

void indicator_quit(GtkWidget *widget,
                             gpointer   user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	g_action_activate(indicator_plugin->quit, NULL);
}

void indicator_show_preferences(GtkWidget *widget,
                             gpointer   user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	g_action_activate(indicator_plugin->preferences, NULL);
}
/**
 * \brief create menu
 * \param none
 * \return new menu widget
 */
GtkWidget *indicator_menu(RmIndicatorPlugin *indicator_plugin)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *submenu;

	menu = gtk_menu_new();

	/* Journal */
	item = gtk_menu_item_new_with_label(_("Journal"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_journal_cb), indicator_plugin);

	/* Contacts */
	item = gtk_menu_item_new_with_label(_("Contacts"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_show_contacts), indicator_plugin);

	/* Phone */
	item = gtk_menu_item_new_with_label(_("Phone"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_phone_cb), indicator_plugin);

	/* Functions */
	item = gtk_menu_item_new_with_label(_("Functions"));
	submenu = indicator_menu_functions(indicator_plugin);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Plugins */
	item = gtk_menu_item_new_with_label(_("Plugins"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_show_plugins), indicator_plugin);

	/* Preferences */
	item = gtk_menu_item_new_with_label(_("Preferences"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_show_preferences), indicator_plugin);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* About */
	item = gtk_menu_item_new_with_label(_("About"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_show_about), indicator_plugin);

	/* Quit */
	item = gtk_menu_item_new_with_label(_("Quit"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_quit), indicator_plugin);

	gtk_widget_show_all(menu);

	return menu;
}

/**
 * \brief "connection-changed" callback function
 * \param object app object
 * \param event event type
 * \param connection connection structure
 * \param unused_pointer unused user pointer
 */
void indicator_connection_changed_cb(RmObject *object, gint event, RmConnection *connection, gpointer user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;

	if ((connection->type & ~RM_CONNECTION_TYPE_SOFTPHONE) == RM_CONNECTION_TYPE_MISSED) {
		app_indicator_set_status(indicator_plugin->indicator, APP_INDICATOR_STATUS_ATTENTION);
	}
}

/**
 * \brief "connection-default-icon" callback function
 * \param widget gtk combo box
 * \param unused_pointer unused user pointer
 */
void indicator_combobox_default_changed_cb(GtkComboBox *widget, gpointer user_data)
{
	/* GSettings has not written the changed value to its container, so we explicit set it here */
	RmIndicatorPlugin *indicator_plugin = user_data;

	g_settings_set_string(indicator_plugin->settings, "default-icon", gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget)));

	/* Update indicator icon */
	gchar *iconname = g_strconcat("org.tabos.roger.", g_settings_get_string(indicator_plugin->settings, "default-icon"), NULL);
	app_indicator_set_icon_full(indicator_plugin->indicator, iconname, "default-icon");
	g_free(iconname);
}

/**
 * \brief "connection-notify-icon" callback function
 * \param widget gtk combo box
 * \param unused_pointer unused user pointer
 */
void indicator_combobox_notify_changed_cb(GtkComboBox *widget, gpointer user_data)
{
	RmIndicatorPlugin *indicator_plugin = user_data;
	/* GSettings has not written the changed value to its container, so we explicit set it here */

	g_settings_set_string(indicator_plugin->settings, "notify-icon", gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget)));

	/* Update indicator attention icon */
	gchar *iconname = g_strconcat("org.tabos.roger.", g_settings_get_string(indicator_plugin->settings, "notify-icon"), NULL);
	app_indicator_set_attention_icon_full(indicator_plugin->indicator, iconname, "notify-icon");
	g_free(iconname);
}

static void indicator_set_hide_on_quit(RmIndicatorPlugin *indicator_plugin,
                                       gboolean           state)
{
	g_action_activate(indicator_plugin->hideonquit, g_variant_new_boolean(state));
}

static void indicator_set_hide_on_start(RmIndicatorPlugin *indicator_plugin,
                                       gboolean           state)
{
	g_action_activate(indicator_plugin->hideonstart, g_variant_new_boolean(state));
}

gboolean indicator_plugin_init(RmPlugin *plugin)
{
	RmIndicatorPlugin *indicator_plugin = g_slice_alloc0(sizeof(RmIndicatorPlugin));
	GApplication *app = g_application_get_default();
	GtkWidget *menu;

	indicator_plugin->preferences = g_action_map_lookup_action(G_ACTION_MAP(app), "preferences");
	indicator_plugin->contacts = g_action_map_lookup_action(G_ACTION_MAP(app), "addressbook");
	indicator_plugin->quit = g_action_map_lookup_action(G_ACTION_MAP(app), "quit");
	indicator_plugin->phone = g_action_map_lookup_action(G_ACTION_MAP(app), "phone");
	indicator_plugin->about = g_action_map_lookup_action(G_ACTION_MAP(app), "about");
	indicator_plugin->plugins = g_action_map_lookup_action(G_ACTION_MAP(app), "plugins");
	indicator_plugin->copy_ip = g_action_map_lookup_action(G_ACTION_MAP(app), "copy_ip");
	indicator_plugin->reconnect = g_action_map_lookup_action(G_ACTION_MAP(app), "reconnect");
	indicator_plugin->journal = g_action_map_lookup_action(G_ACTION_MAP(app), "journal");
	indicator_plugin->hideonquit = g_action_map_lookup_action(G_ACTION_MAP(app), "hideonquit");
	indicator_plugin->hideonstart = g_action_map_lookup_action(G_ACTION_MAP(app), "hideonstart");

	plugin->priv = indicator_plugin;
	indicator_set_hide_on_quit(indicator_plugin, TRUE);

	indicator_plugin->settings = rm_settings_new("org.tabos.roger.plugins.indicator");

	/* Create Application Indicator */
	gchar *iconname = g_strconcat("org.tabos.roger.", g_settings_get_string(indicator_plugin->settings, "default-icon"), NULL);
	indicator_plugin->indicator = app_indicator_new("org.tabos.roger", iconname, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);

	iconname = g_strconcat("org.tabos.roger.", g_settings_get_string(indicator_plugin->settings, "notify-icon"), NULL);
	app_indicator_set_attention_icon_full(indicator_plugin->indicator, iconname, "notify-icon");
	g_free(iconname);

	menu = indicator_menu(indicator_plugin);
	app_indicator_set_menu(indicator_plugin->indicator, GTK_MENU(menu));

	app_indicator_set_status(indicator_plugin->indicator, APP_INDICATOR_STATUS_ACTIVE);

	/* Connect to "connection-changed" signal */
	indicator_plugin->signal_id = g_signal_connect(G_OBJECT(rm_object), "connection-changed", G_CALLBACK(indicator_connection_changed_cb), indicator_plugin);

	if (g_settings_get_boolean(indicator_plugin->settings, "hide-journal-on-startup")) {
		indicator_set_hide_on_start(indicator_plugin, TRUE);
	}

	return TRUE;
}

gboolean indicator_plugin_shutdown(RmPlugin *plugin)
{
	RmIndicatorPlugin *indicator_plugin = plugin->priv;

	/* Unregister delete handler */
	indicator_set_hide_on_quit(indicator_plugin, FALSE);

	/* If signal handler is connected: disconnect */
	if (g_signal_handler_is_connected(G_OBJECT(rm_object), indicator_plugin->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(rm_object), indicator_plugin->signal_id);
	}

	app_indicator_set_status(indicator_plugin->indicator, APP_INDICATOR_STATUS_PASSIVE);
	g_clear_object(&indicator_plugin->indicator);

	plugin->priv = NULL;

	return TRUE;
}

gpointer indicator_plugin_configure(RmPlugin *plugin)
{
	GtkWidget *settings_grid;
	GtkWidget *label;
	GtkWidget *switch_journal;
	GtkWidget *combo_box_default;
	GtkWidget *combo_box_notify;
	GtkWidget *group;
	RmIndicatorPlugin *indicator_plugin = plugin->priv;

	/* Settings grid */
	settings_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(settings_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(settings_grid), 15);

	/* Create label and switch for "Hide Journal on startup" and add to grid */
	label = ui_label_new(_("Hide Journal on startup"));
	gtk_grid_attach(GTK_GRID(settings_grid), label, 0, 0, 1, 1);

	switch_journal = gtk_switch_new();
	gtk_switch_set_active(GTK_SWITCH(switch_journal), g_settings_get_boolean(indicator_plugin->settings, "hide-journal-on-startup"));
	gtk_widget_set_halign(switch_journal, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(settings_grid), switch_journal, 1, 0, 1, 1);

	/* Create label and combo_box for "Default Icon" and add to grid */
	label = ui_label_new(_("Default Icon"));
	gtk_grid_attach(GTK_GRID(settings_grid), label, 0, 1, 1, 1);

	combo_box_default = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_box_default), "default", _("default"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_box_default), "mono-dark", _("mono-dark"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_box_default), "mono-lite", _("mono-lite"));
	gtk_grid_attach(GTK_GRID(settings_grid), combo_box_default, 1, 1, 1, 1);

	/* Create label and combo_box for "Notify Icon" and add to grid */
	label = ui_label_new(_("Notify Icon"));
	gtk_grid_attach(GTK_GRID(settings_grid), label, 0, 2, 1, 1);

	combo_box_notify = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_box_notify), "notify", _("default"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_box_notify), "mono-dark", _("mono-dark"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_box_notify), "mono-lite", _("mono-lite"));
	gtk_grid_attach(GTK_GRID(settings_grid), combo_box_notify, 1, 2, 1, 1);

	/* Set signals */
	g_settings_bind(indicator_plugin->settings, "hide-journal-on-startup", switch_journal, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(indicator_plugin->settings, "default-icon", combo_box_default, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(indicator_plugin->settings, "notify-icon", combo_box_notify, "active-id", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(combo_box_default, "changed", G_CALLBACK(indicator_combobox_default_changed_cb), indicator_plugin);
	g_signal_connect(combo_box_notify, "changed", G_CALLBACK(indicator_combobox_notify_changed_cb), indicator_plugin);

	group = ui_group_create(settings_grid, _("Settings for Application Indicator"), TRUE, TRUE);

	return group;
}

RM_PLUGIN_CONFIG(indicator);
