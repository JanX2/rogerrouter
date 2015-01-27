/**
 * Roger Router - Plugin Application Indicator
 * Copyright (c) 2013-2014 Dieter Schärf
 *
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

#include <libappindicator/app-indicator.h>

#include <libroutermanager/call.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/settings.h>

#include <roger/about.h>
#include <roger/application.h>
#include <roger/journal.h>
#include <roger/main.h>
#include <roger/phone.h>
#include <roger/pref.h>
#include <roger/uitools.h>

#define MAX_LASTCALLS 5

#define ROUTERMANAGER_TYPE_INDICATOR_PLUGIN        (routermanager_indicator_plugin_get_type ())
#define ROUTERMANAGER_INDICATOR_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_INDICATOR_PLUGIN, RouterManagerIndicatorPlugin))

typedef struct {
	guint signal_id;
} RouterManagerIndicatorPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER_CONFIGURABLE(ROUTERMANAGER_TYPE_INDICATOR_PLUGIN, RouterManagerIndicatorPlugin, routermanager_indicator_plugin)

extern GList *journal_list;
extern GtkWidget *journal_win;

static GSettings *indicator_settings = NULL;
static AppIndicator *indicator = NULL;

/**
 * \brief create "Functions" submenu items
 * \param none
 * \return new menu widget
 */
GtkWidget *indicator_menu_functions(void)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	/* Functions - Copy IP adress */
	item = gtk_menu_item_new_with_label(_("Copy IP address"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_copy_ip), NULL);

	/* Functions - Reconnect */
	item = gtk_menu_item_new_with_label(_("Reconnect"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_reconnect), NULL);

	return menu;
}

/**
 * \brief "dial_number" callback function
 * \param obj gtk menu item
 * \param user_data unused user pointer
 */
void indicator_dial_number_cb(GtkMenuItem *item, gpointer user_data)
{
	app_show_phone_window(user_data);
}

/**
 * \brief create last call entrys and add to submenu "Last calls"
 * \param obj gtk menu
 * \param gchar label
 * \param int call_type
 */
void indicator_menu_last_calls_group(GtkWidget *menu, gchar *label, int call_type)
{
	GtkWidget *item;
	GList *list;
	int count = 0;

	item = gtk_menu_item_new_with_label(label);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);

	list = journal_list;

	while (list != NULL) {
		struct call *call = list->data;

		if (call->type == call_type && (!EMPTY_STRING(call->remote->name) || !EMPTY_STRING(call->remote->number))) {
			item = gtk_menu_item_new();

			if (!EMPTY_STRING(call->remote->name)) {
				gtk_menu_item_set_label(GTK_MENU_ITEM(item), call->remote->name);
			} else {
				gtk_menu_item_set_label(GTK_MENU_ITEM(item), call->remote->number);
			}
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_dial_number_cb), call->remote);

			count++;

			if (count == MAX_LASTCALLS) {
				break;
			}
		}
		list = list->next;
	}
}

/**
 * \brief create "Last calls" submenu items
 * \param pnNumber call number
 * \param type call type
 * \return new menu widget
 */
GtkWidget *indicator_menu_last_calls(void)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	/* Last calls - Incomming */
	indicator_menu_last_calls_group(menu, _("Incoming"), CALL_TYPE_INCOMING);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Last calls - Outgoing */
	indicator_menu_last_calls_group(menu, _("Outgoing"), CALL_TYPE_OUTGOING);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Last calls - Missed */
	indicator_menu_last_calls_group(menu, _("Missed"), CALL_TYPE_MISSED);

	return menu;
}

/**
 * \brief "last_calls" callback function
 * \param obj gtk menu item
 */
void indicator_last_calls_cb(GtkMenuItem *item)
{
	GtkWidget *submenu;

	submenu = indicator_menu_last_calls();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	gtk_widget_show_all(submenu);
}

/**
 * \brief "journal" callback function
 * \param none
 */
void indicator_journal_cb(void)
{
	app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);

	if (gtk_widget_get_visible(GTK_WIDGET(journal_win))) {
		gtk_window_present(GTK_WINDOW(journal_win));
	} else {
		gtk_widget_set_visible(GTK_WIDGET(journal_win), TRUE);
	}
}

/**
 * \brief create menu
 * \param none
 * \return new menu widget
 */
GtkWidget *indicator_menu(void)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *submenu;

	menu = gtk_menu_new();

	/* Journal */
	item = gtk_menu_item_new_with_label(_("Journal"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(indicator_journal_cb), NULL);

	/* Contacts */
	item = gtk_menu_item_new_with_label(_("Contacts"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_show_contacts), NULL);

	/* Phone */
	item = gtk_menu_item_new_with_label(_("Phone"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_show_phone_window), NULL);

	/* Last calls */
	item = gtk_menu_item_new_with_label(_("Last calls"));
	submenu = indicator_menu_last_calls();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(indicator_last_calls_cb), NULL);

	/* Functions */
	item = gtk_menu_item_new_with_label(_("Functions"));
	submenu = indicator_menu_functions();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Preferences */
	item = gtk_menu_item_new_with_label(_("Preferences"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_show_preferences), NULL);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Help */
	item = gtk_menu_item_new_with_label(_("Help"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_show_help), NULL);

	/* About */
	item = gtk_menu_item_new_with_label(_("About"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_show_about), NULL);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Quit */
	item = gtk_menu_item_new_with_label(_("Quit"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_quit), NULL);

	gtk_widget_show_all(menu);

	return menu;
}

