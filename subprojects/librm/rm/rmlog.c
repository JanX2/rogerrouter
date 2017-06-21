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

#define G_LOG_USE_STRUCTURED 1

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include <rm/rmlog.h>
#include <rm/rmfile.h>
#include <rm/rmmain.h>

static rm_log_func app_log = NULL;
static gboolean debug_state = FALSE;

/**
 * SECTION:rmlog
 * @title: RmLog
 * @short_description: Log output utility
 * @stability: Stable
 *
 * Sets a new log output handler to redirect output to console and debug file for further user assistence.
 */

/** Internal minium log level, starting with DEBUG */
static GLogLevelFlags rm_log_level = G_LOG_LEVEL_DEBUG;
static GFileOutputStream *rm_file_stream = NULL;

/**
 * rm_log_save_data:
 * @name: file name
 * @data: data pointer
 * @len: length of data
 *
 * Save log data to temp directory (only if #rm_log_set_level() is set to #G_LOG_LEVEL_DEBUG).
 */
void rm_log_save_data(gchar *name, const gchar *data, gsize len)
{
	gchar *file;

	if (rm_log_level != G_LOG_LEVEL_DEBUG) {
		return;
	}

	file = g_build_filename(rm_get_user_cache_dir(), name, NULL);
	g_debug("%s(): Writing to '%s'", __FUNCTION__, file);
	rm_file_save(file, data, len);

	g_free(file);
}

/**
 * rm_log_handler:
 * @log_domain logging domain
 * @log_level: #GLogLevelFlags
 * @message: output message
 * @user_data: user data poiner (UNUSED)
 *
 * Logging function - prints log output to shell.
 */
static void rm_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	GDateTime *datetime;
	GString *output;
	gchar *time;
	gsize len;

	if (debug_state) {
		datetime = g_date_time_new_now_local();
		time = g_date_time_format(datetime, "%H:%M:%S");
		output = g_string_new(time);
		g_free(time);
		g_date_time_unref(datetime);

#if GLIB_CHECK_VERSION(2,49,0)
		g_string_append_printf(output, " %s\n", message);

		const GLogField fields[] = {{"MESSAGE", output->str, -1}};
		gchar *fmt_str = g_log_writer_format_fields(log_level, fields, G_N_ELEMENTS(fields), TRUE);

		g_string_free(output, TRUE);

		output = g_string_new("");
		g_string_append_printf(output, "%s", fmt_str);
		g_free(fmt_str);
#else
		if (log_domain) {
			g_string_append_printf(output, " %s:", log_domain);
		}

		switch (log_level) {
		case G_LOG_LEVEL_ERROR:
			g_string_append(output, " Error: ");
			break;
		case G_LOG_LEVEL_CRITICAL:
			g_string_append(output, " Critical: ");
			break;
		case G_LOG_LEVEL_WARNING:
			g_string_append(output, " Warning: ");
			break;
		case G_LOG_LEVEL_DEBUG:
			g_string_append(output, " Debug: ");
			break;
		default:
			break;
		}

		g_string_append_printf(output, "%s\n", message);
#endif

		g_print("%s", output->str);
		g_output_stream_printf(G_OUTPUT_STREAM(rm_file_stream), &len, NULL, NULL, "%s", output->str);

		g_string_free(output, TRUE);
	}

	if (app_log) {
		app_log(log_level, message);
	}
}

/**
 * rm_log_init:
 *
 * Initialize log handler
 */
void rm_log_init(void)
{
	g_log_set_default_handler(rm_log_handler, NULL);
}

/**
 * rm_log_shutdown:
 *
 * Remove log handler
 */
void rm_log_shutdown(void)
{
	g_log_set_default_handler(rm_log_handler, NULL);
}

/**
 * rm_log_set_debug:
 * @state: %TRUE to enable debugging, %FALSE to disable it
 *
 * Set debug log state
 */
void rm_log_set_debug(gboolean state)
{
	GFile *file;
	GError *error = NULL;
	gchar *filename;

	if (state == debug_state) {
		return;
	}

	if (debug_state) {
		g_object_unref(rm_file_stream);
		rm_file_stream = NULL;
	} else {
		filename = g_build_filename(rm_get_user_cache_dir(), "debug.log", NULL);

		gchar *dirname = g_path_get_dirname(filename);
		g_mkdir_with_parents(dirname, 0700);
		g_free(dirname);

		file = g_file_new_for_path(filename);
		g_free(filename);

		rm_file_stream = g_file_replace(file, NULL, TRUE, G_FILE_CREATE_PRIVATE, NULL, &error);
		if (!rm_file_stream) {
			g_warning("%s(): file stream creation failed (%s)!", __FUNCTION__, error->message);
		} else {
			g_debug("%s(): Writing debug log to %s", __FUNCTION__, filename);
		}
	}

	debug_state = state;
}

/**
 * rm_log_set_level:
 * @level: new minimum log level (#GLogLevelFlags)
 *
 * Sets minium log level.
 */
void rm_log_set_level(GLogLevelFlags level)
{
	rm_log_level = level;
}

/**
 * rm_log_set_app_handler:
 * @app_log: application logger
 *
 * Allows application to access debug information
 */
void rm_log_set_app_handler(rm_log_func app_log_func)
{
	app_log = app_log_func;
}

