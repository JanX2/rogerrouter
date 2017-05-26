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

#include <rmconfig.h>

#include <string.h>

#include <glib.h>

#include <rm/rmmain.h>
#include <rm/rmfilter.h>
#include <rm/rmprofile.h>
#include <rm/rmrouter.h>
#include <rm/rmplugins.h>
#include <rm/rmobject.h>
#include <rm/rmobjectemit.h>
#include <rm/rmlog.h>
#include <rm/rmnetwork.h>
#include <rm/rmnetmonitor.h>
#include <rm/rmphone.h>
#include <rm/rmfaxspooler.h>
#include <rm/rmfaxserver.h>
#include <rm/rmaction.h>
#include <rm/rmpassword.h>
#include <rm/rmlog.h>

#ifdef __APPLE__
#include <gtkmacintegration/gtkosxapplication.h>
#endif

/**
 * SECTION:rmmain
 * @Title: RmMain
 * @Short_description: Main entry of rm.
 *
 * Initialize rm: e.g. logging, profiles, routers, plugins, actions, ...
 */

/** Private data pointing to the plugin directory */
static gchar *rm_plugin_dir = NULL;
/** Private data pointing to the config directory */
static gchar *rm_user_config_dir = NULL;
/** Private data pointing to the cache directory */
static gchar *rm_user_cache_dir = NULL;
/** Private data pointing to the data directory */
static gchar *rm_user_data_dir = NULL;
/** Requested user profile */
static gchar *rm_requested_profile = NULL;
/** Fax server flag */
static gboolean rm_fax_server_active = FALSE;

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
	gchar *ret = NULL;

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

	ret = tmp;
#elif __APPLE__
	gchar *bundle = gtkosx_application_get_bundle_path();

	if (gtkosx_application_get_bundle_id()) {
		ret = g_strdup_printf("%s/Contents/Resources/%s", bundle, type);
	} else {
		ret = g_strdup_printf("%s/../%s", bundle, type);
	}
#else
	ret = g_strdup(type);
#endif

	return ret;
}

/**
 * rm_init_directory_paths:
 *
 * Initialize directory paths.
 */
void rm_init_directory_paths(void)
{
	rm_plugin_dir = rm_get_directory(RM_PLUGINS);
	rm_user_config_dir = g_build_filename(g_get_user_config_dir(), "rm", NULL);
	rm_user_cache_dir = g_build_filename(g_get_user_cache_dir(), "rm", NULL);
	rm_user_data_dir = g_build_filename(g_get_user_data_dir(), "rm", NULL);
}

/**
 * rm_get_plugin_dir:
 *
 * Return plugin directory.
 *
 * Returns: plugin directory string
 */
gchar *rm_get_plugin_dir(void)
{
	return rm_plugin_dir;
}

/**
 * rm_get_user_config_dir:
 *
 * Return user config directory.
 *
 * Returns: user config directory string
 */
gchar *rm_get_user_config_dir(void)
{
	return rm_user_config_dir;
}

/**
 * rm_get_user_cache_dir:
 *
 * Return user cache directory.
 *
 * Returns: user cache directory string
 */
gchar *rm_get_user_cache_dir(void)
{
	return rm_user_cache_dir;
}

/**
 * rm_get_user_data_dir:
 *
 * Return user data directory.
 *
 * Returns: user data directory string
 */
gchar *rm_get_user_data_dir(void)
{
	return rm_user_data_dir;
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
 * rm_get_requested_profile:
 *
 * Get requested profile.
 *
 * Returns: requested profile name
 */
gchar *rm_get_requested_profile(void)
{
	return rm_requested_profile;
}

/**
 * rm_new:
 * @debug: enable debug output
 * @error: a #GError
 *
 * Create rm.
 *
 * Returns: %TRUE on success, %FALSE on error
 */
gboolean rm_new(gboolean debug, GError **error)
{
	/* Init directory path */
	rm_init_directory_paths();

	bindtextdomain(RM_GETTEXT_PACKAGE, rm_get_directory(APP_LOCALE));
	bind_textdomain_codeset(RM_GETTEXT_PACKAGE, "UTF-8");

	/* Initialize logging system */
	rm_log_init();
	rm_log_set_debug(debug);

	/* Create rm data & cache & config directory */
	g_mkdir_with_parents(rm_get_user_config_dir(), 0700);
	g_mkdir_with_parents(rm_get_user_cache_dir(), 0700);
	g_mkdir_with_parents(rm_get_user_data_dir(), 0700);

	/* Create main app object (signals) */
	rm_object = rm_object_new();
	g_assert(rm_object != NULL);

	return TRUE;
}

/**
 * rm_use_fax_server:
 * @on: flag indicating if fax server should be used
 *
 * Activate/Deactivate fax server
 */
void rm_use_fax_server(gboolean on)
{
	rm_fax_server_active = on;
}

/**
 * rm_init:
 * @error: a @GError
 *
 * Initialize rm.
 *
 * Returns: %TRUE on success, %FALSE on error
 */
gboolean rm_init(GError **error)
{
	/* Say hello */
	g_info("%s %s", RM_NAME, RM_VERSION);

	/* Init fax printer */
	if (rm_fax_server_active) {
		rm_faxserver_init();
	} else {
		rm_faxspooler_init();
	}

	/* Initialize network */
	rm_network_init();

	/* Load plugins depending on ui (router, audio, address book, reverse lookup...) */
	rm_plugins_add_search_path(rm_get_plugin_dir());

	/* Initialize plugins */
	rm_plugins_init();

	/* Check password manager */
	if (!rm_password_get_plugins()) {
		g_set_error(error, RM_ERROR, RM_ERROR_ROUTER, "%s", "No password manager plugins active");
		return FALSE;
	}

	/* Initialize router */
	if (!rm_router_init()) {
		g_set_error(error, RM_ERROR, RM_ERROR_ROUTER, "%s", "Failed to initialize router");
		return FALSE;
	}

	/* Initialize profiles */
	rm_profile_init();

	/* Initialize network monitor */
	rm_netmonitor_init();

	/* Initialize notifications */
	rm_notification_init();

	return TRUE;
}

/**
 * rm_shutdown:
 *
 * Shutdown rm
 * - Network monitor
 * - Profile
 * - Router
 * - Plugins
 * - Network
 * - AppObject
 * - Log
 */
void rm_shutdown(void)
{
	/* Shutdown notifications */
	rm_notification_shutdown();

	/* Shutdown network monitor */
	rm_netmonitor_shutdown();

	/* Shutdown profiles */
	rm_profile_shutdown();

	/* Shutdown router */
	rm_router_shutdown();

	/* Shutdown plugins */
	rm_plugins_shutdown();

	/* Shutdown network */
	rm_network_shutdown();

	/* Destroy rm_object */
	g_clear_object(&rm_object);

	/* Shutdown logging */
	rm_log_shutdown();
}
