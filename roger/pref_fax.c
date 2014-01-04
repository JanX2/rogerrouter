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

#include <libroutermanager/profile.h>
#include <libroutermanager/router.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/gstring.h>

#include <roger/main.h>
#include <roger/pref.h>

/**
 * \brief Report dir set callback - store it in the settings
 * \param chooser file chooser widget
 * \param user_data user data pointer (NULL)
 */
void report_dir_file_set_cb(GtkFileChooser *chooser, gpointer user_data)
{
	gchar *filename = gtk_file_chooser_get_current_folder(chooser);

	g_debug("uri: '%s'", filename);
	g_settings_set_string(profile_get_active()->settings, "fax-report-dir", filename);

	g_free(filename);
}

/**
 * \brief Report dir button callback - open file chooser dialog
 * \param button button widget
 * \param user_data user data pointer (NULL)
 */
void report_dir_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Select fax report directory"), pref_get_window(), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		gtk_entry_set_text(GTK_ENTRY(user_data), folder);

		g_free(folder);
	}

	gtk_widget_destroy(dialog);
}

/**
 * \brief Create fax preferences page
 * \return fax widget
 */
GtkWidget *pref_page_fax(void)
{
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *general_grid = gtk_grid_new();
	GtkWidget *report_grid = gtk_grid_new();
	GtkWidget *modem_grid = gtk_grid_new();
	GtkWidget *header_label;
	GtkWidget *header_entry;
	GtkWidget *ident_label;
	GtkWidget *ident_entry;
	GtkWidget *resolution_label;
	GtkWidget *resolution_combobox;
	GtkWidget *report_toggle;
	GtkWidget *report_dir_label;
	GtkWidget *fax_number_label;
	GtkWidget *fax_number_combobox;
	GtkWidget *bitrate_label;
	GtkWidget *bitrate_combobox;
	GtkWidget *controller_label;
	GtkWidget *controller_combobox;
	GtkWidget *ecm_toggle;

	/* General:
	 * Header: <ENTRY>
	 * Ident: <ENTRY>
	 * <CHECKBOX> resolution fax quality
	 *
	 * Reports:
	 * <CHECKBOX> Create fax report
	 * Directory: <FILECHOOSER>
	 *
	 * Modem:
	 * Fax number: <COMBOBOX>
	 * Bitrate: <COMBOBOX>
	 * Controller: <COMBOBOX>
	 * <CHECKBOX> Error correction mode
	 */

	/* General */
	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(general_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(general_grid), 15);

	header_label = ui_label_new(_("Header"));
	gtk_grid_attach(GTK_GRID(general_grid), header_label, 0, 0, 1, 1);

	header_entry = gtk_entry_new();
	gtk_widget_set_hexpand(header_entry, TRUE);
	g_settings_bind(profile_get_active()->settings, "fax-header", header_entry, "text", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(general_grid), header_entry, 1, 0, 1, 1);

	ident_label = ui_label_new(_("Ident"));
	gtk_grid_attach(GTK_GRID(general_grid), ident_label, 0, 1, 1, 1);

	ident_entry = gtk_entry_new();
	gtk_widget_set_hexpand(ident_entry, TRUE);
	g_settings_bind(profile_get_active()->settings, "fax-ident", ident_entry, "text", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(general_grid), ident_entry, 1, 1, 1, 1);

	resolution_label = ui_label_new(_("Resolution"));
	gtk_grid_attach(GTK_GRID(general_grid), resolution_label, 0, 3, 1, 1);

	resolution_combobox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(resolution_combobox), _("Low (98dpi)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(resolution_combobox), _("High (196dpi)"));
	g_settings_bind(profile_get_active()->settings, "fax-resolution", resolution_combobox, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(general_grid), resolution_combobox, 1, 3, 1, 1);

	gtk_grid_attach(GTK_GRID(grid), pref_group_create(general_grid, _("General"), TRUE, TRUE), 0, 0, 1, 1);

	/* Reports */

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(report_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(report_grid), 15);

	report_toggle = gtk_check_button_new_with_label(_("Create fax report"));
	GdkRGBA col;
	gdk_rgba_parse(&col, "#808080");
	gtk_widget_override_color(report_toggle, GTK_STATE_FLAG_NORMAL, &col);

	g_settings_bind(profile_get_active()->settings, "fax-report", report_toggle, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(report_grid), report_toggle, 0, 0, 1, 1);

	report_dir_label = ui_label_new(_("Report directory"));
	gtk_grid_attach(GTK_GRID(report_grid), report_dir_label, 0, 1, 1, 1);

#if 0
	/* Currently the file chooser button implementation is buggy and therefore we will use a workaround (entry + button) */
	GtkWidget *report_dir;
	report_dir = gtk_file_chooser_button_new(_("Select report directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_widget_set_hexpand(report_dir, TRUE);
	const gchar *path = g_settings_get_string(profile_get_active()->settings, "fax-report-dir");
	g_debug("path: '%s'", path);

	if (!gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(report_dir), path)) {
		g_debug("Setting fallback directory");
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(report_dir), g_get_home_dir());
	}
	g_signal_connect(report_dir, "selection-changed", G_CALLBACK(report_dir_file_set_cb), NULL);
	gtk_grid_attach(GTK_GRID(report_grid), report_dir, 1, 1, 1, 1);
#else
	GtkWidget *report_dir_entry = gtk_entry_new();
	GtkWidget *report_dir_button = gtk_button_new_with_label(_("Select"));

	gtk_widget_set_hexpand(report_dir_entry, TRUE);
	g_settings_bind(profile_get_active()->settings, "fax-report-dir", report_dir_entry, "text", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(report_dir_button, "clicked", G_CALLBACK(report_dir_button_clicked_cb), report_dir_entry);

	gtk_grid_attach(GTK_GRID(report_grid), report_dir_entry, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(report_grid), report_dir_button, 2, 1, 1, 1);
#endif

	gtk_grid_attach(GTK_GRID(grid), pref_group_create(report_grid, _("Report"), TRUE, TRUE), 0, 1, 1, 1);

	/* Modem */

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(modem_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(modem_grid), 15);

	/* MSN */
	fax_number_label = ui_label_new(_("MSN"));
	gtk_grid_attach(GTK_GRID(modem_grid), fax_number_label, 0, 0, 1, 1);

	fax_number_combobox = gtk_combo_box_text_new();
	gchar **numbers = router_get_numbers(profile_get_active());
	gint idx;

	for (idx = 0; idx < g_strv_length(numbers); idx++) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(fax_number_combobox), numbers[idx], numbers[idx]);
	}
	g_settings_bind(profile_get_active()->settings, "fax-number", fax_number_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	gtk_widget_set_hexpand(fax_number_combobox, TRUE);
	gtk_grid_attach(GTK_GRID(modem_grid), fax_number_combobox, 1, 0, 1, 1);

	bitrate_label = ui_label_new(_("Bitrate"));
	gtk_grid_attach(GTK_GRID(modem_grid), bitrate_label, 0, 1, 1, 1);

	bitrate_combobox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(bitrate_combobox), "4800");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(bitrate_combobox), "9600");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(bitrate_combobox), "14400");
