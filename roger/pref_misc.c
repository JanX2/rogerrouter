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

#include <gtk/gtk.h>
#include <libcallibre/profile.h>
#include <libcallibre/plugins.h>

#include <main.h>
#include <pref.h>
#include <pref_misc.h>

void gs_file_set_cb(GtkFileChooser *chooser, gpointer user_data)
{
	gchar *filename = gtk_file_chooser_get_current_folder(chooser);

	g_settings_set_string(profile_get_active()->settings, "ghostscript", filename);

	g_free(filename);
}

void gs_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Select ghostscript executable"), pref_get_window(), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		gtk_entry_set_text(GTK_ENTRY(user_data), folder);

		g_free(folder);
	}

	gtk_widget_destroy(dialog);
}

GtkWidget *pref_page_misc(void)
{
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *gs_label;
	GtkWidget *gs_entry;
	GtkWidget *gs_button;

	/* Ghostscript executable:
	 * <FILE-CHOOSER>
	 */

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

	gs_label = ui_label_new(_("Ghostscript executable"));
	gtk_grid_attach(GTK_GRID(grid), gs_label, 0, 0, 1, 1);

	gs_entry = gtk_entry_new();
	gtk_widget_set_hexpand(gs_entry, TRUE);
	g_settings_bind(profile_get_active()->settings, "ghostscript", gs_entry, "text", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(grid), gs_entry, 1, 0, 1, 1);

	gs_button = gtk_button_new_with_label(_("Select"));
	gtk_grid_attach(GTK_GRID(grid), gs_button, 2, 0, 1, 1);
	g_signal_connect(gs_button, "clicked", G_CALLBACK(gs_button_clicked_cb), gs_entry);

	return pref_group_create(grid, _("Misc"), TRUE, TRUE);
}
