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

#if defined(G_OS_UNIX) && !defined(__APPLE__)
#include <gio/gio.h>

#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/fax_printer.h>

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
	{ -1, "" }
};

static const char *event_to_text(GFileMonitorEvent event)
{
	int index;

	if (event == -1) {
		return "empty";
	}

	for (index = 0; event_translation_table[index].event != -1; index++) {
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

	/* Sort out invalid files */
	if (has_file_extension(file_name, ".tmp") || has_file_extension(file_name, ".tif")) {
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
static gboolean fax_setup_file_monitor(GError **error)
{
	GFileMonitor *file_monitor = NULL;
	GFile *file = NULL;
	GDir *user_spool_dir = NULL;
	gboolean ret;
	const gchar *user_name = g_get_user_name();
	g_assert(user_name != NULL);
	// const gchar *new_file = NULL;
#ifdef HAVE_CUPS_BACKEND
	gchar *dir_name =  g_strdup_printf("%s/%s", SPOOLER_DIR, user_name);
#else
	gchar *dir_name =  g_strdup(SPOOLER_DIR);
#endif

#ifdef FAX_SPOOLER_DEBUG
	g_debug("Setting file monitor to '%s'", dir_name);
#endif
	user_spool_dir = g_dir_open(dir_name, 0, error);
	/* fax all pre-existing faxfiles */
	//while ( (new_file = g_dir_read_name(user_spool_dir)) != NULL ) {
	//	/* Sort out invalid files */
	//	if ( (! has_file_extension(new_file, ".tmp")) && (! has_file_extension(new_file, ".tif"))) {
	//		g_debug("Print job received on spooler");
	//		emit_fax_process(new_file);
	//	}
	//}
	g_dir_close(user_spool_dir);

	/* Create GFile for GFileMonitor */
	file = g_file_new_for_path(dir_name);
	/* Create file monitor for spool directory */
	file_monitor = g_file_monitor_directory(file, 0, NULL, error);
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

	g_free(dir_name);
	return ret;
}

/**
 * \brief Fax spooler callback for the common spool directory
 * Monitors the common fax spooler directory for changes and if a spool directory for this user appears, start a file monitor for it
 * \param monitor file monitor
 * \param file file structure
 * \param other_file unused file structure
 * \param event_type file monitor event
 * \param user_data unused pointer
 */
static void fax_spooler_new_dir_cb(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer user_data)
{
	gchar *file_name = NULL;
	const gchar *user_name = NULL;
	GError *file_error;
	gchar *dir_basename;

	g_debug("fax_spooler_new_dir_cb(): event type: %d", event_type);
#ifdef SPOOLER_DEBUG
	g_debug("=> %s", event_to_text(event_type));
#endif
	if (event_type != G_FILE_MONITOR_EVENT_CREATED) {
		return;
	}

	user_name = g_get_user_name();
	if (user_name == NULL) {
		g_debug("Cannot get username!\n");
		return;
	}

	file_name = g_file_get_path(file);
	g_assert(file_name != NULL);
	dir_basename = g_path_get_basename(file_name);
	if (g_strcmp0(dir_basename, user_name) == 0) {
		g_file_monitor_cancel(monitor);
		fax_setup_file_monitor(&file_error);
	}
	g_free(file_name);
	g_free(dir_basename);
}

/**
 * \brief Initialize new printer spool queue
 */
gboolean fax_printer_init(GError **error)
{
	GFileMonitor *file_monitor;
	GFile *file;
	GDir *dir;
	gchar *dir_name = g_strdup_printf(SPOOLER_DIR);
	gchar *user_dir_name = NULL;
	GError *file_error = NULL;
	gboolean ret = FALSE;

	/* Check if spooler is present */
	if (!g_file_test(dir_name, G_FILE_TEST_IS_DIR)) {
		g_debug("Spooler directory %s does not exist!", dir_name);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "Spooler directory %s does not exists!", dir_name);
		g_free(dir_name);
		return FALSE;
	}

	dir = g_dir_open(dir_name, 0, &file_error);
	if (!dir) {
		g_debug("Could not access spooler directory. Is user in group fax?\n%s", file_error ? file_error->message : "");
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "Could not access spooler directory. Is user in group fax?\n%s", file_error ? file_error->message : "");
		g_free(dir_name);
		return FALSE;
	}
	g_dir_close(dir);

	user_dir_name = g_strdup_printf("%s/%s", dir_name, g_get_user_name());

	/* Check if users spooler dir is present */
	if (g_file_test(user_dir_name, G_FILE_TEST_IS_DIR)) {
		ret = fax_setup_file_monitor(&file_error);
		g_free(dir_name);
		g_free(user_dir_name);
		return ret;
	}

	/* user spool dir does not exist, wait for it to appear */

#ifdef FAX_SPOOLER_DEBUG
	g_debug("Setting file monitor to '%s'", dir_name);
#endif

	/* Create GFile for GFileMonitor */
	file = g_file_new_for_path(dir_name);
	/* Create file monitor for spool directory */
	file_monitor = g_file_monitor_directory(file, 0, NULL, &file_error);
	if (file_monitor) {
		/* Set callback for file monitor */
		g_signal_connect(file_monitor, "changed", G_CALLBACK(fax_spooler_new_dir_cb), NULL);
		ret = TRUE;
	} else {
		g_debug("Error occured creating file monitor\n");
		g_debug("Message: %s\n", file_error->message);
		g_set_error(error, RM_ERROR, RM_ERROR_FAX, "%s", file_error->message);
		ret = FALSE;
	}

	g_free(dir_name);

	return ret;
}

#endif
