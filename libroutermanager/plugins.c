/**
 * The libroutermanager project
 * Copyright (c) 2012-2014 Jan-Michael Brummer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <libpeas/peas.h>

#include <libroutermanager/appobject.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/plugins.h>

/** Internal search path list */
static GSList *search_path_list = NULL;
/** Internal peas extension set */
static PeasExtensionSet *exten = NULL;
/** peas engine */
PeasEngine *engine = NULL;

/**
 * \brief Add additional search path for peas
 * \param path path to search for plugins for
 */
void routermanager_plugins_add_search_path(gchar *path)
{
	search_path_list = g_slist_append(search_path_list, g_strdup(path));
}

/**
 * \brief Add extension callback
 * \param set peas extension set
 * \param info peas plugin info
 * \param exten peas extension
 * \param unused unused data poiner
 */
static void plugins_extension_added_cb(PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *exten, gpointer unused)
{
	/* Active plugin now */
	peas_activatable_activate(PEAS_ACTIVATABLE(exten));
}

/**
 * \brief Remove extension callback
 * \param set peas extension set
 * \param info peas plugin info
 * \param exten peas extension
 * \param unused unused data poiner
 */
static void plugins_extension_removed_cb(PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *exten, gpointer unused)
{
	/* Remove plugin now */
	peas_activatable_deactivate(PEAS_ACTIVATABLE(exten));
}

/**
 * \brief Find and load builtin plugins
 */
void plugins_init(void)
{
	GSList *slist;
	const GList *list;

	/* Get default engine */
	engine = peas_engine_get_default();

	/* Set app object as object to engine */
	exten = peas_extension_set_new(engine, PEAS_TYPE_ACTIVATABLE, "object", app_object, NULL);

	/* Connect extension added/removed signals */
	g_signal_connect(exten, "extension-added", G_CALLBACK(plugins_extension_added_cb), NULL);
	g_signal_connect(exten, "extension-removed", G_CALLBACK(plugins_extension_removed_cb), NULL);

	/* Look for plugins in plugin_dir */
	peas_engine_add_search_path(engine, ROUTERMANAGER_PLUGINS "/plugins/", ROUTERMANAGER_PLUGINS "/plugins/");

	/* And all other directories */
	for (slist = search_path_list; slist != NULL; slist = slist->next) {
		gchar *plugin_dir = slist->data;

		peas_engine_add_search_path(engine, plugin_dir, plugin_dir);
	}

	/* In addition to C we want to support python and javascript plugins */
	peas_engine_enable_loader(engine, "python");
	peas_engine_enable_loader(engine, "gjs");

	/* Traverse through detected plugins and loaded builtin plugins now */
	for (list = peas_engine_get_plugin_list(engine); list != NULL; list = list->next) {
		PeasPluginInfo *info = list->data;

		if (peas_plugin_info_is_builtin(info)) {
			peas_engine_load_plugin(engine, info);
		}
	}
}

/**
 * \brief Add plugins information to profile settings
 */
void plugins_user_plugins(void)
{
	g_settings_bind(profile_get_active()->settings, "active-plugins", engine, "loaded-plugins", G_SETTINGS_BIND_DEFAULT);
}

/**
 * \brief Shutdown plugins
 */
void plugins_shutdown(void)
{
	if (exten) {
		g_signal_handlers_disconnect_by_func(exten, G_CALLBACK(plugins_extension_added_cb), NULL);
		g_signal_handlers_disconnect_by_func(exten, G_CALLBACK(plugins_extension_removed_cb), NULL);

		peas_extension_set_foreach(exten, plugins_extension_removed_cb, NULL);

		g_clear_object(&exten);
		g_clear_object(&engine);
	}
}
