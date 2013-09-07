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
#include <libroutermanager/routermanager.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/profile.h>

#include <roger/journal.h>
#include <roger/about.h>
#include <roger/application.h>
#include <roger/phone.h>
#include <roger/main.h>
#include <roger/pref.h>
#include <roger/uitools.h>

#define ROUTERMANAGER_TYPE_STATUSICON_PLUGIN        (routermanager_statusicon_plugin_get_type ())
#define ROUTERMANAGER_STATUSICON_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_STATUSICON_PLUGIN, RouterManagerStatusIconPlugin))

typedef struct {
        guint signal_id;
} RouterManagerStatusIconPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER_CONFIGURABLE(ROUTERMANAGER_TYPE_STATUSICON_PLUGIN, RouterManagerStatusIconPlugin, routermanager_statusicon_plugin)

#define MAX_LASTCALLS 5

static GSettings *statusicon_settings = NULL;
static GtkStatusIcon *statusicon = NULL;

extern GtkWidget *journal_win;

/**
 * \brief Create separator item with 'text'
 * \param text separator text
 * \return new menuitem widget
 */
GtkWidget *create_separator_item(gchar *text) {
	GtkWidget *label = NULL;
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *item = NULL;

	item = gtk_menu_item_new();

	label = gtk_label_new(text);

	gtk_box_pack_start(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), TRUE, TRUE, 0);

	g_object_set(G_OBJECT(item), "child", box, "sensitive", FALSE, NULL);

	return item;
}

/**
 * \brief Create call menu item for type type
 * \param pnNumber call number
 * \param type call type
 * \return new menuitem widget
 */
GtkWidget *create_call_item(struct call *call) {
	GtkWidget *item = NULL;
	GtkWidget *label = NULL;
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *image = NULL;
	GdkPixbuf *pix = NULL;
	gchar *number_text;
	GtkWidget *name_label = NULL;
	gchar *name;

	item = gtk_menu_item_new();

	label = gtk_label_new("");
	number_text = g_markup_printf_escaped("<small>%s</small>", call->remote.number);
	gtk_label_set_markup(GTK_LABEL(label), number_text);
	g_free(number_text);

	name_label = gtk_label_new("");
	name = g_markup_printf_escaped("<b><small>%s</small></b>", strlen(call->remote.name) ? call->remote.name : _("unknown"));
	gtk_label_set_markup(GTK_LABEL(name_label), name);
	g_free(name);

	gtk_box_pack_start(GTK_BOX(box), name_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(name_label), 0.0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	g_object_set(G_OBJECT(item), "child", box, NULL);

	return item;
}


void statusicon_gtk_activated(void)
{
	gchar *path = g_strconcat(get_directory(APP_DATA), G_DIR_SEPARATOR_S, "app.png", NULL);
	gtk_status_icon_set_visible(statusicon, FALSE);
	gtk_status_icon_set_from_pixbuf(statusicon, gdk_pixbuf_new_from_file(path, NULL));
	gtk_status_icon_set_visible(statusicon, TRUE);
	g_free(path);

	gtk_widget_set_visible(GTK_WIDGET(journal_win), !gtk_widget_get_visible(GTK_WIDGET(journal_win)));
}

void statusicon_journal_cb(GtkMenuItem *item, gpointer user_data)
{
	if (gtk_widget_get_visible(GTK_WIDGET(journal_win))) {
		gtk_window_present(GTK_WINDOW(journal_win));
	} else {
		statusicon_gtk_activated();
	}
}

void statusicon_dial_number(GtkMenuItem *item, gpointer user_data)
{
	app_show_phone_window(user_data);
}

extern GList *journal_list;

void statusicon_add_lastcalls(GtkWidget *menu)
{
	GList *list = NULL;
	GtkWidget *item = NULL;
	GtkWidget *submenu = NULL;
	int count = 0;

	item = gtk_menu_item_new_with_label(_("Last calls"));
	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	list = journal_list;
	if (g_list_length(list) <= 0) {
		gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);
		return;
	}

	item = create_separator_item(_("Incoming"));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	list = journal_list;
	count = 0;
	while (list != NULL) {
		struct call *call = list->data;

		if (call->type == CALL_TYPE_INCOMING) {
			item = create_call_item(call);
			gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_dial_number), &call->remote);
			//gtk_widget_set_sensitive(GTK_WIDGET(item), routerHasDial(getActiveProfile()));
			count++;

			if (count == MAX_LASTCALLS) {
				break;
			}
		}

		list = list->next;
	}

	item = create_separator_item(_("Outgoing"));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	list = journal_list;
	count = 0;
	while (list != NULL) {
		struct call *call = list->data;

		if (call->type == CALL_TYPE_OUTGOING) {
			item = create_call_item(call);
			gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_dial_number), &call->remote);
			//gtk_widget_set_sensitive(GTK_WIDGET(item), routerHasDial(getActiveProfile()));
			count++;

			if (count == MAX_LASTCALLS) {
				break;
			}
		}

		list = list->next;
	}

	item = create_separator_item(_("Missed"));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	list = journal_list;
	count = 0;
	while (list != NULL) {
		struct call *call = list->data;

		if (call->type == CALL_TYPE_MISSED) {
			item = create_call_item(call);

			gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statusicon_dial_number), &call->remote);
			//gtk_widget_set_sensitive(GTK_WIDGET(item), routerHasDial(getActiveProfile()));
			count++;

			if (count == MAX_LASTCALLS) {
				break;
			}
		}

		list = list->next;
	}
}

