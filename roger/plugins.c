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

#include <gtk/gtk.h>

#include <rm/rm.h>

#include <main.h>
#include <plugins.h>
#include <uitools.h>
#include <journal.h>

static GtkWidget *plugins_window = NULL;
static GtkWidget *config_button = NULL;
static GtkWidget *help_button = NULL;
extern GApplication *journal_application;

/**
 * plugins_switch_state_set_cb:
 * @widget: a #GtkSwitch
 * @state: state of switch (on/off)
 * @user_data: a #RmPlugin
 *
 * Depending on the state de/activate plugin and set config button sensitive
 *
 * Returns: %FALSE
 */
static gboolean plugins_switch_state_set_cb(GtkSwitch *widget, gboolean state, gpointer user_data)
{
	RmPlugin *plugin = user_data;

	if (state) {
		/* Enable plugin */
		rm_plugins_enable(plugin);
		gtk_widget_set_sensitive(config_button, TRUE);
	} else {
		/* Disable plugin */
		gtk_widget_set_sensitive(config_button, FALSE);
		rm_plugins_disable(plugin);
	}

	return FALSE;
}

/**
 * plugins_url_button_clicked_cb:
 * @widget: a #GtkWidget (unused)
 * @user_data: URL to open
 *
 * Opens URL of selected plugin.
 */
static void plugins_url_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	GError *error = NULL;
	gchar *uri = user_data;

	//rm_os_execute(user_data);
	g_debug("%s(): clicked", __FUNCTION__);
	if (!gtk_show_uri_on_window(GTK_WINDOW(plugins_window), uri, gtk_get_current_event_time (), &error)) {
		g_debug("%s(): Could not open uri '%s'", __FUNCTION__, uri);
		g_debug("%s(): '%s'", __FUNCTION__, error->message);
	} else {
		g_debug("%s(): Opened '%s'", __FUNCTION__, uri);
	}
}

/**
 * plugins_about_button_clicked_cb:
 * @button: button #GtkWidget
 * @user_data: a #GtkListBox
 *
 * Create and show about dialog of selected plugin.
 */
static void plugins_about_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(user_data));
	GtkWidget *child = gtk_container_get_children(GTK_CONTAINER(row))->data;
	RmPlugin *plugin = g_object_get_data(G_OBJECT(child), "plugin");
	GtkWidget *about;
	GtkWidget *header_bar;
	GtkWidget *vbox;
	GtkWidget *name_label;
	GtkWidget *description_label;
	GtkWidget *copyright_label;
	GtkWidget *hbox;
	GtkWidget *help_button;
	GtkWidget *homepage_button;
	GtkWidget *dummy;
	gchar *name;

	if (!plugin) {
		return;
	}

	about = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, NULL);
	gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(plugins_window));
	gtk_window_set_resizable(GTK_WINDOW(about), FALSE);
	gtk_window_set_modal(GTK_WINDOW(about), TRUE);
	gtk_window_set_position(GTK_WINDOW(about), GTK_WIN_POS_CENTER_ON_PARENT);

	header_bar = gtk_dialog_get_header_bar(GTK_DIALOG(about));
	gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), _("About this plugin"));
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_hexpand(vbox, TRUE);
	gtk_widget_set_vexpand(vbox, TRUE);
	gtk_widget_set_margin(vbox, 18, 18, 18, 18);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(about))), vbox);

	name_label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), name_label, FALSE, FALSE, 0);
	name = g_strdup_printf(_("<b>%s</b>"), plugin->name);
	gtk_label_set_markup(GTK_LABEL(name_label), name);
	g_free(name);

	description_label = gtk_label_new(plugin->description);
	gtk_box_pack_start(GTK_BOX(vbox), description_label, FALSE, FALSE, 0);

	copyright_label = gtk_label_new(plugin->copyright);
	gtk_box_pack_start(GTK_BOX(vbox), copyright_label, FALSE, FALSE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	help_button = gtk_button_new_with_label(_("Help"));
	g_signal_connect(help_button, "clicked", G_CALLBACK(plugins_url_button_clicked_cb), plugin->help);
	gtk_box_pack_start(GTK_BOX(hbox), help_button, TRUE, TRUE, 0);

	dummy = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), dummy, FALSE, FALSE, 0);

	dummy = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), dummy, FALSE, FALSE, 0);

	homepage_button = gtk_button_new_with_label(_("Homepage"));
	g_signal_connect(homepage_button, "clicked", G_CALLBACK(plugins_url_button_clicked_cb), plugin->homepage);
	gtk_box_pack_end(GTK_BOX(hbox), homepage_button, TRUE, TRUE, 0);

	gtk_widget_show_all(vbox);
	gtk_dialog_run(GTK_DIALOG(about));
	gtk_widget_destroy(about);
}

