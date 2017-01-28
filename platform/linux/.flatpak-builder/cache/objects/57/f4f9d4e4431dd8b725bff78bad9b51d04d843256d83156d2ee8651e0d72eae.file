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

#ifndef __RM_STRING_H_
#define __RM_STRING_H_

#include <string.h>

G_BEGIN_DECLS

#ifndef RM_EMPTY_STRING
/** Convenient function to check for an empty string (either NULL or with a len of 0) */
#define RM_EMPTY_STRING(x) (!(x) || !strlen(x))
#endif

gchar *rm_strcasestr(const gchar *haystack, const gchar *needle);
gchar *rm_convert_utf8(const gchar *text, gssize len);
gboolean rm_strv_contains(const gchar * const *strv, const gchar *str);

gchar **rm_strv_add(gchar **strv, const gchar *str);
gchar **rm_strv_remove(gchar **strv, const gchar *str);

G_END_DECLS

#endif
