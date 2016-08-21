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

#include <config.h>

#include <libpeas/peas.h>

#include <libroutermanager/rmprofile.h>
#include <libroutermanager/rmobject.h>
#include <libroutermanager/rmplugins.h>

/**
 * SECTION:rmplugins
 * @Title: RmPlugins
 * @Short_description: Plugins handling functions
 *
 * Adds plugin support based on libpeas.
 */

/** Internal search path list */
static GSList *rm_search_path_list = NULL;
/** Internal peas exten set */
static PeasExtensionSet *rm_extension = NULL;
/** peas engine */
PeasEngine *rm_engine = NULL;

/**
 * rm_plugins_add_search_path:
 * @path: additional search path for plugins
 *
 * Add additional search @path for peas.
 */
void rm_plugins_add_search_path(gchar *path)
{
	rm_search_path_list = g_slist_append(rm_search_path_list, g_strdup(path));
}

/**
 * rm_plugins_extension_added_cb:
 * @set: a #PeasExtensionSet
 * @info: a #PeasPluginInfo
 * @extension: a #PeasExtension
 * @unused: unused data poiner
 *
 * Add extension callback, activates a plugin.
 */
static void rm_plugins_extension_added_cb(PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *extension, gpointer unused)
{
	/* Active plugin now */
	g_debug(" + %s (%s) activated", peas_plugin_info_get_name(info), peas_plugin_info_get_module_name(info));

	peas_activatable_activate(PEAS_ACTIVATABLE(extension));
}

/**
 * rm_plugins_extension_removed_cb:
 * @set: a #PeasExtensionSet
 * @info: a #PeasPluginInfo
 * @extension: a #PeasExtension
 * @unused: unused data poiner
 *
 * Remove extensionsion callback, deactivates a plugin.
 */
static void rm_plugins_extension_removed_cb(PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *extension, gpointer unused)
{
	/* Remove 'non-builtin' plugin now */
	if (!peas_plugin_info_is_builtin(info)) {
		g_debug(" - %s (%s) deactivated", peas_plugin_info_get_name(info), peas_plugin_info_get_module_name(info));
	}

	peas_activatable_deactivate(PEAS_ACTIVATABLE(extension));
}

/**
 * rm_plugins_init:
 *
 * Find and load builtin plugins.
 */
void rm_plugins_init(void)
{
	GSList *slist;
	const GList *list;

	/* Get default rm_engine */
	rm_engine = peas_engine_get_default();

	/* Set app object as object to rm_engine */
	rm_extension = peas_extension_set_new(rm_engine, PEAS_TYPE_ACTIVATABLE, "object", rm_object, NULL);

	/* Connect rm_extensionsion added/removed signals */
	g_signal_connect(rm_extension, "extension-added", G_CALLBACK(rm_plugins_extension_added_cb), NULL);
	g_signal_connect(rm_extension, "extension-removed", G_CALLBACK(rm_plugins_extension_removed_cb), NULL);

	/* Look for plugins in plugin_dir */
	peas_engine_add_search_path(rm_engine, RM_PLUGINS, RM_PLUGINS);

	/* And all other directories */
	for (slist = rm_search_path_list; slist != NULL; slist = slist->next) {
		gchar *plugin_dir = slist->data;

		peas_engine_add_search_path(rm_engine, plugin_dir, plugin_dir);
	}

	/* In addition to C we want to support python plugins */
	peas_engine_enable_loader(rm_engine, "python3");

	/* Traverse through detected plugins and loaded builtin plugins now */
	for (list = peas_engine_get_plugin_list(rm_engine); list != NULL; list = list->next) {
		PeasPluginInfo *info = list->data;

		if (peas_plugin_info_is_builtin(info)) {
			peas_engine_load_plugin(rm_engine, info);
		}
	}
}

/**
 * rm_plugins_bind_loaded_plugins:
 *
 * Add plugins information to profile settings.
 */
void rm_plugins_bind_loaded_plugins(void)
{
	g_settings_bind(rm_profile_get_active()->settings, "active-plugins", rm_engine, "loaded-plugins", G_SETTINGS_BIND_DEFAULT);
}

/**
 * rm_plugins_shutdown:
 *
 * Shutdown plugins.
 */
void rm_plugins_shutdown(void)
{
	if (!rm_extension) {
		return;
	}

	g_signal_handlers_disconnect_by_func(rm_extension, G_CALLBACK(rm_plugins_extension_added_cb), NULL);
	g_signal_handlers_disconnect_by_func(rm_extension, G_CALLBACK(rm_plugins_extension_removed_cb), NULL);

	peas_extension_set_foreach(rm_extension, rm_plugins_extension_removed_cb, NULL);

	g_clear_object(&rm_extension);
	g_clear_object(&rm_engine);
}
