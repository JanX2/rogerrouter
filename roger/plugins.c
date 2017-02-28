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

#include <rm/rmplugins.h>
#include <rm/rmosdep.h>

#include <main.h>
#include <plugins.h>
#include <uitools.h>
#include <journal.h>

static GtkWidget *plugins_window = NULL;

gboolean extensions_state_set_cb(GtkSwitch *widget, gboolean state, gpointer user_data)
{
	RmPlugin *plugin = user_data;

	if (state) {
		rm_plugins_enable(plugin);
	} else {
		rm_plugins_disable(plugin);
	}

	return FALSE;
}

static void extensions_about_clicked_cb(GtkWidget *button,
                                       gpointer   user_data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(user_data));
	GtkWidget *child = gtk_container_get_children(GTK_CONTAINER(row))->data;
	RmPlugin *plugin = g_object_get_data(G_OBJECT(child), "plugin");
	GtkWidget *about;
	GtkWidget *image;
	GdkPixbuf *pixbuf;

	if (!plugin) {
		return;
	}

	about = gtk_about_dialog_new();
	gtk_window_set_title(GTK_WINDOW(about), plugin->name);
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), plugin->name);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), plugin->description);
	image = gtk_image_new_from_icon_name("application-x-addon-symbolic", GTK_ICON_SIZE_DIALOG);
	pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), pixbuf);

	gtk_dialog_run(GTK_DIALOG(about));
	gtk_widget_destroy(about);
}

static void extensions_help_clicked_cb(GtkWidget *button,
                                       gpointer   user_data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(user_data));
	GtkWidget *child = gtk_container_get_children(GTK_CONTAINER(row))->data;
	RmPlugin *plugin = g_object_get_data(G_OBJECT(child), "plugin");

	if (!plugin) {
		return;
	}

	rm_os_execute(plugin->help_url);
}

static void extensions_configure_clicked_cb(GtkWidget *button,
                                       gpointer   user_data)
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

		if (!config) {
			return;
		}

		win = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(win), plugin->name);
		gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(win))), config);
		gtk_widget_show_all(win);
		gtk_dialog_run(GTK_DIALOG(win));
		gtk_widget_destroy(win);
	}
}

static GtkWidget *config_button = NULL;
static GtkWidget *help_button = NULL;

static void extensions_listbox_row_selected_cb(GtkListBox    *box,
               GtkListBoxRow *row,
               gpointer       user_data)
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

	if (plugin->configure) {
		gtk_widget_set_sensitive(config_button, TRUE);
	} else {
		gtk_widget_set_sensitive(config_button, FALSE);
	}

	if (plugin->help_url) {
		gtk_widget_set_sensitive (help_button, TRUE);
	} else {
		gtk_widget_set_sensitive (help_button, FALSE);
	}
}

gboolean plugins_delete_event_cb(GtkWidget *win,
                                 gpointer   user_data)
{
	g_debug("%s(): Called", __FUNCTION__);
	return FALSE;
}

void app_show_plugins(void)
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

	extern GApplication *journal_application;
	plugins_window = gtk_application_window_new(GTK_APPLICATION(journal_application));
	gtk_window_set_default_size(GTK_WINDOW(plugins_window), 700, 350);

	header = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), _("Configure extensions"));
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_window_set_titlebar(GTK_WINDOW(plugins_window), header);
	gtk_window_set_modal(GTK_WINDOW(plugins_window), TRUE);
	//g_signal_connect(plugins_window, "delete-event", G_CALLBACK(plugins_delete_event_cb), NULL);
	g_signal_connect(plugins_window, "destroy", G_CALLBACK(gtk_widget_destroyed), &plugins_window);

	gtk_window_set_transient_for(GTK_WINDOW(plugins_window), GTK_WINDOW(journal_get_window()));

	scroll_win = gtk_scrolled_window_new (NULL, NULL);
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
		gtk_grid_attach(GTK_GRID(child), description, 0, 1, 1, 1);

		on_switch = gtk_switch_new();
		gtk_widget_set_margin(on_switch, 6, 6, 3, 6);
		gtk_widget_set_vexpand(on_switch, FALSE);
		gtk_switch_set_active(GTK_SWITCH(on_switch), plugin->enabled);
		g_signal_connect(G_OBJECT(on_switch), "state-set", G_CALLBACK(extensions_state_set_cb), plugin);

		gtk_widget_set_sensitive(on_switch, !plugin->builtin);
		gtk_grid_attach(GTK_GRID(child), on_switch, 1, 0, 1, 2);

		g_object_set_data(G_OBJECT(child), "plugin", plugin);

		gtk_list_box_insert(GTK_LIST_BOX(listbox), child, -1);
	}
	gtk_container_add(GTK_CONTAINER(scroll_win), listbox);
	g_signal_connect(listbox, "row-selected", G_CALLBACK(extensions_listbox_row_selected_cb), NULL);

	toolbar = gtk_toolbar_new();
	gtk_style_context_add_class(gtk_widget_get_style_context(toolbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	GtkToolItem *item = gtk_tool_item_new();
	GtkWidget *button;
	GtkWidget *image;

	config_button = gtk_button_new();
	image = gtk_image_new_from_icon_name("preferences-system-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(config_button), image);

	gtk_container_add(GTK_CONTAINER(item), config_button);

	g_signal_connect(config_button, "clicked", G_CALLBACK(extensions_configure_clicked_cb), listbox);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);

	item = gtk_separator_tool_item_new();

	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
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
	g_signal_connect(help_button, "clicked", G_CALLBACK(extensions_about_clicked_cb), listbox);
	gtk_box_pack_start(GTK_BOX(button_box), button, FALSE, FALSE, 0);

	button = gtk_button_new();
	help_button = button;
	image = gtk_image_new_from_icon_name("help-faq-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	g_signal_connect(help_button, "clicked", G_CALLBACK(extensions_help_clicked_cb), listbox);
	gtk_box_pack_start(GTK_BOX(button_box), button, FALSE, FALSE, 0);

	gtk_grid_attach(GTK_GRID(grid), toolbar, 0, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(plugins_window), grid);

	gtk_widget_show_all(plugins_window);
}
