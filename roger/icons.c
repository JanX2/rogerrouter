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

#include <string.h>

#include <gtk/gtk.h>

#include <libroutermanager/routermanager.h>
#include <libroutermanager/gstring.h>

#include <roger/icons.h>

struct icons app_icons[11] = {
	{"list-add-symbolic", "list-add"},
	{"document-open-symbolic", "document-open"},
	{"list-remove-symbolic", "list-remove"},
	{"view-refresh-symbolic", "view-refresh"},
	{"user-trash-symbolic", "user-trash"},
	{"call-start-symbolic", "call-start"},
	{"user-info", "user-info"},
	{"printer-symbolic", "printer"},
	{"edit-clear-symbolic", "edit-clear"},
	{"edit-find-symbolic", "edit-find"},
	{"call-stop-symbolic", "call-stop"}
};

GtkWidget *get_icon(gint type, gint size)
{
	GtkWidget *image = NULL;
	GdkPixbuf *pixbuf;
	GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
	GError *error = NULL;

	/* Try to load primary icon, if it is successful set it as image, otherwise use secondary */
	pixbuf = gtk_icon_theme_load_icon(icon_theme, app_icons[type].primary, 48, 0, &error);
	if (pixbuf) {
		image = gtk_image_new_from_icon_name(app_icons[type].primary, size);
		g_object_unref(pixbuf);
	} else {
		image = gtk_image_new_from_icon_name(app_icons[type].secondary, size);
	}

	return image;
}

GdkPixbuf *image_get_scaled(GdkPixbuf *image, gint req_width, gint req_height)
{
	GdkPixbuf *scaled = NULL;
	gint width, height;
	gint orig_width, orig_height;
	gfloat factor;

	if (req_width != -1 && req_height != -1) {
		orig_width = req_width;
		orig_height = req_height;
	} else {
		gtk_icon_size_lookup(GTK_ICON_SIZE_DIALOG, &orig_width, &orig_height);
	}

	if (!image) {
		image = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "avatar-default-symbolic", orig_width, 0, NULL);
	}

	width = gdk_pixbuf_get_width(image);
	height = gdk_pixbuf_get_height(image);
	if (width > height) {
		factor = (float)width / (float)height;
		width = orig_width;
		height = orig_height / factor;
	} else {
		factor = (float)height / (float)width;
		height = orig_height;
		width = orig_width / (float)factor;
	}

	scaled = gdk_pixbuf_scale_simple(image, width, height, GDK_INTERP_BILINEAR);

	return scaled;
}