#if defined(T30_SUPPORT_V34)
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(bitrate_combobox), "28800");
#endif
#if defined(T30_SUPPORT_V34HDX)
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(bitrate_combobox), "33600");
#endif
	g_settings_bind(profile_get_active()->settings, "fax-bitrate", bitrate_combobox, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(modem_grid), bitrate_combobox, 1, 1, 1, 1);

	controller_label = ui_label_new(_("Controller"));
	gtk_grid_attach(GTK_GRID(modem_grid), controller_label, 0, 2, 1, 1);

	controller_combobox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("ISDN-Controller 1"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("ISDN-Controller 2"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("Outside line ISDN"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("Outside line Analog"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("Internet"));
	g_settings_bind(profile_get_active()->settings, "fax-controller", controller_combobox, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(modem_grid), controller_combobox, 1, 2, 1, 1);

	ecm_toggle = gtk_check_button_new_with_label(_("Error correction mode (ECM)"));
	gtk_widget_override_color(ecm_toggle, GTK_STATE_FLAG_NORMAL, &col);
	g_settings_bind(profile_get_active()->settings, "fax-ecm", ecm_toggle, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(modem_grid), ecm_toggle, 0, 3, 2, 1);

	gtk_grid_attach(GTK_GRID(grid), pref_group_create(modem_grid, _("Modem"), TRUE, TRUE), 0, 2, 1, 1);

	return grid;
}
