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

#include <libroutermanager/call.h>
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

#define ROUTERMANAGER_TYPE_STATUSICON_PLUGIN        (routermanager_statusicon_plugin_get_type ())
#define ROUTERMANAGER_STATUSICON_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_STATUSICON_PLUGIN, RouterManagerStatusIconPlugin))

typedef struct {
	guint signal_id;
} RouterManagerStatusIconPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER_CONFIGURABLE(ROUTERMANAGER_TYPE_STATUSICON_PLUGIN, RouterManagerStatusIconPlugin, routermanager_statusicon_plugin)

extern GList *journal_list;
extern GtkWidget *journal_win;

static GSettings *statusicon_settings = NULL;
static GtkStatusIcon *statusicon = NULL;

/**
 * \brief create "Functions" submenu items
 * \param none
 * \return new menu widget
 */
GtkWidget *statusicon_menu_functions(void)
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
void statusicon_dial_number_cb(GtkMenuItem *item, gpointer user_data)
{
	app_show_phone_window(user_data);
}

/**
 * \brief create last call entrys and add to submenu "Last calls"
 * \param obj gtk menu
 * \param gchar label
 * \param int call_type
 */
void statusicon_menu_last_calls_group(GtkWidget *menu, gchar *label, int call_type)
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

		if (call->type == call_type && (strlen(call->remote->name) || strlen(call->remote->number))) {
			item = gtk_menu_item_new();

			if (strlen(call->remote->name)) {
				gtk_menu_item_set_label(GTK_MENU_ITEM(item), call->remote->name);
			} else {
				gtk_menu_item_set_label(GTK_MENU_ITEM(item), call->remote->number);
			}
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_dial_number_cb), call->remote);

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
GtkWidget *statusicon_menu_last_calls(void)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	/* Last calls - Incomming */
	statusicon_menu_last_calls_group(menu, _("Incoming"), CALL_TYPE_INCOMING);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Last calls - Outgoing */
	statusicon_menu_last_calls_group(menu, _("Outgoing"), CALL_TYPE_OUTGOING);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Last calls - Missed */
	statusicon_menu_last_calls_group(menu, _("Missed"), CALL_TYPE_MISSED);

	return menu;
}

void statusicon_activate_cb(void)
{
	gchar *iconname = g_strconcat("roger-", g_settings_get_string(statusicon_settings, "default-icon"), NULL);
	gtk_status_icon_set_from_icon_name(statusicon, iconname);
	g_free(iconname);

	gtk_widget_set_visible(GTK_WIDGET(journal_win), !gtk_widget_get_visible(GTK_WIDGET(journal_win)));
}

void statusicon_journal_cb(void)
{
	if (gtk_widget_get_visible(GTK_WIDGET(journal_win))) {
		gtk_window_present(GTK_WINDOW(journal_win));
	} else {
		statusicon_activate_cb();
	}
}

void statusicon_popup_menu_cb(GtkStatusIcon *statusicon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *submenu;

	menu = gtk_menu_new();

	/* Journal */
	item = gtk_menu_item_new_with_label(_("Journal"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(statusicon_journal_cb), NULL);

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
	submenu = statusicon_menu_last_calls();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Functions */
	item = gtk_menu_item_new_with_label(_("Functions"));
	submenu = statusicon_menu_functions();
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

#ifdef G_OS_WIN32
	button = 0;
#endif

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, statusicon, button, activate_time);
}

/** * \brief "connection-notify" callback function
 * \param obj app object
 * \param connection connection structure
 * \param unused_pointer unused user pointer
 */
void statusicon_connection_notify_cb(AppObject *obj, struct connection *connection, gpointer unused_pointer)
{
	g_debug("Called: '%d/%d", connection->type, CONNECTION_TYPE_MISS);
	if (connection->type == CONNECTION_TYPE_MISS) {
		g_debug("Setting missed icon");
		gchar *iconname = g_strconcat("roger-", g_settings_get_string(statusicon_settings, "notify-icon"), NULL);
		gtk_status_icon_set_from_icon_name(statusicon, iconname);
		g_free(iconname);
	}
}

/**
 * \brief "connection-default-icon" callback function
 * \param widget gtk combo box
 * \param unused_pointer unused user pointer
 */
