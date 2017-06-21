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

#include <glib.h>
#include <gio/gio.h>

#include <rm/rmosdep.h>

/**
 * SECTION:rmosdep
 * @title: RmOsDep
 * @short_description: OS dependent functions which are not handled by glib
 * @stability: Stable
 *
 * Covers os execute function for opening urls or other apps with their default applications depending on the platform.
 */

/**
 * rm_os_execute:
 * @uri: uri to open
 *
 * Execute URI (e.g. opens web browser for specific url)
 */
void rm_os_execute(const gchar *uri)
{
#if 0
	gchar *exec;

	/* create execution command line for g_spawn */
	exec = g_strdup_printf("%s %s", RM_OS_OPEN, uri);

#ifdef G_OS_WIN32
	ShellExecute(0, "open", uri, 0, 0, SW_SHOW);
#else
	g_spawn_command_line_async(exec, NULL);
#endif

	/* free command line */
	g_free(exec);
#else
	GError *error = NULL;
	GFile *file = g_file_new_for_path(uri);
	GAppInfo *info = g_file_query_default_handler(file, NULL, &error);
	GList *list = g_list_append(NULL, file);

	if (!info) {
		g_warning("%s(): %s", __FUNCTION__, error->message);
		g_object_unref(file);
		return;
	}

	g_debug("%s(): %s/%s", __FUNCTION__, g_app_info_get_display_name(info), g_app_info_get_executable(info));
	if (!g_app_info_launch(info, list, NULL, &error)) {
		g_warning("%s(): %s", __FUNCTION__, error->message);

	}

	g_list_free(list);
	g_object_unref(file);
	g_object_unref(info);
#endif
}
