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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

//#define USE_KEYFILE 1

#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include <rm/rmprofile.h>

/**
 * SECTION:rmsettings
 * @title: RmSettings
 * @short_description: Settings wrapper (either uses default GSettings backend or keyfile backend depending on compilation)
 * @stability: Stable
 *
 * Settings keeps track of application/user preferences and stores them in the selected backend.
 */

#ifdef USE_KEYFILE
/**
 * rm_settings_replace_dots:
 * @str: input string
 *
 * Replaces every '.' char with G_DIR_SEPARATOR.
 *
 * Returns: modified input string
 */
static void rm_settings_replace_dots(gchar *str) {
	while (*str) {
		if (*str == '.') {
			*str = G_DIR_SEPARATOR;
		}
		++str;
	}
}

/**
 * rm_settings_remove_prefix:
 * @str: input string
 *
 * Remove leading prefix /a/b/c/
 *
 * Returns: modified input string
 */
static void rm_settings_remove_prefix(gchar *str) {
	gchar *pos;
	gchar *end;

	pos = strchr(str, '/');
	g_assert(pos != NULL);

	end = strchr(pos + 1, '/');

	if (end) {
		gint len = strlen(end + 1);
		strncpy(str, end + 1, len);

		str[len] = '\0';
	} else {
		*str = '\0';
	}
}
#endif

/**
 * rm_settings_new:
 * @scheme: scheme name
 *
 * Creates new #GSettings configuration (either keyfile based, or system based (default)).
 *
 * Returns: newly create gsettings
 */
GSettings *rm_settings_new(gchar *scheme)
{
	GSettings *settings = NULL;

#ifdef USE_KEYFILE
	GSettingsBackend *keyfile;
	gchar *filename;
	gchar *file = g_strdup(scheme);

	rm_settings_replace_dots(file);
	rm_settings_remove_prefix(file);

	filename = g_build_filename(g_get_user_config_dir(), G_DIR_SEPARATOR_S, file, G_DIR_SEPARATOR_S, "config", NULL);

	keyfile = g_keyfile_settings_backend_new(filename, "/", NULL);
	settings = g_settings_new_with_backend(scheme, keyfile);
	g_object_unref(keyfile);

	g_free(file);
	g_free(filename);
#else
	settings = g_settings_new(scheme);
#endif

	return settings;
}

/**
 * rm_settings_new_with_path:
 * @scheme: scheme name
 * @settings_path: settings path name
 *
 * Creates new #GSettings configuration with settings path (either keyfile based, or system based (default)).
 *
 * Returns: newly create gsettings
 */
GSettings *rm_settings_new_with_path(gchar *scheme, gchar *settings_path)
{
	GSettings *settings = NULL;

#ifdef USE_KEYFILE
	GSettingsBackend *keyfile;
	gchar *filename;
	gchar *file = g_strdup(settings_path + 1);

	rm_settings_remove_prefix(file);

	filename = g_build_filename(g_get_user_config_dir(), G_DIR_SEPARATOR_S, file, G_DIR_SEPARATOR_S, "config", NULL);
	keyfile = g_keyfile_settings_backend_new(filename, "/", NULL);
	settings = g_settings_new_with_backend_and_path(scheme, keyfile, settings_path);
	g_object_unref(keyfile);

	g_free(file);
	g_free(filename);
#else
	settings = g_settings_new_with_path(scheme, settings_path);
#endif

	return settings;
}

/**
 * rm_settings_new_profile:
 * @scheme: scheme name
 * @name: settings name
 *
 * Creates new #GSettings configuration with a profile specfic settings path (either keyfile based, or system based (default)).
 *
 * Returns: newly create gsettings
 */
GSettings *rm_settings_new_profile(gchar *scheme, gchar *name, gchar *profile_name)
{
	GSettings *settings;
	gchar *settings_path;

	settings_path = g_strconcat("/org/tabos/rm/", profile_name, "/", name, "/", NULL);

	settings = rm_settings_new_with_path(scheme, settings_path);
	g_free(settings_path);

	return settings;
}

/**
 * rm_settings_backend_is_dconf:
 *
 * Checks whether backend is dconf.
 *
 * Returns: %TRUE if dconf settings is used, otherwise %FALSE
 */
gboolean rm_settings_backend_is_dconf(void)
{
	GSettingsBackend *backend;
	gboolean res;

	backend = g_settings_backend_get_default();

	res = g_str_equal(G_OBJECT_TYPE_NAME(backend), "DConfSettingsBackend");
	g_object_unref(backend);

	return res;
}
