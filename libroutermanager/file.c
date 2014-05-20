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

#include <string.h>

#include <gio/gio.h>

#include <libroutermanager/file.h>

/**
 * \brief Save data to file
 * \param file file name
 * \param data data pointer
 * \param len length of data
 */
void file_save(gchar *name, const gchar *data, gsize len)
{
	GFile *file = g_file_new_for_path(name);
	GError *error = NULL;
	GFileOutputStream *stream;

	if (len == -1) {
		len = strlen(data);
	}

	if (g_file_query_exists(file, NULL)) {
		stream = g_file_replace(file, NULL, FALSE, G_FILE_CREATE_PRIVATE, NULL, &error);
	} else {
		stream = g_file_create(file, G_FILE_CREATE_PRIVATE, NULL, &error);
	}

	g_object_unref(file);

	if (stream != NULL) {
		g_output_stream_write(G_OUTPUT_STREAM(stream), data, len, NULL, NULL);
		g_output_stream_flush(G_OUTPUT_STREAM(stream), NULL, NULL);
		g_output_stream_close(G_OUTPUT_STREAM(stream), NULL, NULL);

		g_object_unref(stream);
	} else {
		g_warning("%s", error->message);
		g_error_free(error);
	}
}

/**
 * \brief Load file
 * \param name file name
 * \param size pointer to store length of data to
 * \return file data pointer
 */
gchar *file_load(gchar *name, gsize *size)
{
	GFile *file;
	GFileInfo *file_info;
	goffset file_size;
	gchar *data = NULL;
	GFileInputStream *input_stream = NULL;

	file = g_file_new_for_path(name);
	if (!g_file_query_exists(file, NULL)) {
		return NULL;
	}

	file_info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	file_size = g_file_info_get_size(file_info);
	if (file_size) {
		data = g_malloc0(file_size + 1);
		input_stream = g_file_read(file, NULL, NULL);

		g_input_stream_read_all(G_INPUT_STREAM(input_stream), data, file_size, size, NULL, NULL);

		g_object_unref(input_stream);
	}
	g_object_unref(file_info);
	g_object_unref(file);

	return data;
}
