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

#include <string.h>

#include <gtk/gtk.h>

#include <rm/rm.h>

#include <roger/about.h>
#include <roger/application.h>
#include <roger/journal.h>
#include <roger/main.h>
#include <roger/phone.h>
#include <roger/uitools.h>
#include <roger/settings.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct {
	/*< private >*/
	GtkStatusIcon *statusicon;
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
} RmStatusIconPlugin;

/**
 * statusicon_copy_ip_activate_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * Activates action "copy_ip"
 */
void statusicon_copy_ip_activate_cb(GtkWidget *widget, gpointer user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_action_activate(statusicon_plugin->copy_ip, NULL);
}

/**
 * statusicon_reconnect_activate_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * Activates action "reconnect"
 */
void statusicon_reconnect_activate_cb(GtkWidget *widget, gpointer user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_action_activate(statusicon_plugin->reconnect, NULL);
}

/**
 * statusicon_menu_functions:
 * @statusicon_plugin: a #RmStatusIconPlugin
 *
 * Create "Functions" submenu items
 *
 * Returns: a new #GtkWidget menu
 */
GtkWidget *statusicon_menu_functions(RmStatusIconPlugin *statusicon_plugin)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	/* Functions - Copy IP adress */
	item = gtk_menu_item_new_with_label(_("Copy IP address"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_copy_ip_activate_cb), statusicon_plugin);

	/* Functions - Reconnect */
	item = gtk_menu_item_new_with_label(_("Reconnect"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_reconnect_activate_cb), statusicon_plugin);

	return menu;
}

/**
 * statusicon_journal_activate_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * Acticates action "journal"
 */
void statusicon_journal_activate_cb(GtkWidget *widget, gpointer user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	gchar *iconname = g_strconcat("roger-", g_settings_get_string(statusicon_plugin->settings, "default-icon"), NULL);
	gtk_status_icon_set_from_icon_name(statusicon_plugin->statusicon, iconname);
	g_free(iconname);

	g_action_activate(statusicon_plugin->journal, NULL);
}

/**
 * statusicon_phone_activate_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * Acticates action "phone"
 */
void statusicon_phone_activate_cb(GtkWidget *widget, gpointer user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_action_activate(statusicon_plugin->phone, NULL);
}

/**
 * statusicon_contacts_activate_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * Acticates action "contacts"
 */
void statusicon_contacts_activate_cb(GtkWidget *widget,
                             gpointer   user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_action_activate(statusicon_plugin->contacts, NULL);
}

/**
 * statusicon_plugins_activate_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * Acticates action "plugins"
 */
void statusicon_plugins_activate_cb(GtkWidget *widget,
                             gpointer   user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_action_activate(statusicon_plugin->plugins, NULL);
}

/**
 * statusicon_about_activate_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * Acticates action "about"
 */
void statusicon_about_activate_cb(GtkWidget *widget, gpointer user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_action_activate(statusicon_plugin->about, NULL);
}

/**
 * statusicon_quit_activate_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * Acticates action "quit"
 */
void statusicon_quit_activate_cb(GtkWidget *widget, gpointer user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_action_activate(statusicon_plugin->quit, NULL);
}

/**
 * statusicon_preferences_activate_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * Acticates action "preferences"
 */
void statusicon_preferences_activate_cb(GtkWidget *widget, gpointer user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_action_activate(statusicon_plugin->preferences, NULL);
}

/**
 * statusicon_popup_menu_cb:
 * @statusicon: a #GtkStatusIcon
 * @button: button number
 * @activate_time: activation time
 * @user_data: a #RmStatusIconPlugin
 *
 * Create and show popup menu
 */
void statusicon_popup_menu_cb(GtkStatusIcon *statusicon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *submenu;
	RmStatusIconPlugin *statusicon_plugin = user_data;

	menu = gtk_menu_new();

	/* Journal */
	item = gtk_menu_item_new_with_label(_("Journal"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_journal_activate_cb), statusicon_plugin);

	/* Contacts */
	item = gtk_menu_item_new_with_label(_("Contacts"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_contacts_activate_cb), statusicon_plugin);

	/* Phone */
	item = gtk_menu_item_new_with_label(_("Phone"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_phone_activate_cb), statusicon_plugin);

	/* Functions */
	item = gtk_menu_item_new_with_label(_("Functions"));
	submenu = statusicon_menu_functions(statusicon_plugin);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Plugins */
	item = gtk_menu_item_new_with_label(_("Plugins"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_plugins_activate_cb), statusicon_plugin);

	/* Preferences */
	item = gtk_menu_item_new_with_label(_("Preferences"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_preferences_activate_cb), statusicon_plugin);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* About */
	item = gtk_menu_item_new_with_label(_("About"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_about_activate_cb), statusicon_plugin);

	/* Quit */
	item = gtk_menu_item_new_with_label(_("Quit"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_quit_activate_cb), statusicon_plugin);

	gtk_widget_show_all(menu);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, statusicon_plugin->statusicon, button, activate_time);
}

