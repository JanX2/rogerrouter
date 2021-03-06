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

#include <roger/shortcuts.h>
#include <roger/journal.h>

/**
 * app_shortcuts:
 *
 * Entry for application shortcuts window
 */
void app_shortcuts(void)
{
	static GtkWidget *shortcuts_window;

	if (!shortcuts_window) {
		GtkBuilder *builder;

		builder = gtk_builder_new_from_resource("/org/tabos/roger/shortcuts.glade");
		shortcuts_window = GTK_WIDGET(gtk_builder_get_object(builder, "shortcuts_window"));

		g_signal_connect(shortcuts_window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), &shortcuts_window);
		g_debug("%s(): Called", __FUNCTION__);
		g_object_unref(builder);
	}
	gtk_window_set_transient_for(GTK_WINDOW(shortcuts_window), GTK_WINDOW(journal_get_window()));

	gtk_widget_show_all(shortcuts_window);
	gtk_window_present(GTK_WINDOW(shortcuts_window));
}
