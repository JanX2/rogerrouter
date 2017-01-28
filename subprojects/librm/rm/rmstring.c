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

#include <string.h>

#include <glib.h>

#include <rm/rmstring.h>

/**
 * SECTION:rmstring
 * @title: RmString
 * @short_description: String helper functions
 * @stability: Stable
 *
 * Adds string helper functions to simplify the code.
 */

/**
 * rm_strcasestr:
 * @haystack: haystack
 * @needle: needle
 *
 * Search for case-sensitive needle in haystack
 *
 * Returns: pointer to position or %NULL
 */
gchar *rm_strcasestr(const gchar *haystack, const gchar *needle)
{
	size_t n = strlen(needle);

	if (!haystack || !needle) {
		return NULL;
	}

	for (; *haystack; haystack++) {
		if (g_ascii_strncasecmp(haystack, needle, n) == 0) {
			return (gchar *) haystack;
		}
	}

	return NULL;
}

/**
 * rm_convert_utf8:
 * @text: input text string
 * @len: length of string or -1 for #strlen
 *
 * Convert string (if needed) to UTF-8.
 *
 * Returns: input string in UTF-8 (must be freed)
 */
gchar *rm_convert_utf8(const gchar *text, gssize len)
{
	GError *error = NULL;
	gsize read_bytes, written_bytes;
	gchar *str = NULL;
	gssize idx;

	if (!text) {
		g_assert_not_reached();
	}

	if (len == -1) {
		len = strlen(text);
	}

	if (g_utf8_validate(text, len, NULL)) {
		return g_strndup(text, len);
	}

	str = g_convert(text, len, "UTF-8", "ISO-8859-1", &read_bytes, &written_bytes, &error);
	if (str == NULL) {
		str = g_strndup(text, len);

		for (idx = 0; idx < len; idx++) {
			if ((guchar)str[idx] > 128) {
				str[idx] = '?';
			}
		}
	}

	if (error) {
		g_error_free(error);
	}

	return str;
}

/**
 * rm_strv_contains:
 * @strv: a %NULL%-terminated array of strings
 * @str: a string
 *
 * Checks if @strv contains @str
 *
 * Returns: %TRUE if @str is found in @strv, otherwise %FALSE
 */
gboolean rm_strv_contains(const gchar * const *strv, const gchar *str)
{
#if GLIB_CHECK_VERSION(2, 44, 0)
	return g_strv_contains(strv, str);
#else
	g_return_val_if_fail(strv != NULL, FALSE);
	g_return_val_if_fail(str != NULL, FALSE);

	for (; *strv != NULL; strv++) {
		if (g_str_equal(str, *strv)) {
			return TRUE;
		}
	}

	return FALSE;
#endif
}

gchar **rm_strv_add(gchar **strv, const gchar *str)
{
	guint len = g_strv_length(strv);
	gint i;
	gchar **new_strv;

	if (rm_strv_contains((const gchar * const *)strv, str)) {
		return strv;
	}

	new_strv = g_malloc0(sizeof(gchar *) * (len + 2));

	for (i = 0; i < len; i++) {
		new_strv[i] = g_strdup(strv[i]);
	}

	new_strv[i] = g_strdup(str);
	new_strv[i + 1] = NULL;

	return new_strv;
}

gchar **rm_strv_remove(gchar **strv, const gchar *str)
{
	guint len = g_strv_length(strv);
	gint i, cnt = 0;
	gchar **new_strv;

	new_strv = g_malloc0(sizeof(gchar *) * (len + 1));

	for (i = 0; i < len; i++) {
		if (strcmp(strv[i], str)) {
			new_strv[cnt++] = g_strdup(strv[i]);
		}
	}

	new_strv[cnt] = NULL;

	return new_strv;
}
