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
#include <libroutermanager/router.h>
#include <libroutermanager/plugins.h>

#include <roger/main.h>
#include <roger/pref.h>

#include <config.h>

/**
 * \brief Create softphone preference page
 * \return preference widget
 */
GtkWidget *pref_page_softphone(void)
{
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *general_grid = gtk_grid_new();
	GtkWidget *phone_number_label;
	GtkWidget *phone_number_combobox;
	GtkWidget *controller_label;
	GtkWidget *controller_combobox;

	/* General:
	 * Phone number: <COMBOBOX>
	 * Controller: <COMBOBOX>
	 *
	 * Bluetooth-Headset:
	 * Use buttons of: <COMBOBOX>
	 */

	/* General */
	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(general_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(general_grid), 15);

	phone_number_label = ui_label_new(_("MSN"));
	gtk_grid_attach(GTK_GRID(general_grid), phone_number_label, 0, 0, 1, 1);

	phone_number_combobox = gtk_combo_box_text_new();
	gchar **numbers = router_get_numbers(profile_get_active());
	gint idx;

	for (idx = 0; idx < g_strv_length(numbers); idx++) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(phone_number_combobox), numbers[idx], numbers[idx]);
	}
	g_settings_bind(profile_get_active()->settings, "phone-number", phone_number_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);
	gtk_widget_set_hexpand(phone_number_combobox, TRUE);
	gtk_grid_attach(GTK_GRID(general_grid), phone_number_combobox, 1, 0, 1, 1);

	controller_label = ui_label_new(_("Controller"));
	gtk_grid_attach(GTK_GRID(general_grid), controller_label, 0, 1, 1, 1);

	controller_combobox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("ISDN-Controller 1"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("ISDN-Controller 2"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("Outside line ISDN"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("Outside line Analog"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(controller_combobox), _("Internet"));
	g_settings_bind(profile_get_active()->settings, "phone-controller", controller_combobox, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_widget_set_hexpand(controller_combobox, TRUE);
	gtk_grid_attach(GTK_GRID(general_grid), controller_combobox, 1, 1, 1, 1);

	gtk_grid_attach(GTK_GRID(grid), pref_group_create(general_grid, _("General"), TRUE, TRUE), 0, 0, 1, 1);

	return grid;
}