void statusicon_combobox_default_changed_cb(GtkComboBox *widget, gpointer user_data)
{
	/* GSettings has not written the changed value to its container, so we explicit set it here */
	GtkWidget *combo_box = user_data;

	g_settings_set_string(statusicon_settings, "default-icon", gtk_combo_box_get_active_id(GTK_COMBO_BOX(combo_box)));

	/* Update statusicon icon */
	gchar *iconname = g_strconcat("roger-", g_settings_get_string(statusicon_settings, "default-icon"), NULL);
	gtk_status_icon_set_from_icon_name(statusicon, iconname);
	g_free(iconname);
}

/**
 * \brief "connection-notify-icon" callback function
 * \param widget gtk combo box
 * \param unused_pointer unused user pointer
 */
void statusicon_combobox_notify_changed_cb(GtkComboBox *widget, gpointer user_data)
{
	/* GSettings has not written the changed value to its container, so we explicit set it here */
	GtkWidget *combo_box = user_data;

	g_settings_set_string(statusicon_settings, "notify-icon", gtk_combo_box_get_active_id(GTK_COMBO_BOX(combo_box)));
}

static gboolean add_statusicon(gpointer user_data)
{
	/* Create StatusIcon GTK */
	statusicon = gtk_status_icon_new();

	g_signal_connect(G_OBJECT(statusicon), "popup-menu", G_CALLBACK(statusicon_popup_menu_cb), NULL);
	g_signal_connect(G_OBJECT(statusicon), "activate", G_CALLBACK(statusicon_activate_cb), NULL);

	gchar *iconname = g_strconcat("roger-", g_settings_get_string(statusicon_settings, "default-icon"), NULL);
	gtk_status_icon_set_from_icon_name(statusicon, iconname);
	g_free(iconname);

	gtk_status_icon_set_tooltip_text(statusicon, _("Roger Router"));
	gtk_status_icon_set_visible(statusicon, TRUE);

	return FALSE;
}

void impl_activate(PeasActivatable *plugin)
{
	RouterManagerStatusIconPlugin *statusicon_plugin;

	journal_set_hide_on_quit(TRUE);

	statusicon_settings = rm_settings_plugin_new("org.tabos.roger.plugins.statusicon", "statusicon");

	/* Connect to "call-notify" signal */
	statusicon_plugin = ROUTERMANAGER_STATUSICON_PLUGIN(plugin);
	statusicon_plugin->priv->signal_id = g_signal_connect(G_OBJECT(app_object), "connection-notify", G_CALLBACK(statusicon_connection_notify_cb), NULL);

	if (g_settings_get_boolean(statusicon_settings, "hide-journal-on-startup")) {
		journal_set_hide_on_start(TRUE);
	}

	g_idle_add(add_statusicon, NULL);
}

void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerStatusIconPlugin *statusicon_plugin;

	/* Unregister delete handler */
	journal_set_hide_on_quit(FALSE);

	/* If signal handler is connected: disconnect */
	statusicon_plugin = ROUTERMANAGER_STATUSICON_PLUGIN(plugin);
	if (g_signal_handler_is_connected(G_OBJECT(app_object), statusicon_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), statusicon_plugin->priv->signal_id);
	}

	if (journal_win) {
		/* Make sure journal window is visible on deactivate */
		gtk_widget_show(GTK_WIDGET(journal_win));
	}

//#if !GTK_CHECK_VERSION(3, 7, 0)
	/* This is currently broken in GTK 3.8.0 - for now the application must be restart to remove the status icon..... */
	gtk_status_icon_set_visible(statusicon, FALSE);
	g_object_unref(statusicon);
//#endif

	statusicon = NULL;
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
	gtk_switch_set_active(GTK_SWITCH(switch_journal), g_settings_get_boolean(statusicon_settings, "hide-journal-on-startup"));
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
	g_settings_bind(statusicon_settings, "hide-journal-on-startup", switch_journal, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(statusicon_settings, "default-icon", combo_box_default, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(statusicon_settings, "notify-icon", combo_box_notify, "active-id", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(combo_box_default, "changed", G_CALLBACK(statusicon_combobox_default_changed_cb), combo_box_default);
	g_signal_connect(combo_box_notify, "changed", G_CALLBACK(statusicon_combobox_notify_changed_cb), combo_box_notify);

	group = pref_group_create(settings_grid, _("Settings for Status icon GTK"), TRUE, TRUE);

	return group;
}
