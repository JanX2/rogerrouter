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

#include <glib.h>

#include <string.h>
#include <sys/types.h>

#include <gio/gio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libroutermanager/rmobjectemit.h>
#include <libroutermanager/rmmain.h>
#include <libroutermanager/rmfaxprinter.h>

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
struct event_translate rm_event_translation_table[] = {
	{G_FILE_MONITOR_EVENT_CHANGED , "file changed"},
	{G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT, "file changes finished"},
	{G_FILE_MONITOR_EVENT_DELETED, "file deleted"},
	{G_FILE_MONITOR_EVENT_CREATED, "file created"},
	{G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED, "file attrbutes changed"},
	{G_FILE_MONITOR_EVENT_PRE_UNMOUNT, "file system about to be unmounted"},
	{G_FILE_MONITOR_EVENT_UNMOUNTED, "file system unmounted"},
	{G_FILE_MONITOR_EVENT_MOVED, "file moved"},
};

/**
 * rm_faxspooler_event_to_text:
 * @event: a #GFileMonitorEvent
 *
 * Translates a file monitor event into a human readable string
 *
 * Returns: a newly created event string
 */
static const char *rm_faxspooler_event_to_text(GFileMonitorEvent event)
{
	int index;

	for (index = 0; index < G_N_ELEMENTS(rm_event_translation_table); index++) {
		if (rm_event_translation_table[index].event == event) {
			return rm_event_translation_table[index].text;
		}
	}

	return "Undefined event";
}
#endif

/**
 * rm_faxspooler_has_file_extension:
 * @file: file name
 * @ext: file extension
 *
 * Check if given file name ends with the requested extension.
 *
 * Returns: %TRUE% if file ends with given extension or %FALSE% if not
 */
gboolean rm_faxspooler_has_file_extension(const char *file, const char *ext)
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
 * rm_faxspooler_changed_cb:
 * @monitor: a #GFileMonitor
 * @file: a #GFile
 * @other_file: a #GFile
 * @event_type: a #GFileMonitorEvent
 * @user_data: additional user data
 *
 * Fax spooler callback for the users own spool directory. Monitors the users fax spooler directory
 * for changes and if a new final file is created emit fax process signal.
 */
static void rm_faxspooler_changed_cb(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer user_data)
{
	GFileType type;
	gchar *file_name = NULL;

	g_debug("%s(): event_type: %d", __FUNCTION__, event_type);
#ifdef SPOOLER_DEBUG
	g_debug("%s(): => %s", __FUNCTION__, rm_faxspooler_event_to_text(event_type));
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
	if (rm_faxspooler_has_file_extension(file_name, ".tmp") || rm_faxspooler_has_file_extension(file_name, ".tif") || rm_faxspooler_has_file_extension(file_name, ".sff")) {
		/* Skip it */
		goto end;
	}

	g_debug("%s(): Print job received on spooler", __FUNCTION__);
	rm_object_emit_fax_process(file_name);

end:
	g_free(file_name);
}

/**
 * rm_faxspooler_setup_file_monitor:
 * @dir_name: directory to monitor
 * @error: a #GError
 *
 * Create file monitor for the users own spooler directory.
 *
 * Returns: %TRUE% on successful creation, %FALSE% on error.
 */
static gboolean rm_faxspooler_setup_file_monitor(const gchar *dir_name, GError **error)
{
	GFileMonitor *file_monitor = NULL;
	GFile *file = NULL;
	gboolean ret;
	const gchar *user_name = g_get_user_name();
	g_assert(user_name != NULL);

#ifdef SPOOLER_DEBUG
	g_debug("%s(): Setting file monitor to '%s'", __FUNCTION__, dir_name);
#endif

	/* Create GFile for GFileMonitor */
	file = g_file_new_for_path(dir_name);
	/* Create file monitor for spool directory */
	file_monitor = g_file_monitor_directory(file, 0, NULL, error);
	g_object_unref(file);
	if (file_monitor) {
		/* Set callback for file monitor */
		g_signal_connect(file_monitor, "changed", G_CALLBACK(rm_faxspooler_changed_cb), NULL);
		ret = TRUE;
	} else {
		g_debug("%s(): Error occurred creating new file monitor", __FUNCTION__);
		g_debug("%s(): Message: %s\n", __FUNCTION__, (*error)->message);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "Spooler directory %s does not exists!", dir_name);
		ret = FALSE;
	}
	return ret;
}

/**
 * rm_faxprinter_init:
 * @error: a #GError
 *
 * Initialize new printer spool queue and file monitor
 *
 * Returns: %TRUE on successful creation, %FALSE on error.
 */
gboolean rm_faxprinter_init(GError **error)
{
	GDir *dir;
	GError *file_error = NULL;

	/* Check if spooler is present */
	if (!g_file_test(SPOOLER_DIR, G_FILE_TEST_IS_DIR)) {
		g_warning("%s(): Spooler directory %s does not exist!", __FUNCTION__, SPOOLER_DIR);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, _("Spooler directory %s does not exists!"), SPOOLER_DIR);
		return FALSE;
	}

	dir = g_dir_open(SPOOLER_DIR,  0, &file_error);
	if (!dir) {
		g_warning("%s(): Could not access spooler directory. Is user in group fax?\n%s", __FUNCTION__, file_error ? file_error->message : "");
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, _("Could not access spooler directory. Is user in group fax?\n%s"), file_error ? file_error->message : "");
		return FALSE;
	}
	g_dir_close(dir);

	return rm_faxspooler_setup_file_monitor(SPOOLER_DIR, &file_error);
}
#endif
