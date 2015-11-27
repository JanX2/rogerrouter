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

#include <string.h>
#include <sys/types.h>

#include <gio/gio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/fax_printer.h>

#ifdef USE_PRINTER_SPOOLER
#define SPOOLER_DIR "/var/spool/roger"

#define SPOOLER_DEBUG 1

#ifdef SPOOLER_DEBUG
/** translations from event to text for file monitor events */
struct event_translate {
	GFileMonitorEvent event;
	gchar *text;
};

/** text values of file monitor events */
struct event_translate event_translation_table[] = {
	{G_FILE_MONITOR_EVENT_CHANGED , "file changed"},
	{G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT, "file changes finished"},
	{G_FILE_MONITOR_EVENT_DELETED, "file deleted"},
	{G_FILE_MONITOR_EVENT_CREATED, "file created"},
	{G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED, "file attrbutes changed"},
	{G_FILE_MONITOR_EVENT_PRE_UNMOUNT, "file system about to be unmounted"},
	{G_FILE_MONITOR_EVENT_UNMOUNTED, "file system unmounted"},
	{G_FILE_MONITOR_EVENT_MOVED, "file moved"},
};

static const char *event_to_text(GFileMonitorEvent event)
{
	int index;

	for (index = 0; index < sizeof(event_translation_table) / sizeof(event_translation_table[0]); index++) {
		if (event_translation_table[index].event == event) {
			return event_translation_table[index].text;
		}
	}

	return "Undefined event";
}
#endif

/**
 * \brief Check if given file name ends with the requested extension
 * \param file file name
 * \param ext file extension
 * \return TRUE if file ends with given extension or FALSE if not
 */
gboolean has_file_extension(const char *file, const char *ext)
{
	int len, ext_len;

	if (!file || *file == '\0' || !ext) {
		return FALSE;
	}

	ext_len = strlen(ext);
	len = strlen(file) - ext_len;

	if (len < 0) {
		return FALSE;
	}

	return (strncmp(file + len, ext, ext_len) == 0);
}

/**
 * \brief Fax spooler callback for the users own spool directory
 * Monitors the users fax spooler directory for changes and if a new final file is created emit fax process signal
 * \param monitor file monitor
 * \param file file structure
 * \param other_file unused file structure
 * \param event_type file monitor event
 * \param user_data unused pointer
 */
static void fax_spooler_new_file_cb(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer user_data)
{
	GFileType type;
	gchar *file_name = NULL;

	g_debug("fax_spooler_new_file_cb(): event type: %d", event_type);
#ifdef SPOOLER_DEBUG
	g_debug("=> %s", event_to_text(event_type));
#endif
	if (event_type != G_FILE_MONITOR_EVENT_CREATED) {
		return;
	}

	file_name = g_file_get_path(file);
	g_assert(file_name != NULL);

	type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
	g_debug("%s(): type: %d", __FUNCTION__, type);
	if (type != G_FILE_TYPE_REGULAR) {
		return;
	}

	/* Sort out invalid files */
	if (has_file_extension(file_name, ".tmp") || has_file_extension(file_name, ".tif") || has_file_extension(file_name, ".sff")) {
		/* Skip it */
		goto end;
	}

	g_debug("Print job received on spooler");
	emit_fax_process(file_name);

end:
	g_free(file_name);
}

/**
 * Create file monitor for the users own spooler directory
 *
 */
static gboolean fax_setup_file_monitor(const gchar *dir_name, GError **error)
{
	GFileMonitor *file_monitor = NULL;
	GFile *file = NULL;
	gboolean ret;
	const gchar *user_name = g_get_user_name();
	g_assert(user_name != NULL);

#ifdef SPOOLER_DEBUG
	g_debug("Setting file monitor to '%s'", dir_name);
#endif

	/* Create GFile for GFileMonitor */
	file = g_file_new_for_path(dir_name);
	/* Create file monitor for spool directory */
	file_monitor = g_file_monitor_directory(file, 0, NULL, error);
	g_object_unref(file);
	if (file_monitor) {
		/* Set callback for file monitor */
		g_signal_connect(file_monitor, "changed", G_CALLBACK(fax_spooler_new_file_cb), NULL);
		ret = TRUE;
	} else {
		g_debug("Error occured creating new file monitor\n");
		g_debug("Message: %s\n", (*error)->message);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "Spooler directory %s does not exists!", dir_name);
		ret = FALSE;
	}
	return ret;
}

/**
 * \brief Initialize new printer spool queue
 */
gboolean fax_printer_init(GError **error)
{
	GDir *dir;
	GError *file_error = NULL;

	/* Check if spooler is present */
	if (!g_file_test(SPOOLER_DIR, G_FILE_TEST_IS_DIR)) {
		g_debug("Spooler directory %s does not exist!", SPOOLER_DIR);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "Spooler directory %s does not exists!", SPOOLER_DIR);
		return FALSE;
	}

	dir = g_dir_open(SPOOLER_DIR,  0, &file_error);
	if (!dir) {
		g_debug("Could not access spooler directory. Is user in group fax?\n%s", file_error ? file_error->message : "");
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "Could not access spooler directory. Is user in group fax?\n%s", file_error ? file_error->message : "");
		return FALSE;
	}
	g_dir_close(dir);

	return fax_setup_file_monitor(SPOOLER_DIR, &file_error);
}
#endif
