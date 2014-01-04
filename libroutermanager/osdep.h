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

#ifndef LIBROUTERMANAGER_OSDEP_H
#define LIBROUTERMANAGER_OSDEP_H
#ifdef G_OS_WIN32
#define OS_OPEN "start"
#define APP_USER_DIR APP_NAME
#endif

#ifdef G_OS_UNIX
#ifdef __MACOSX__
#define OS_OPEN "open"
#define APP_USER_DIR "." APP_NAME
/* work around for warnings where gtk-mac-integration expects a label to have an accel closure */
/* we simply use an accel label instead of the normal label */
#define gtk_label_new(text) gtk_accel_label_new(text)
#else
#define OS_OPEN "xdg-open"
#define APP_USER_DIR ".config/" APP_NAME
#endif
#endif

G_BEGIN_DECLS

void os_execute(const gchar *uri);

G_END_DECLS

#endif
