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
#include <libroutermanager/router.h>
#include <libroutermanager/ftp.h>

#include <roger/main.h>
#include <roger/phone.h>
#include <roger/pref.h>
#include <roger/pref_prefix.h>

/**
 * \brief Create router phone widget
 * \return router phone widget
 */
static GtkWidget *pref_page_router_phone(void)
{
	GtkWidget *group;
	GtkWidget *line_access_code_label;
	GtkWidget *line_access_code_entry;
	GtkWidget *international_call_prefix_label;
	GtkWidget *international_call_prefix_entry;
	GtkWidget *country_code_label;
	GtkWidget *country_code_entry;
	GtkWidget *national_call_prefix_label;
	GtkWidget *national_call_prefix_entry;
	GtkWidget *area_code_label;
	GtkWidget *area_code_entry;
	gint line = 0;
	gboolean is_cable = router_is_cable(profile_get_active());

	/**
	 * Group settings:
	 * Line access code: <ENTRY>
	 * International call prefix: <ENTRY>
	 * Country Code: <ENTRY>
	 * National call prefix: <ENTRY>
	 * Area Code: <ENTRY>
	 * Port: <ENTRY>
	 */
	group = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(group), 5);
	gtk_grid_set_column_spacing(GTK_GRID(group), 15);

	/* Line access code */
	line_access_code_label = ui_label_new(_("Line access code"));
	gtk_grid_attach(GTK_GRID(group), line_access_code_label, 0, line, 1, 1);

	line_access_code_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(line_access_code_entry, _("If needed enter line access code"));
	gtk_widget_set_hexpand(line_access_code_entry, TRUE);
	gtk_grid_attach(GTK_GRID(group), line_access_code_entry, 1, line, 1, 1);
	g_settings_bind(profile_get_active()->settings, "line-access-code", line_access_code_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/* International call prefix */
	line++;
	international_call_prefix_label = ui_label_new(_("International call prefix"));
	gtk_grid_attach(GTK_GRID(group), international_call_prefix_label, 0, line, 1, 1);

	international_call_prefix_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(international_call_prefix_entry, _("Prefix to be added before the country code to make an international call (usually 00)"));
	gtk_widget_set_hexpand(international_call_prefix_entry, TRUE);
	gtk_editable_set_editable(GTK_EDITABLE(international_call_prefix_entry), is_cable);
	gtk_grid_attach(GTK_GRID(group), international_call_prefix_entry, 1, line, 1, 1);
	g_settings_bind(profile_get_active()->settings, "international-call-prefix", international_call_prefix_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/* Country code */
	line++;
	country_code_label = ui_label_new(_("Country code"));
	gtk_grid_attach(GTK_GRID(group), country_code_label, 0, line, 1, 1);

	country_code_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(country_code_entry, _("Country code without precending zero (e.g. 49=Germany)"));
	gtk_widget_set_hexpand(country_code_entry, TRUE);
	gtk_editable_set_editable(GTK_EDITABLE(country_code_entry), is_cable);
	gtk_grid_attach(GTK_GRID(group), country_code_entry, 1, line, 1, 1);
	g_settings_bind(profile_get_active()->settings, "country-code", country_code_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/* National call prefix */
	line++;
	national_call_prefix_label = ui_label_new(_("National call prefix"));
	gtk_grid_attach(GTK_GRID(group), national_call_prefix_label, 0, line, 1, 1);

	national_call_prefix_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(national_call_prefix_entry, _("Prefix to be added before the area code to make a long distance national call (usually 0)"));
	gtk_widget_set_hexpand(national_call_prefix_entry, TRUE);
	gtk_editable_set_editable(GTK_EDITABLE(national_call_prefix_entry), is_cable);
	gtk_grid_attach(GTK_GRID(group), national_call_prefix_entry, 1, line, 1, 1);
	g_settings_bind(profile_get_active()->settings, "national-call-prefix", national_call_prefix_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/* Area code */
	line++;
	area_code_label = ui_label_new(_("Area code"));
	gtk_grid_attach(GTK_GRID(group), area_code_label, 0, line, 1, 1);

	area_code_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(area_code_entry, _("Area Code without precending zero"));
	gtk_widget_set_hexpand(area_code_entry, TRUE);
	gtk_editable_set_editable(GTK_EDITABLE(area_code_entry), is_cable);
	gtk_grid_attach(GTK_GRID(group), area_code_entry, 1, line, 1, 1);
	g_settings_bind(profile_get_active()->settings, "area-code", area_code_entry, "text", G_SETTINGS_BIND_DEFAULT);

	gtk_widget_set_hexpand(group, TRUE);

	return pref_group_create(group, _("Prefixes"), TRUE, FALSE);
}

/**
 * \brief Create prefix preferences widget
 * \return prefix widget
 */
GtkWidget *pref_page_prefix(void)
{
	GtkWidget *grid;
	GtkWidget *group;

	grid = gtk_grid_new();

	group = pref_page_router_phone();
	gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

	return grid;
}