/**
 * statusicon_connection_changed_cb:
 * @object: a #RmObject
 * @event: event id
 * @connection: a #RmConnection
 * @user_data: a #RmStatusIconPlugin
 *
 * "connection-changed" callback function. Set icon to notify in case of missed calls.
 */
void statusicon_connection_changed_cb(RmObject *object, gint event, RmConnection *connection, gpointer user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	if ((connection->type & ~RM_CONNECTION_TYPE_SOFTPHONE) == RM_CONNECTION_TYPE_MISSED) {
		gchar *iconname = g_strconcat("roger-", g_settings_get_string(statusicon_plugin->settings, "notify-icon"), NULL);
		gtk_status_icon_set_from_icon_name(statusicon_plugin->statusicon, iconname);
		g_free(iconname);
	}
}

/**
 * statusicon_combobox_default_changed_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * "connection-default-icon" callback function. Changes default icon type.
 */
void statusicon_combobox_default_changed_cb(GtkComboBox *widget, gpointer user_data)
{
	/* GSettings has not written the changed value to its container, so we explicit set it here */
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_settings_set_string(statusicon_plugin->settings, "default-icon", gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget)));

	/* Update statusicon icon */
	gchar *iconname = g_strconcat("roger-", g_settings_get_string(statusicon_plugin->settings, "default-icon"), NULL);
	gtk_status_icon_set_from_icon_name(statusicon_plugin->statusicon, iconname);
	g_free(iconname);
}

/**
 * statusicon_combobox_notify_changed_cb:
 * @widget: a #GtkWidget
 * @user_data: a #RmStatusIconPlugin
 *
 * "connection-notify-icon" callback function.
 */
void statusicon_combobox_notify_changed_cb(GtkComboBox *widget, gpointer user_data)
{
	/* GSettings has not written the changed value to its container, so we explicit set it here */
	RmStatusIconPlugin *statusicon_plugin = user_data;

	g_settings_set_string(statusicon_plugin->settings, "notify-icon", gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget)));
}

/**
 * add_statusicon:
 * @user_data: a #RmStatusIconPlugin
 *
 * Create and add new statusicon plugin
 *
 * Returns: %FALSE
 */
static gboolean add_statusicon(gpointer user_data)
{
	RmStatusIconPlugin *statusicon_plugin = user_data;

	gchar *iconname = g_strconcat("roger-", g_settings_get_string(statusicon_plugin->settings, "default-icon"), NULL);
	statusicon_plugin->statusicon = gtk_status_icon_new_from_icon_name(iconname);
	g_free(iconname);

	g_signal_connect(G_OBJECT(statusicon_plugin->statusicon), "popup-menu", G_CALLBACK(statusicon_popup_menu_cb), statusicon_plugin);
	g_signal_connect(G_OBJECT(statusicon_plugin->statusicon), "activate", G_CALLBACK(statusicon_journal_activate_cb), statusicon_plugin);

	gtk_status_icon_set_tooltip_text(statusicon_plugin->statusicon, _("Roger Router"));
	gtk_status_icon_set_visible(statusicon_plugin->statusicon, TRUE);

	return FALSE;
}

/**
 * statusicon_set_hide_on_quit:
 * @statusicon_plugin: a #RmStatusIconPlugin
 * @state: hide state
 */
static void statusicon_set_hide_on_quit(RmStatusIconPlugin *statusicon_plugin, gboolean state)
{
	g_action_activate(statusicon_plugin->hideonquit, g_variant_new_boolean(state));
}

/**
 * statusicon_set_hide_on_start:
 * @statusicon_plugin: a #RmStatusIconPlugin
 * @state: start state
 */
static void statusicon_set_hide_on_start(RmStatusIconPlugin *statusicon_plugin, gboolean state)
{
	g_action_activate(statusicon_plugin->hideonstart, g_variant_new_boolean(state));
}

/**
 * statusicon_plugin_init:
 * @plugin: a #RmPlugin
 *
 * Initialize statusicon plugin
 *
 * Returns: %TRUE
 */
