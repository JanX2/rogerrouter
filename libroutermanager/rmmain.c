/**
 * The libroutermanager project
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#include <string.h>

#include <glib.h>

#include <libroutermanager/rmmain.h>
#include <libroutermanager/filter.h>
#include <libroutermanager/rmprofile.h>
#include <libroutermanager/router.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/logging.h>
#include <libroutermanager/network.h>
#include <libroutermanager/net_monitor.h>
#include <libroutermanager/rmphone.h>
#include <libroutermanager/fax_printer.h>
#include <libroutermanager/rmaction.h>
#include <libroutermanager/password.h>

#ifdef __APPLE__
#include <gtkmacintegration/gtkosxapplication.h>
#endif

/**
 * SECTION:rmmain
 * @Title: RmMain
 * @Short_description: Main entry of libroutermanager.
 *
 * Initialize libroutermanager: e.g. logging, profiles, routers, plugins, actions, ...
 */

/** Private data pointing to the plugin directory */
static gchar *rm_plugin_dir = NULL;
/** Requested user profile */
static gchar *rm_requested_profile = NULL;

/**
 * rm_print_error_quark:
 *
 * Print error quark.
 *
 * Returns: a @GQuark
 */
GQuark rm_print_error_quark(void)
{
	return g_quark_from_static_string("rm-print-error-quark");
}

/**
 * rm_get_directory:
 * @type: directory type name
 *
 * Get directory of requested type (based on operating system).
 *
 * Returns: directory path, free it with g_free() when done.
 */
gchar *rm_get_directory(gchar *type)
{
#ifdef G_OS_WIN32
	GFile *directory;
	GFile *child;
	gchar *tmp;

	tmp = g_win32_get_package_installation_directory_of_module(NULL);

	directory = g_file_new_for_path(tmp);
	g_free(tmp);

	child = g_file_get_child(directory, type);
	g_object_unref(directory);

	directory = child;

	tmp = g_file_get_path(directory);
	g_object_unref(directory);

	return tmp;
#elif __APPLE__
	gchar *bundle = gtkosx_application_get_bundle_path();

	if (gtkosx_application_get_bundle_id()) {
		return g_strdup_printf("%s/Contents/Resources/%s", bundle, type);
	} else {
		return g_strdup_printf("%s/../%s", bundle, type);
	}
#else
	return g_strdup(type);
#endif
}

/**
 * rm_init_directory_paths:
 *
 * Initialize directory paths.
 */
void rm_init_directory_paths(void)
{
	rm_plugin_dir = rm_get_directory(RM_PLUGINS);
}

/**
 * rm_get_plugin_dir:
 *
 * Return plugin directory.
 *
 * Returns: plugin directory string
 */
static gchar *rm_get_plugin_dir(void)
{
	g_debug("plugin_dir = %s", rm_plugin_dir);
	return rm_plugin_dir;
}

/**
 * rm_set_requested_profile:
 * @name: requested profile name
 *
 * Set requested profile name.
 */
void rm_set_requested_profile(gchar *name)
{
	if (rm_requested_profile) {
		g_free(rm_requested_profile);
	}

	rm_requested_profile = g_strdup(name);
}

/**
 * \brief Get requested profile
 * \return requested profile name
 */
gchar *rm_get_requested_profile(void)
{
	return rm_requested_profile;
}

/**
 * \brief Create routermanager
 * \param debug enable debug output
 * \return TRUE on success, FALSE on error
 */
gboolean rm_new(gboolean debug, GError **error)
{
	gchar *dir;

#if !GLIB_CHECK_VERSION(2, 36, 0)
	/* Init g_type */
	g_type_init();
#endif

	/* Init directory path */
	rm_init_directory_paths();

	/* Initialize logging system */
	log_init(debug);

	/* Say hello */
	g_debug("%s %s", PACKAGE_NAME, PACKAGE_VERSION);

	/* Create routermanager data & cache directory */
	dir = g_build_filename(g_get_user_data_dir(), "routermanager", NULL);
	g_mkdir_with_parents(dir, 0700);
	g_free(dir);

	dir = g_build_filename(g_get_user_cache_dir(), "routermanager", NULL);
	g_mkdir_with_parents(dir, 0700);
	g_free(dir);

	/* Create main app object (signals) */
	app_object = app_object_new();
	g_assert(app_object != NULL);

	return TRUE;
}

/**
 * rm_init:
 * @error: a @GError
 *
 * Initialize routermanager.
 *
 * Returns: TRUE on success, FALSE on error
 */
gboolean rm_init(GError **error)
{
	/* Init filter */
	filter_init();

	/* Init fax printer */
	if (!fax_printer_init(error)) {
		return FALSE;
	}

	/* Initialize network */
	net_init();

	/* Load plugins depending on ui (router, audio, address book, reverse lookup...) */
	routermanager_plugins_add_search_path(rm_get_plugin_dir());

	/* Initialize plugins */
	plugins_init();

	/* Check password manager */
	if (!password_manager_get_plugins()) {
		g_set_error(error, RM_ERROR, RM_ERROR_ROUTER, "%s", "No password manager plugins active");
		return FALSE;
	}

	/* Initialize router */
	if (!router_init()) {
		g_set_error(error, RM_ERROR, RM_ERROR_ROUTER, "%s", "Failed to initialize router");
		return FALSE;
	}

	/* Initialize profiles */
	rm_profile_init();

	/* Initialize network monitor */
	net_monitor_init();

	return TRUE;
}

/**
 * rm_shutdown:
 *
 * Shutdown routermanager
 * - Network monitor
 * - Profile
 * - Router
 * - Plugins
 * - Network
 * - Filter
 * - AppObject
 * - Log
 */
void rm_shutdown(void)
{
	/* Shutdown network monitor */
	net_monitor_shutdown();

	/* Shutdown profiles */
	rm_profile_shutdown();

	/* Shutdown router */
	router_shutdown();

	/* Shutdown plugins */
	plugins_shutdown();

	/* Shutdown network */
	net_shutdown();

	/* Shutdown filter */
	filter_shutdown();

	/* Destroy app_object */
	g_clear_object(&app_object);

	/* Shutdown logging */
	log_shutdown();
}
