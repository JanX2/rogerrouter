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
#include <libroutermanager/password.h>

#include <roger/main.h>
#include <roger/pref.h>
#include <roger/pref_security.h>

/**
 * \brief Create security page widget
 * \return security widget
 */
GtkWidget *pref_page_security(void)
{
	GtkWidget *g;
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *pm_label;
	GtkWidget *pm_combobox;

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

	pm_label = ui_label_new(_("Password manager"));
	gtk_grid_attach(GTK_GRID(grid), pm_label, 0, 0, 1, 1);

	pm_combobox = gtk_combo_box_text_new();
	gtk_widget_set_hexpand(pm_combobox, TRUE);
	gtk_grid_attach(GTK_GRID(grid), pm_combobox, 1, 0, 1, 1);

	GSList *plugins = password_manager_get_plugins();
	GSList *list;

	for (list = plugins; list; list = list->next) {
		struct password_manager *pm = list->data;

		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pm_combobox), pm->name, pm->name);
	}

	if (!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(pm_combobox))) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(pm_combobox), 0);
	}

	g_settings_bind(profile_get_active()->settings, "password-manager", pm_combobox, "active-id", G_SETTINGS_BIND_DEFAULT);

	g = pref_group_create(grid, _("Security"), TRUE, TRUE);
	gtk_widget_set_margin(g, 6, 6, 6, 6);

	return g;
}
