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

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include <libroutermanager/logging.h>
#include <libroutermanager/file.h>

/** Internal minium log level, starting with DEBUG */
static GLogLevelFlags log_level = G_LOG_LEVEL_DEBUG;

/**
 * \brief Save log data to temp directory (if log level is set to DEBUG)
 * \param name file name
 * \param data data pointer
 * \param len length of data
 */
void log_save_data(gchar *name, const gchar *data, gsize len)
{
	gchar *file;

	if (log_level != G_LOG_LEVEL_DEBUG) {
		return;
	}

	file = g_build_filename(g_get_user_cache_dir(), "routermanager", name, NULL);
	file_save(file, data, len);

	g_free(file);
}

/**
 * \brief Logging function - prints log output to shell
 * \paramm log_domain logging domain
 * \param log_level log level flags
 * \param message output message
 * \param user_data user data poiner (UNUSED)
 */
static void log_func(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	GDateTime *datetime = g_date_time_new_now_local();
	gchar *time = g_date_time_format(datetime, "%H:%M:%S");

	g_printf("%s ", time);
	g_free(time);
	g_date_time_unref(datetime);

	if (log_domain) {
		g_printf("%s:", log_domain);
	}

	switch (log_level) {
	case G_LOG_LEVEL_ERROR:
		g_printf("Error: ");
		break;
	case G_LOG_LEVEL_CRITICAL:
		g_printf("Critical: ");
		break;
	case G_LOG_LEVEL_WARNING:
		g_printf("Warning: ");
		break;
	case G_LOG_LEVEL_DEBUG:
		g_printf("Debug: ");
		break;
	default:
		break;
	}

	g_printf("%s\n", message);
}

/**
 * \brief Initialize logging (set default log handler)
 */
void log_init(gboolean debug)
{
	if (!debug) {
		return;
	}

	g_log_set_default_handler(log_func, NULL);
}

/**
 * \brief Shutdown logging
 */
void log_shutdown(void)
{
	g_debug("Shutdown logging");
	g_log_set_default_handler(NULL, NULL);
}

/**
 * \brief Set minium log level
 * \param new minimum log level
 */
void log_set_level(GLogLevelFlags level)
{
	log_level = level;
}
