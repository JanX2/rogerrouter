/**
 * The libcallibre project
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

#include <string.h>

#include <glib.h>

#include <libcallibre/callibre.h>
#include <libcallibre/filter.h>
#include <libcallibre/profile.h>
#include <libcallibre/router.h>
#include <libcallibre/plugins.h>
#include <libcallibre/appobject.h>
#include <libcallibre/appobject-emit.h>
#include <libcallibre/logging.h>
#include <libcallibre/network.h>
#include <libcallibre/net_monitor.h>
#include <libcallibre/fax_phone.h>
#include <libcallibre/fax_printer.h>
#include <libcallibre/action.h>
#include <libcallibre/bluetooth.h>

static gchar *plugin_dir = NULL;

/**
 * \brief Get directory of requested type
 * \param pnType directory type name
 * \return directory as duplicated string
 */
gchar *get_directory(gchar *type)
{
#ifdef G_OS_WIN32
	GFile *directory;
	GFile *child;
	char *tmp = NULL;

	tmp = g_win32_get_package_installation_directory_of_module(NULL);

	directory = g_file_new_for_path(tmp);
	g_free(tmp);

	child = g_file_get_child(directory, type);
	g_object_unref(directory);

	directory = child;

	tmp = g_file_get_path(directory);

	return tmp;
#else
	return g_strdup(type);
#endif
}

/**
 * \brief Initialize directory paths
 */
void init_directory_paths(void)
{
	plugin_dir = get_directory(CALLIBRE_PLUGINS);
}

gchar *get_plugin_dir(void)
{
	return plugin_dir;
}

/**
 * \brief Initialize callibre
 * \param ui_ops ui operations structure
 * \return TRUE on success, FALSE on error
 */
gboolean callibre_init(struct ui_ops *ui_ops, gboolean debug)
{
	gchar *dir;

#if !GLIB_CHECK_VERSION(2, 36, 0)
	/* Init g_type */
	g_type_init();
#endif

	/* Init directory path */
	init_directory_paths();

	/* Initialize logging system */
	log_init(debug);

	/* Say hello */
	g_debug("%s %s", PACKAGE_NAME, PACKAGE_VERSION);

	/* Create callibre directory */
	dir = g_build_filename(g_get_user_data_dir(), "callibre", NULL);
	g_mkdir_with_parents(dir, 0700);
	g_free(dir);

	/* Create main app object (signals) */
	app_object = app_object_new();
	g_assert(app_object != NULL);

	/* Init filter */
	filter_init();

	/* Init fax printer */
	fax_printer_init();

	/* Initialize network */
	net_init();

	/* Load plugins depending on ui (router, audio, address book, reverse lookup...) */
	callibre_plugins_add_search_path(get_plugin_dir());

	/* Initialize plugins */
	plugins_init();

	/* Initialize router */
	if (router_init() == FALSE) {
		return FALSE;
	}

	/* Initialize profiles */
	profile_init(ui_ops);

	/* Initialize network monitor */
	net_monitor_init();

#ifdef HAVE_DBUS
	/* Bluetooth */
	bluetooth_init();
#endif

	return TRUE;
}

/**
 * \brief Shutdown callibre
 * - Network monitor
 * - Profile
 * - Router
 * - Plugins
 * - Network
 * - Filter
 * - AppObject
 * - Log
 */
void callibre_shutdown(void)
{
	/* Shutdown network monitor */
	net_monitor_shutdown();

	/* Shutdown profiles */
	profile_shutdown();

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
