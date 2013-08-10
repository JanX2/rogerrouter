/**
 * The libroutermanager project
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

#include <glib.h>

#include <string.h>
#include <sys/types.h>

#ifdef G_OS_UNIX
#include <gio/gio.h>

#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/fax_printer.h>

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
 * \brief Fax spooler callback
 * Monitors the fax spooler directory for changes and if a new USER-xxxx.tif file is created emit fax process signal
 * \param monitor file monitor
 * \param file file structure
 * \param other_file unused file structure
 * \param event_type file monitor event
 * \param user_data unused pointer
 */
static void fax_spooler_changed_cb(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer user_data)
{
	gchar *file_name = g_file_get_path(file);
	const gchar *user_name = g_get_user_name();
	gchar *name = NULL;
	static int type = -1;

	/* Return if there is no valid file or user name */
	if (!file_name || !user_name) {
		goto end;
	}

	/* Sort out invalid files */
	name = g_strdup_printf("%s-", user_name);
	if (has_file_extension(file_name, ".tif") || strncmp(g_path_get_basename(file_name), name, strlen(name))) {
		/* Skip it */
		goto end;
	}

	if (type == -1 && event_type == G_FILE_MONITOR_EVENT_CREATED) {
		type = event_type;
	}

	if (event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
		if (type == G_FILE_MONITOR_EVENT_CREATED) {
			type = event_type;
			g_debug("Print job received on spooler");
			emit_fax_process(file_name);
		}
		type = -1;
	}

end:
	g_free(name);
	g_free(file_name);
}

/**
 * \brief Initialize new printer spool queue
 */
void fax_printer_init(void)
{
	GFileMonitor *file_monitor;
	GFile *file;
	GDir *dir;
	gchar *dir_name = g_strdup_printf("/var/spool/roger");

	/* Check if spooler is present, create directory if needed */
	dir = g_dir_open(dir_name, 0, NULL);
	if (!dir) {
		g_free(dir_name);
		g_warning("Spooler directory not present!");
		return;
	}
	g_dir_close(dir);

#ifdef FAX_SPOOLER_DEBUG
	g_debug("Setting file monitor to '%s'", dir_name);
#endif

	/* Create GFile for GFileMonitor */
	file = g_file_new_for_path(dir_name);
	/* Create file monitor for spool directory */
	file_monitor = g_file_monitor_directory(file, 0, NULL, NULL);
	/* Set callback for file monitor */
	g_signal_connect(file_monitor, "changed", G_CALLBACK(fax_spooler_changed_cb), NULL);

	g_free(dir_name);
}

#endif
