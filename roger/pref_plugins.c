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

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/plugins.h>

#include <roger/main.h>
#include <roger/pref.h>
#include <roger/pref_plugins.h>

/**
 * \brief Create plugins preferences page
 * \return plugins widget
 */
GtkWidget *pref_page_plugins(void)
{
	GtkWidget *plugins = peas_gtk_plugin_manager_new(engine);

	gtk_widget_set_hexpand(plugins, TRUE);
	gtk_widget_set_vexpand(plugins, TRUE);

	g_settings_bind(profile_get_active()->settings, "active-plugins", engine, "loaded-plugins", G_SETTINGS_BIND_DEFAULT);

#ifdef DEBUG
	peas_gtk_plugin_manager_view_set_show_builtin(PEAS_GTK_PLUGIN_MANAGER_VIEW(peas_gtk_plugin_manager_get_view(PEAS_GTK_PLUGIN_MANAGER(plugins))), TRUE);
#endif

	return pref_group_create(plugins, _("Choose which plugins should be loaded at startup"), TRUE, TRUE);
}
