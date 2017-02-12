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

#ifndef __RM_OSDEP_H
#define __RM_OSDEP_H

#ifdef G_OS_WIN32
#include <windows.h>
#include <shellapi.h>

#define RM_OS_OPEN "start"
#elif __APPLE__
#define RM_OS_OPEN "open"
#else
#define RM_OS_OPEN "xdg-open"
#endif

G_BEGIN_DECLS

void rm_os_execute(const gchar *uri);

G_END_DECLS

#endif