gboolean statusicon_plugin_init(RmPlugin *plugin)
{
	RmStatusIconPlugin *statusicon_plugin = g_slice_new0(RmStatusIconPlugin);
	GApplication *app = g_application_get_default();

	statusicon_plugin->preferences = g_action_map_lookup_action(G_ACTION_MAP(app), "preferences");
	statusicon_plugin->contacts = g_action_map_lookup_action(G_ACTION_MAP(app), "addressbook");
	statusicon_plugin->quit = g_action_map_lookup_action(G_ACTION_MAP(app), "quit");
	statusicon_plugin->phone = g_action_map_lookup_action(G_ACTION_MAP(app), "phone");
	statusicon_plugin->about = g_action_map_lookup_action(G_ACTION_MAP(app), "about");
	statusicon_plugin->plugins = g_action_map_lookup_action(G_ACTION_MAP(app), "plugins");
	statusicon_plugin->copy_ip = g_action_map_lookup_action(G_ACTION_MAP(app), "copy_ip");
	statusicon_plugin->reconnect = g_action_map_lookup_action(G_ACTION_MAP(app), "reconnect");
	statusicon_plugin->journal = g_action_map_lookup_action(G_ACTION_MAP(app), "journal");
	statusicon_plugin->hideonquit = g_action_map_lookup_action(G_ACTION_MAP(app), "hideonquit");
	statusicon_plugin->hideonstart = g_action_map_lookup_action(G_ACTION_MAP(app), "hideonstart");

	statusicon_set_hide_on_quit(statusicon_plugin, TRUE);

	statusicon_plugin->settings = rm_settings_new("org.tabos.roger.plugins.statusicon");

	/* Connect to "connection-changed" signal */
	plugin->priv = statusicon_plugin;
	statusicon_plugin->signal_id = g_signal_connect(G_OBJECT(rm_object), "connection-changed", G_CALLBACK(statusicon_connection_changed_cb), statusicon_plugin);

	if (g_settings_get_boolean(statusicon_plugin->settings, "hide-journal-on-startup")) {
		statusicon_set_hide_on_start(statusicon_plugin, TRUE);
	}

	g_idle_add(add_statusicon, statusicon_plugin);

	return TRUE;
}

/**
 * statusicon_plugin_shutdown:
 * @plugin: a #RmPlugin
 *
 * Shutdown statusicon plugin
 *
 * Returns: %TRUE
 */
gboolean statusicon_plugin_shutdown(RmPlugin *plugin)
{
	RmStatusIconPlugin *statusicon_plugin = plugin->priv;

	/* Unregister delete handler */
	statusicon_set_hide_on_quit(statusicon_plugin, FALSE);

	/* If signal handler is connected: disconnect */
	if (g_signal_handler_is_connected(G_OBJECT(rm_object), statusicon_plugin->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(rm_object), statusicon_plugin->signal_id);
	}

	gtk_status_icon_set_visible(statusicon_plugin->statusicon, FALSE);
	g_clear_object(&statusicon_plugin->statusicon);

	statusicon_plugin->statusicon = NULL;
	plugin->priv = NULL;

	return TRUE;
}

/**
 * statusicon_plugin_configure:
 * @plugin: a #RmPlugin
 *
 * Configure plugin
 *
 * Returns: Configuration widget
 */
gpointer statusicon_plugin_configure(RmPlugin *plugin)
{
	GtkWidget *settings_grid;
	GtkWidget *label;
	GtkWidget *switch_journal;
	GtkWidget *combo_box_default;
	GtkWidget *combo_box_notify;
	GtkWidget *group;
	RmStatusIconPlugin *statusicon_plugin = plugin->priv;

	/* Settings grid */
	settings_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(settings_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(settings_grid), 15);

	/* Create label and switch for "Hide Journal on startup" and add to grid */
	label = ui_label_new(_("Hide Journal on startup"));
	gtk_grid_attach(GTK_GRID(settings_grid), label, 0, 0, 1, 1);

	switch_journal = gtk_switch_new();
	gtk_switch_set_active(GTK_SWITCH(switch_journal), g_settings_get_boolean(statusicon_plugin->settings, "hide-journal-on-startup"));
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
	g_settings_bind(statusicon_plugin->settings, "hide-journal-on-startup", switch_journal, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(statusicon_plugin->settings, "default-icon", combo_box_default, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(statusicon_plugin->settings, "notify-icon", combo_box_notify, "active-id", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(combo_box_default, "changed", G_CALLBACK(statusicon_combobox_default_changed_cb), statusicon_plugin);
	g_signal_connect(combo_box_notify, "changed", G_CALLBACK(statusicon_combobox_notify_changed_cb), statusicon_plugin);

	group = ui_group_create(settings_grid, _("Settings for Status icon GTK"), TRUE, TRUE);

	return group;
}

G_GNUC_END_IGNORE_DEPRECATIONS

RM_PLUGIN_CONFIG(statusicon)
