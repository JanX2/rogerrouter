/**
 * Roger Router - Plugin Application Indicator
 * Copyright (c) 2013 Dieter Schärf
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
#include <libroutermanager/plugins.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/gstring.h>

#include <roger/about.h>
#include <roger/application.h>
#include <roger/journal.h>
#include <roger/main.h>
#include <roger/phone.h>
#include <roger/pref.h>

#define MAX_LASTCALLS 5

#define ROUTERMANAGER_TYPE_INDICATOR_PLUGIN        (routermanager_indicator_plugin_get_type ())
#define ROUTERMANAGER_INDICATOR_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_INDICATOR_PLUGIN, RouterManagerIndicatorPlugin))

typedef struct {
	guint signal_id;
} RouterManagerIndicatorPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER_CONFIGURABLE(ROUTERMANAGER_TYPE_INDICATOR_PLUGIN, RouterManagerIndicatorPlugin, routermanager_indicator_plugin)

extern GList *journal_list;
extern GtkWidget *journal_win;

static AppIndicator *indicator;
static GSettings *indicator_settings;

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

void impl_activate(PeasActivatable *plugin)
{
	RouterManagerIndicatorPlugin *indicator_plugin;
	GtkWidget *menu;
	gchar *icon;

	journal_set_hide_on_quit(TRUE);

	indicator_settings = g_settings_new("org.tabos.roger.plugins.indicator");

	/* Create Application Indicator */
	icon = g_strconcat(get_directory(APP_DATA), G_DIR_SEPARATOR_S, "callout.png", NULL);
	indicator = app_indicator_new("roger", icon, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
	g_free(icon);

	menu = indicator_menu();
	app_indicator_set_menu(indicator, GTK_MENU(menu));

	icon = g_strconcat(get_directory(APP_DATA), G_DIR_SEPARATOR_S, "callmissed.png", NULL);
	app_indicator_set_attention_icon(indicator, icon);
	g_free(icon);

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
	GtkWidget *toggle_button;
	GtkWidget *group;

	toggle_button = gtk_check_button_new_with_label(_("Hide Journal on startup"));
	g_settings_bind(indicator_settings, "hide-journal-on-startup", toggle_button, "active", G_SETTINGS_BIND_DEFAULT);

	group = pref_group_create(toggle_button, _("Startup"), TRUE, FALSE);

	return group;
}
