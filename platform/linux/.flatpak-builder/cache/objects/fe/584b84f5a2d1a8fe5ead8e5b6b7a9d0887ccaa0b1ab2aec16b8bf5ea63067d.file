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
	rm_file_save(file, data, len);

	g_free(file);
}

/**
 * rm_log_func:
 * @log_domain logging domain
 * @log_level: #GLogLevelFlags
 * @message: output message
 * @user_data: user data poiner (UNUSED)
 *
 * Logging function - prints log output to shell.
 */
static void rm_log_func(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	GDateTime *datetime = g_date_time_new_now_local();
	GString *output;
	gchar *time = g_date_time_format(datetime, "%H:%M:%S");
	gsize len;

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

/**
 * rm_log_init:
 * @debug: flag to toggle debug on/off
 *
 * Initialize logging (set default log handler).
 */
void rm_log_init(gboolean debug)
{
	GFile *file;
	gchar *filename;

	if (!debug) {
		return;
	}

	filename = g_build_filename(rm_get_user_cache_dir(), "debug.log", NULL);

	gchar *dirname = g_path_get_dirname(filename);
	g_mkdir_with_parents(dirname, 0700);
	g_free(dirname);

	g_unlink(filename);

	file = g_file_new_for_path(filename);
	g_free(filename);

	rm_file_stream = g_file_create(file, G_FILE_CREATE_NONE, NULL, NULL);

	g_setenv("G_MESSAGE_DEBUG", "all", TRUE);
	g_log_set_default_handler(rm_log_func, NULL);
}

/**
 * rm_log_shutdown:
 * \brief Shutdown logging
 */
void rm_log_shutdown(void)
{
	g_debug("%s(): Shutdown logging", __FUNCTION__);

	if (rm_file_stream) {
		g_output_stream_close(G_OUTPUT_STREAM(rm_file_stream), NULL, NULL);
	}

	g_log_set_default_handler(NULL, NULL);
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
