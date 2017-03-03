/*
 * The rm project
 * Copyright (c) 2012-2017 Jan-Michael Brummer
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
 * You should have received a copy of the GNU Lesser General Publicl
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>

#include <rmconfig.h>

#include <rm/rmprofile.h>
#include <rm/rmobject.h>
#include <rm/rmplugins.h>
#include <rm/rmstring.h>

GSList *rm_plugins = NULL;

/**
 * SECTION:rmplugins
 * @Title: RmPlugins
 * @Short_description: Plugins handling functions
 *
 * Adds plugin support based on libpeas.
 */

/** Internal search path list */
static GSList *rm_search_path_list = NULL;

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
 * \brief load plugin by filename
 * \param pnName file name
 * \return error code
 */
static gint rm_plugins_load_plugin(char *name)
{
	RmPlugin *plugin;
	GModule *module;
	typedef void (*rmInitPlugin)(RmPlugin *plugin);
	rmInitPlugin init_plugin;
	gpointer tmp;
	GKeyFile *keyfile;
	gchar *module_name;
	gchar *lib_name;

	keyfile = g_key_file_new();
	g_key_file_load_from_file (keyfile, name, G_KEY_FILE_NONE, NULL);

	module_name = g_key_file_get_string(keyfile, "Plugin", "Module", NULL);

	lib_name = g_strdup_printf("%s/lib%s.%s", g_dirname(name), module_name, G_MODULE_SUFFIX);
	//g_debug("%s(): lib_name: %s", __FUNCTION__, lib_name);
	module = g_module_open(lib_name, G_MODULE_BIND_LAZY);
	if (!module) {
		g_warning("%s(): Cannot load plugin %s\n%s", __FUNCTION__, name, g_module_error());
		return -1;
	}

	if (!g_module_symbol(module, "__rm_init_plugin", &tmp)) {
		g_warning("%s(): Cannot load symbol '__rm_init_plugin' : %s", __FUNCTION__, g_module_error());
		g_module_close(module);
		return -2;
	}
	init_plugin = tmp;

	plugin = g_slice_alloc0 (sizeof(RmPlugin));

	init_plugin(plugin);

	plugin->module = module;

	plugin->module_name = module_name;
	plugin->name = g_key_file_get_locale_string(keyfile, "Plugin", "Name", NULL, NULL);
	plugin->description = g_key_file_get_locale_string(keyfile, "Plugin", "Comment", NULL, NULL);
	plugin->copyright = g_key_file_get_string(keyfile, "Plugin", "Copyright", NULL);
	plugin->builtin = g_key_file_get_boolean(keyfile, "Plugin", "Builtin", NULL);
	plugin->help = g_key_file_get_string(keyfile, "Plugin", "Help", NULL);
	plugin->homepage = g_key_file_get_string(keyfile, "Plugin", "Website", NULL);

	/**** First step: Load them immediately, late we will connect it to the profiles */
	if (plugin->builtin) {
		rm_plugins_enable(plugin);
	}
	/**** First step: Load them immediately, late we will connect it to the profiles */

	rm_plugins = g_slist_prepend(rm_plugins, plugin);

	return 0;
}

/**
 * rm_plugins_has_file_extension:
 * @file: filename
 * @ext: extension
 *
 * Check if file has extension
 *
 * Returns: %TRUE or %FALSE
 */
gboolean rm_plugins_has_file_extension(const char *file, const char *ext)
{
	int len, ext_len;

	if (file == NULL || *file == '\0' || ext == NULL) {
		return FALSE;
	}

	ext_len = strlen(ext);
	len = strlen(file) - ext_len;

	if (len < 0) {
		return FALSE;
	}

	return (strncmp(file + len, ext, ext_len ) == 0);
}

/**
 * rm_plugins_load_dir:
 * @plugin_dir: plugin directroy to scan for new plugins
 *
 * Scans @plugin_dir for new plugins (with system extension) and loads them
 */
void rm_plugins_load_dir(gchar *plugin_dir)
{
	GDir *dir;
	const gchar *file;
	gchar *path;
	char *ext = ".plugin";

	dir = g_dir_open(plugin_dir, 0, NULL);

	if (dir) {
		while ((file = g_dir_read_name(dir)) != NULL) {
			path = g_build_filename(plugin_dir, file, NULL);

			if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
				rm_plugins_load_dir(path);
			} else if (rm_plugins_has_file_extension(file, ext)) {
				rm_plugins_load_plugin(path);
			}
			g_free(path);
		}
		g_dir_close(dir);
	}
}

/**
 * rm_plugins_init:
 *
 * Find and load builtin plugins.
 */
void rm_plugins_init(void)
{
	GSList *slist;

	/* And all other directories */
	for (slist = rm_search_path_list; slist != NULL; slist = slist->next) {
		gchar *plugin_dir = slist->data;

		rm_plugins_load_dir(plugin_dir);
	}
}

/**
 * rm_plugins_bind_loaded_plugins:
 *
 * Add plugins information to profile settings.
 */
void rm_plugins_bind_loaded_plugins(void)
{
	GSList *list;
	gchar **active_plugins = g_settings_get_strv(rm_profile_get_active()->settings, "active-plugins");

	g_debug("%s(): Called for profile", __FUNCTION__);

	for (list = rm_plugins; list != NULL; list = list->next) {
		RmPlugin *plugin = list->data;

		if (plugin->builtin) {
			continue;
		}

		if (rm_strv_contains((const gchar * const *) active_plugins, plugin->module_name)) {
			if (!plugin->enabled) {
				rm_plugins_enable(plugin);
			}
		} else if (plugin->enabled) {
			plugin->shutdown(plugin);
		}
	}
}

/**
 * rm_plugins_shutdown:
 *
 * Shutdown plugins.
 */
void rm_plugins_shutdown(void)
{
	GSList *list;

	for (list = rm_plugins; list != NULL; list = list->next) {
		RmPlugin *plugin = list->data;

		if (plugin->enabled) {
			g_debug("%s(): Shuting down %s", __FUNCTION__, plugin->name);
			plugin->enabled = !plugin->shutdown(plugin);
			g_debug("%s(): Shuting down %s done", __FUNCTION__, plugin->name);
		}
	}
}

GSList *rm_plugins_get(void)
{
	return rm_plugins;
}

void rm_plugins_disable(RmPlugin *plugin)
{
	RmProfile *profile = rm_profile_get_active();
	gchar **active_plugins;

	if (plugin->enabled) {
		g_debug("%s(): - %s", __FUNCTION__, plugin->name);
		plugin->enabled = !plugin->shutdown(plugin);
	}

	if (!profile) {
		return;
	}

	active_plugins = g_settings_get_strv(profile->settings, "active-plugins");
	active_plugins = rm_strv_remove(active_plugins, plugin->module_name);

	g_settings_set_strv(profile->settings, "active-plugins", (const gchar * const *) active_plugins);
}

void rm_plugins_enable(RmPlugin *plugin)
{
	RmProfile *profile = rm_profile_get_active();
	gchar **active_plugins;

	if (!plugin->enabled) {
		g_debug("%s(): + %s", __FUNCTION__, plugin->name);
		plugin->enabled = plugin->init(plugin);
	}

	if (!profile) {
		return;
	}

	active_plugins = g_settings_get_strv(profile->settings, "active-plugins");
	active_plugins = rm_strv_add(active_plugins, plugin->module_name);

	g_settings_set_strv(profile->settings, "active-plugins", (const gchar * const *) active_plugins);
}

