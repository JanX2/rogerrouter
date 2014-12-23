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

//#define USE_KEYFILE 1

#include <glib.h>
#include <gio/gio.h>

#ifdef USE_KEYFILE
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>
#endif

#include <libroutermanager/settings.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/profile.h>

/**
 * \brief Create new gsettings configuration (either keyfile based, or system based)
 * \param scheme scheme name
 * \param path root path or NULL
 * \param file filename (used in keyfile case)
 * \return newly create gsettings
 */
GSettings *rm_settings_new(gchar *scheme, gchar *root_path, gchar *file)
{
	GSettings *settings = NULL;

#ifdef USE_KEYFILE
	GSettingsBackend *keyfile;
	gchar *filename;

	filename = g_build_filename(g_get_user_config_dir(), "routermanager", file, NULL);
	g_debug("filename: '%s'", filename);
	//keyfile = g_keyfile_settings_backend_new(filename, ROUTERMANAGER_PATH, "General");
	if (!root_path) {
		root_path = "/";
	}
	if (root_path) {
		g_debug("root_path: '%s'", root_path);
	}
	keyfile = g_keyfile_settings_backend_new(filename, root_path, "General");
	g_debug("keyfile: '%p'", keyfile);
	settings = g_settings_new_with_backend(scheme, keyfile);
	g_debug("settings: '%p'", settings);
	g_free(filename);
#else
	settings = g_settings_new(scheme);
#endif

	return settings;
}

/**
 * \brief Create new gsettings configuration (either keyfile based, or system based)
 * \param scheme scheme name
 * \param settings_path settings path name
 * \param file filename (used in keyfile case)
 * \return newly create gsettings
 */
GSettings *rm_settings_new_with_path(gchar *scheme, gchar *settings_path, gchar *file)
{
	GSettings *settings = NULL;

#ifdef USE_KEYFILE
	GSettingsBackend *keyfile;
	gchar *filename;

	filename = g_build_filename(g_get_user_config_dir(), "routermanager/", file, NULL);
	//keyfile = g_keyfile_settings_backend_new(filename, ROUTERMANAGER_PATH, NULL);
	keyfile = g_keyfile_settings_backend_new(filename, "/", NULL);
	settings = g_settings_new_with_backend_and_path(scheme, keyfile, settings_path);
	g_free(filename);
#else
	settings = g_settings_new_with_path(scheme, settings_path);
#endif

	return settings;
}

/**
 * \brief Create new plugin gsettings configuration (either keyfile based, or system based)
 * \param scheme scheme name
 * \param name plugin name
 * \return newly create gsettings
 */
GSettings *rm_settings_plugin_new(gchar *scheme, gchar *name)
{
	GSettings *settings = NULL;
	struct profile *profile = profile_get_active();
	gchar *filename;

	filename = g_strconcat(profile->name, "/plugin-", name, ".conf", NULL);
	settings = rm_settings_new(scheme, NULL, filename);
	g_free(filename);

	return settings;
}