void statusicon_add_functions(GtkWidget *menu)
{
	GtkWidget *item = NULL;
	GtkWidget *submenu = NULL;

	item = gtk_menu_item_new_with_label(_("Functions"));
	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_menu_item_new_with_label(_("Copy IP address"));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(app_copy_ip), NULL);

	item = gtk_menu_item_new_with_label(_("Reconnect"));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(app_reconnect), NULL);
}

static void statusicon_gtk_popup(GtkStatusIcon *statusicon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu;

	menu = gtk_menu_new();

	/* Journal */
	GtkWidget *item = gtk_menu_item_new_with_label(_("Journal"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(statusicon_journal_cb), NULL);

	/* Contacts */
	/*item = gtk_image_menu_item_new_with_label(_("Contacts"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_show_contacts), NULL);*/

	/* Phone */
	item = gtk_menu_item_new_with_label(_("Phone"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_show_phone_window), NULL);

	/* Preferences */
	item = gtk_menu_item_new_with_label(_("Preferences"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(app_show_preferences), NULL);

	statusicon_add_lastcalls(menu);

	statusicon_add_functions(menu);

	/* Line */
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

	/* Line */
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
		gtk_status_icon_set_visible(statusicon, FALSE);
		gtk_status_icon_set_from_pixbuf(statusicon, journal_get_call_icon(CALL_TYPE_MISSED));
		gtk_status_icon_set_visible(statusicon, TRUE);
	}
}

void impl_activate(PeasActivatable *plugin)
{
	RouterManagerStatusIconPlugin *statusicon_plugin = ROUTERMANAGER_STATUSICON_PLUGIN(plugin);
	gchar *path = g_strconcat(get_directory(APP_DATA), G_DIR_SEPARATOR_S, "app.png", NULL);

	journal_set_hide_on_quit(TRUE);

	statusicon_settings = g_settings_new("org.tabos.roger.plugins.statusicon");

	statusicon = gtk_status_icon_new();

	g_signal_connect(G_OBJECT(statusicon), "popup-menu", G_CALLBACK(statusicon_gtk_popup), NULL);
	g_signal_connect(G_OBJECT(statusicon), "activate", G_CALLBACK(statusicon_gtk_activated), NULL);

	gtk_status_icon_set_from_pixbuf(statusicon, gdk_pixbuf_new_from_file(path, NULL));
	g_free(path);

	gtk_status_icon_set_tooltip_text(statusicon, _("Roger Router"));
	gtk_status_icon_set_visible(statusicon, TRUE);

	/* Connect to "call-notify" signal */
	statusicon_plugin->priv->signal_id = g_signal_connect(G_OBJECT(app_object), "connection-notify", G_CALLBACK(statusicon_connection_notify_cb), NULL);

	if (g_settings_get_boolean(statusicon_settings, "hide-journal-on-startup")) {
		journal_set_hide_on_start(TRUE);
	}
}

void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerStatusIconPlugin *statusicon_plugin = ROUTERMANAGER_STATUSICON_PLUGIN(plugin);

	/* Unregister delete handler */
	journal_set_hide_on_quit(FALSE);

	/* If signal handler is connected: disconnect */
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

GtkWidget *statusicon_configure(void)
{
	GtkWidget *toggle_button;
	GtkWidget *group;

	toggle_button = gtk_check_button_new_with_label(_("Hide Journal on startup"));
	g_settings_bind(statusicon_settings, "hide-journal-on-startup", toggle_button, "active", G_SETTINGS_BIND_DEFAULT);

	group = pref_group_create(toggle_button, _("Startup"), TRUE, FALSE);

	return group;
}

GtkWidget *impl_create_configure_widget(PeasGtkConfigurable *config)
{
        return statusicon_configure();
}
