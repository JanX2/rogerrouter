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

#include <libroutermanager/call.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/routermanager.h>

#include <roger/about.h>
#include <roger/application.h>
#include <roger/journal.h>
#include <roger/main.h>
#include <roger/phone.h>
#include <roger/pref.h>

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
GtkWidget* statusicon_menu_last_calls(void)
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
	gchar *path = g_strconcat(get_directory(APP_DATA), G_DIR_SEPARATOR_S, "app.png", NULL);
	gtk_status_icon_set_from_pixbuf(statusicon, gdk_pixbuf_new_from_file(path, NULL));
	g_free(path);

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
		gtk_status_icon_set_from_pixbuf(statusicon, journal_get_call_icon(CALL_TYPE_MISSED));
	}
}

void impl_activate(PeasActivatable *plugin)
{
	RouterManagerStatusIconPlugin *statusicon_plugin;
	gchar *path = g_strconcat(get_directory(APP_DATA), G_DIR_SEPARATOR_S, "app.png", NULL);

	journal_set_hide_on_quit(TRUE);

	statusicon_settings = g_settings_new("org.tabos.roger.plugins.statusicon");

	/* Create StatusIcon GTK */
	statusicon = gtk_status_icon_new();

	g_signal_connect(G_OBJECT(statusicon), "popup-menu", G_CALLBACK(statusicon_popup_menu_cb), NULL);
	g_signal_connect(G_OBJECT(statusicon), "activate", G_CALLBACK(statusicon_activate_cb), NULL);

	gtk_status_icon_set_from_pixbuf(statusicon, gdk_pixbuf_new_from_file(path, NULL));
	g_free(path);
	gtk_status_icon_set_tooltip_text(statusicon, _("Roger Router"));
	gtk_status_icon_set_visible(statusicon, TRUE);

	/* Connect to "call-notify" signal */
	statusicon_plugin = ROUTERMANAGER_STATUSICON_PLUGIN(plugin);
	statusicon_plugin->priv->signal_id = g_signal_connect(G_OBJECT(app_object), "connection-notify", G_CALLBACK(statusicon_connection_notify_cb), NULL);

	if (g_settings_get_boolean(statusicon_settings, "hide-journal-on-startup")) {
		journal_set_hide_on_start(TRUE);
	}
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
	GtkWidget *toggle_button;
	GtkWidget *group;

	toggle_button = gtk_check_button_new_with_label(_("Hide Journal on startup"));
	g_settings_bind(statusicon_settings, "hide-journal-on-startup", toggle_button, "active", G_SETTINGS_BIND_DEFAULT);

	group = pref_group_create(toggle_button, _("Startup"), TRUE, FALSE);

	return group;
}
