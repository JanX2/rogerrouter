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

#include <gtk/gtk.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/plugins.h>

#include <roger/main.h>
#include <roger/pref.h>
#include <roger/pref_misc.h>

/**
 * \brief Ghostscript file set callback - set file name to settings
 * \param choose file chooser widget
 * \param user_data user data pointer (NULL)
 */
void gs_file_set_cb(GtkFileChooser *chooser, gpointer user_data)
{
	gchar *filename = gtk_file_chooser_get_current_folder(chooser);

	g_settings_set_string(profile_get_active()->settings, "ghostscript", filename);

	g_free(filename);
}

/**
 * \brief Ghostscript button clicked callback - open file chooser dialog
 * \param button gs button widget
 * \param user_data user data pointer (NULL)
 */
void gs_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Select ghostscript executable"), pref_get_window(), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		gtk_entry_set_text(GTK_ENTRY(user_data), folder);

		g_free(folder);
	}

	gtk_widget_destroy(dialog);
}

/**
 * \brief Create misc page widget
 * \return misc widget
 */
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
