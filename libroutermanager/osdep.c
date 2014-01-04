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

#ifdef G_OS_WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include <libroutermanager/osdep.h>

/**
 * \brief Execute URI
 * \param uri uri name
 */
void os_execute(const gchar *uri)
{
	gchar *exec;

	/* create execution command line for g_spawn */
	exec = g_strdup_printf("%s %s", OS_OPEN, uri);

#ifdef G_OS_WIN32
	ShellExecute(0, "open", uri, 0, 0, SW_SHOW);
#else
	g_spawn_command_line_async(exec, NULL);
#endif

	/* free command line */
	g_free(exec);
}
