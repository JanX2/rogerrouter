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
#include <libroutermanager/audio.h>

#include <roger/main.h>
#include <roger/pref.h>
#include <roger/pref_audio.h>

/**
 * \brief Create audio preferences widget
 * \return audio widget
 */
GtkWidget *pref_page_audio(void)
{
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *plugin_label;
	GtkWidget *plugin_combobox;
	//GtkWidget *space;
	GtkWidget *output_label;
	GtkWidget *output_combobox;
	GtkWidget *input_label;
	GtkWidget *input_combobox;

	/* Audio:
	 * Plugin: <COMBOBOX>
	 *
	 * Output: <COMBOBOX>
	 * Input: <COMBOBOX>
	 */

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

	plugin_label = ui_label_new(_("Plugin"));
	gtk_grid_attach(GTK_GRID(grid), plugin_label, 0, 0, 1, 1);

	plugin_combobox = gtk_combo_box_text_new();
	gtk_widget_set_hexpand(plugin_combobox, TRUE);
	gtk_grid_attach(GTK_GRID(grid), plugin_combobox, 1, 0, 1, 1);

	output_label = ui_label_new(_("Output"));
	gtk_grid_attach(GTK_GRID(grid), output_label, 0, 2, 1, 1);

	output_combobox = gtk_combo_box_text_new();
	gtk_grid_attach(GTK_GRID(grid), output_combobox, 1, 2, 1, 1);

	input_label = ui_label_new(_("Input"));
	gtk_grid_attach(GTK_GRID(grid), input_label, 0, 3, 1, 1);

	input_combobox = gtk_combo_box_text_new();
	gtk_grid_attach(GTK_GRID(grid), input_combobox, 1, 3, 1, 1);

	struct audio *audio = audio_get_default();
	if (audio) {
		GSList *devices = audio->get_devices();
		GSList *list;

		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(plugin_combobox), audio->name, audio->name);

		for (list = devices; list; list = list->next) {
			struct audio_device *device = list->data;

			if (device->type == AUDIO_INPUT) {
				gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(input_combobox), device->internal_name, device->name);
			} else {
				gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(output_combobox), device->internal_name, device->name);
			}
		}

		if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(input_combobox))) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(input_combobox), 0);
		}

		if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(output_combobox))) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(output_combobox), 0);
		}

		if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(plugin_combobox))) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(plugin_combobox), 0);
		}
	}

	g_settings_bind(profile_get_active()->settings, "audio-plugin", plugin_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile_get_active()->settings, "audio-output", output_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(profile_get_active()->settings, "audio-input", input_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	return pref_group_create(grid, _("Audio"), TRUE, TRUE);
}