/**
 * plugins_help_button_clicked_cb:
 * @button: button #GtkWidget
 * user_data: a #GtkListBox
 *
 * Opens help page of selected plugin
 */
static void plugins_help_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(user_data));
	GtkWidget *child = gtk_container_get_children(GTK_CONTAINER(row))->data;
	RmPlugin *plugin = g_object_get_data(G_OBJECT(child), "plugin");

	if (!plugin) {
		return;
	}

	gtk_show_uri_on_window(GTK_WINDOW(journal_get_window()), plugin->help, gtk_get_current_event_time (), NULL);
}

/**
 * plugins_configure_button_clicked_cb:
 * @button: button #GtkWidget
 * @user_data: a #GtkListBox
 *
 * Creates and opens configure window of selected plugin
 */
static void plugins_configure_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(user_data));
	GtkWidget *child = gtk_container_get_children(GTK_CONTAINER(row))->data;
	RmPlugin *plugin = g_object_get_data(G_OBJECT(child), "plugin");

	if (!plugin) {
		return;
	}

	if (plugin->configure) {
		GtkWidget *config = plugin->configure(plugin);
		GtkWidget *win;
		GtkWidget *headerbar;

		if (!config) {
			return;
		}

		win = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, NULL);;
		gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(plugins_window));
		gtk_window_set_modal(GTK_WINDOW(win), TRUE);

		headerbar = gtk_dialog_get_header_bar(GTK_DIALOG(win));
		gtk_header_bar_set_title(GTK_HEADER_BAR (headerbar), plugin->name);
		gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(headerbar), TRUE);

		gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(win))), config);

		gtk_widget_show_all(config);
		gtk_dialog_run(GTK_DIALOG(win));
		gtk_widget_destroy(win);
	}
}

/**
 * plugins_listbox_row_selected_cb:
 * @box: a #GtkListBox
 * @row: a #GtkListBoxRow
 * @user_data: UNUSED
 *
 * Select row: depending on plugin state & function activate configure/help button
 */
static void plugins_listbox_row_selected_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	GtkWidget *child;
	RmPlugin *plugin;

	if (!box || !row) {
		return;
	}

	child = gtk_container_get_children(GTK_CONTAINER(row))->data;
	plugin = g_object_get_data(G_OBJECT(child), "plugin");
	if (!plugin) {
		return;
	}

	if (plugin->configure && plugin->enabled) {
		gtk_widget_set_sensitive(config_button, TRUE);
	} else {
		gtk_widget_set_sensitive(config_button, FALSE);
	}

	if (plugin->help) {
		gtk_widget_set_sensitive(help_button, TRUE);
	} else {
		gtk_widget_set_sensitive(help_button, FALSE);
	}
}

/**
 * app_plugins:
 *
 * Show plugins dialog
 */
