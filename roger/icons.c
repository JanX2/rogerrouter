/**
 * Roger Router
 * Copyright (c) 2012-2014 Jan-Michael Brummer
 *
 * This file is part of Roger Router.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <gtk/gtk.h>
#include <roger/icons.h>

GdkPixbuf *image_get_scaled(GdkPixbuf *image, gint req_width, gint req_height)
{
	GdkPixbuf *scaled = NULL;
	gint width, height;

	g_assert(image != NULL);

	if (req_width != -1 && req_height != -1) {
		width = req_width;
		height = req_height;
	} else {
		gint orig_width, orig_height;
		gfloat factor;

		gtk_icon_size_lookup(GTK_ICON_SIZE_DIALOG, &orig_width, &orig_height);

		width = gdk_pixbuf_get_width(image);
		height = gdk_pixbuf_get_height(image);

		if (req_width != -1) {
			factor = (float) req_width / orig_width;
			width = orig_width / factor;
			height = orig_height / height;
		} else if (req_height != -1) {
			factor = (float) req_height / orig_height;
			width = orig_width / factor;
			height = orig_height / height;
		} else {
			width = orig_width;
			height = orig_height;
		}
	}

	scaled = gdk_pixbuf_scale_simple(image, width, height, GDK_INTERP_BILINEAR);

	return scaled;
}
