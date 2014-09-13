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

#define USE_KEYFILE 1

#include <glib.h>
#include <gio/gio.h>

#ifdef USE_KEYFILE
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>
#endif

#include <libroutermanager/settings.h>
#include <libroutermanager/routermanager.h>

/**
 * \brief Create new gsettings configuration (either keyfile based, or system based)
 * \param scheme scheme name
 * \param file filename (used in keyfile case)
 * \return newly create gsettings
 */
GSettings *rm_settings_new(gchar *scheme, gchar *file)
{
	GSettings *settings = NULL;

#ifdef USE_KEYFILE
	GSettingsBackend *keyfile;
	gchar *filename;

	filename = g_build_filename(g_get_user_config_dir(), "routermanager", file, NULL);
	keyfile = g_keyfile_settings_backend_new(filename, ROUTERMANAGER_PATH, "General");
	settings = g_settings_new_with_backend(scheme, keyfile);
	g_free(filename);
#else
	settings = g_settings_new(scheme);
#endif

	return settings;
}

/**
 * \brief Create new gsettings configuration (either keyfile based, or system based)
 * \param scheme scheme name
 * \param path path name
 * \param file filename (used in keyfile case)
 * \return newly create gsettings
 */
GSettings *rm_settings_new_with_path(gchar *scheme, gchar *path, gchar *file)
{
	GSettings *settings = NULL;

#ifdef USE_KEYFILE
	GSettingsBackend *keyfile;
	gchar *filename;

	filename = g_build_filename(g_get_user_config_dir(), "routermanager", file, NULL);
	keyfile = g_keyfile_settings_backend_new(filename, ROUTERMANAGER_PATH, NULL);
	settings = g_settings_new_with_backend_and_path(scheme, keyfile, path);
	g_free(filename);
#else
	settings = g_settings_new_with_path(scheme, path);
#endif

	return settings;
}