void app_plugins(void)
{
	GtkWidget *header;
	GtkWidget *grid;
	GtkWidget *scroll_win;
	GtkWidget *toolbar;
	GSList *list;

	if (plugins_window) {
		gtk_window_present(GTK_WINDOW(plugins_window));
		return;
	}

	plugins_window = gtk_application_window_new(GTK_APPLICATION(journal_application));
	gtk_window_set_default_size(GTK_WINDOW(plugins_window), 700, 500);
	gtk_window_set_transient_for(GTK_WINDOW(plugins_window), GTK_WINDOW(journal_get_window()));
	gtk_window_set_position(GTK_WINDOW(plugins_window), GTK_WIN_POS_CENTER_ON_PARENT);
	//gtk_window_set_modal(GTK_WINDOW(plugins_window), TRUE);

	header = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), _("Configure plugins"));
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_window_set_titlebar(GTK_WINDOW(plugins_window), header);
	g_signal_connect(plugins_window, "destroy", G_CALLBACK(gtk_widget_destroyed), &plugins_window);

	gtk_window_set_transient_for(GTK_WINDOW(plugins_window), GTK_WINDOW(journal_get_window()));

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_vexpand(scroll_win, TRUE);

	GtkWidget *listbox = gtk_list_box_new();

	grid = gtk_grid_new();
	gtk_grid_attach(GTK_GRID(grid), scroll_win, 0, 0, 1, 1);

	for (list = rm_plugins_get(); list != NULL; list = list->next) {
		RmPlugin *plugin = list->data;

		if (plugin->builtin) {
			continue;
		}

		GtkWidget *child = gtk_grid_new();
		GtkWidget *on_switch;
		GtkWidget *name = gtk_label_new(plugin->name);
		GtkWidget *description = gtk_label_new(plugin->description);

		gtk_widget_set_margin(child, 12, 6, 12, 6);
		gchar *tmp = g_strdup_printf("<b>%s</b>", plugin->name);
		gtk_label_set_markup(GTK_LABEL(name), tmp);
		g_free(tmp);
		gtk_widget_set_hexpand(name, TRUE);
		gtk_widget_set_halign(name, GTK_ALIGN_START);

		gtk_widget_set_hexpand(description, TRUE);
		gtk_widget_set_halign(description, GTK_ALIGN_START);

		gtk_grid_attach(GTK_GRID(child), name, 0, 0, 1, 1);

		gtk_label_set_ellipsize(GTK_LABEL(description), PANGO_ELLIPSIZE_END);
		gtk_grid_attach(GTK_GRID(child), description, 0, 1, 1, 1);

		on_switch = gtk_switch_new();
		gtk_widget_set_margin(on_switch, 6, 6, 3, 6);
		gtk_widget_set_vexpand(on_switch, FALSE);
		gtk_switch_set_active(GTK_SWITCH(on_switch), plugin->enabled);
		g_signal_connect(G_OBJECT(on_switch), "state-set", G_CALLBACK(plugins_switch_state_set_cb), plugin);

		gtk_widget_set_sensitive(on_switch, !plugin->builtin);
		gtk_grid_attach(GTK_GRID(child), on_switch, 1, 0, 1, 2);

		g_object_set_data(G_OBJECT(child), "plugin", plugin);

		gtk_list_box_insert(GTK_LIST_BOX(listbox), child, -1);
	}
	gtk_container_add(GTK_CONTAINER(scroll_win), listbox);
	g_signal_connect(listbox, "row-selected", G_CALLBACK(plugins_listbox_row_selected_cb), NULL);

	toolbar = gtk_toolbar_new();
	gtk_style_context_add_class(gtk_widget_get_style_context(toolbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	GtkToolItem *item = gtk_tool_item_new();
	GtkWidget *button;
	GtkWidget *image;

	config_button = gtk_button_new();
	image = gtk_image_new_from_icon_name("preferences-system-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(config_button), image);

	gtk_container_add(GTK_CONTAINER(item), config_button);

	g_signal_connect(config_button, "clicked", G_CALLBACK(plugins_configure_button_clicked_cb), listbox);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);

	item = gtk_separator_tool_item_new();

	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
	gtk_tool_item_set_expand(item, TRUE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 1);

	GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_hexpand(button_box, FALSE);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_EXPAND);
	gtk_style_context_add_class(gtk_widget_get_style_context(button_box), GTK_STYLE_CLASS_LINKED);
	item = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(item), button_box);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 2);

	button = gtk_button_new();
	help_button = button;
	image = gtk_image_new_from_icon_name("dialog-information-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	g_signal_connect(help_button, "clicked", G_CALLBACK(plugins_about_button_clicked_cb), listbox);
	gtk_box_pack_start(GTK_BOX(button_box), button, FALSE, FALSE, 0);

	button = gtk_button_new();
	help_button = button;
	image = gtk_image_new_from_icon_name("help-faq-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	g_signal_connect(help_button, "clicked", G_CALLBACK(plugins_help_button_clicked_cb), listbox);
	gtk_box_pack_start(GTK_BOX(button_box), button, FALSE, FALSE, 0);

	gtk_grid_attach(GTK_GRID(grid), toolbar, 0, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(plugins_window), grid);

	gtk_widget_show_all(plugins_window);
}
