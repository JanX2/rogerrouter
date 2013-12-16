/**
 * Roger Router
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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
	{"call-end-symbolic", "call-stop"}
};

GtkWidget *get_icon(gint type, gint size)
{
	GtkWidget *image = NULL;

	image = gtk_image_new_from_icon_name(app_icons[type].primary, size);
	if (!image && !EMPTY_STRING(app_icons[type].secondary)) {
		image = gtk_image_new_from_icon_name(app_icons[type].secondary, size);
	}

	return image;
}