/**
 * \brief "connection-notify" callback function
 * \param obj app object
 * \param connection connection structure
 * \param unused_pointer unused user pointer
 */
void indicator_connection_notify_cb(AppObject *obj, struct connection *connection, gpointer unused_pointer)
{
	g_debug("Called: '%d/%d", connection->type, CONNECTION_TYPE_MISS);
	if (connection->type == CONNECTION_TYPE_MISS) {
		g_debug("Setting missed icon");
		app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ATTENTION);
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
	GtkWidget *combo_box = user_data;

	g_settings_set_string(indicator_settings, "default-icon", gtk_combo_box_get_active_id(GTK_COMBO_BOX(combo_box)));

	/* Update indicator icon */
	gchar *iconname = g_strconcat("roger-", g_settings_get_string(indicator_settings, "default-icon"), NULL);
	app_indicator_set_icon_full(indicator, iconname, "default-icon");
	g_free(iconname);
}

/**
 * \brief "connection-notify-icon" callback function
 * \param widget gtk combo box
 * \param unused_pointer unused user pointer
 */
void indicator_combobox_notify_changed_cb(GtkComboBox *widget, gpointer user_data)
{
	/* GSettings has not written the changed value to its container, so we explicit set it here */
	GtkWidget *combo_box = user_data;

	g_settings_set_string(indicator_settings, "notify-icon", gtk_combo_box_get_active_id(GTK_COMBO_BOX(combo_box)));

	/* Update indicator attention icon */
	gchar *iconname = g_strconcat("roger-", g_settings_get_string(indicator_settings, "notify-icon"), NULL);
	app_indicator_set_attention_icon_full(indicator, iconname, "notify-icon");
	g_free(iconname);
}

void impl_activate(PeasActivatable *plugin)
{
	RouterManagerIndicatorPlugin *indicator_plugin;
	GtkWidget *menu;

	journal_set_hide_on_quit(TRUE);

	indicator_settings = rm_settings_plugin_new("org.tabos.roger.plugins.indicator", "indicator");

	/* Create Application Indicator */
	gchar *iconname = g_strconcat("roger-", g_settings_get_string(indicator_settings, "default-icon"), NULL);
	indicator = app_indicator_new("roger", iconname, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
	iconname = g_strconcat("roger-", g_settings_get_string(indicator_settings, "notify-icon"), NULL);
	app_indicator_set_attention_icon_full(indicator, iconname, "notify-icon");
	g_free(iconname);

	menu = indicator_menu();
	app_indicator_set_menu(indicator, GTK_MENU(menu));

	app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);

	/* Connect to "call-notify" signal */
	indicator_plugin = ROUTERMANAGER_INDICATOR_PLUGIN(plugin);
	indicator_plugin->priv->signal_id = g_signal_connect(G_OBJECT(app_object), "connection-notify", G_CALLBACK(indicator_connection_notify_cb), NULL);

	if (g_settings_get_boolean(indicator_settings, "hide-journal-on-startup")) {
		journal_set_hide_on_start(TRUE);
	}
}

void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerIndicatorPlugin *indicator_plugin;

	/* Unregister delete handler */
	journal_set_hide_on_quit(FALSE);

	/* If signal handler is connected: disconnect */
	indicator_plugin = ROUTERMANAGER_INDICATOR_PLUGIN(plugin);
	if (g_signal_handler_is_connected(G_OBJECT(app_object), indicator_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), indicator_plugin->priv->signal_id);
	}

	if (journal_win) {
		/* Make sure journal window is visible on deactivate */
		gtk_widget_show(GTK_WIDGET(journal_win));
	}

	app_indicator_set_status(indicator, APP_INDICATOR_STATUS_PASSIVE);
	g_object_unref(indicator);

	indicator = NULL;
}

GtkWidget *impl_create_configure_widget(PeasGtkConfigurable *config)
{
	GtkWidget *settings_grid;
	GtkWidget *label;
	GtkWidget *switch_journal;
	GtkWidget *combo_box_default;
	GtkWidget *combo_box_notify;
	GtkWidget *group;

	/* Settings grid */
	settings_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(settings_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(settings_grid), 15);

	/* Create label and switch for "Hide Journal on startup" and add to grid */
	label = ui_label_new(_("Hide Journal on startup"));
	gtk_grid_attach(GTK_GRID(settings_grid), label, 0, 0, 1, 1);

	switch_journal = gtk_switch_new();
	gtk_switch_set_active(GTK_SWITCH(switch_journal), g_settings_get_boolean(indicator_settings, "hide-journal-on-startup"));
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
	g_settings_bind(indicator_settings, "hide-journal-on-startup", switch_journal, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(indicator_settings, "default-icon", combo_box_default, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(indicator_settings, "notify-icon", combo_box_notify, "active-id", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(combo_box_default, "changed", G_CALLBACK(indicator_combobox_default_changed_cb), combo_box_default);
	g_signal_connect(combo_box_notify, "changed", G_CALLBACK(indicator_combobox_notify_changed_cb), combo_box_notify);

	group = pref_group_create(settings_grid, _("Settings for Application Indicator"), TRUE, TRUE);

	return group;
}
