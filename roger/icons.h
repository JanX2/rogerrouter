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

#ifndef ICONS_H
#define ICONS_H


G_BEGIN_DECLS

enum {
	APP_ICON_ADD,
	APP_ICON_EDIT,
	APP_ICON_REMOVE,
	APP_ICON_REFRESH,
	APP_ICON_TRASH,
	APP_ICON_CALL,
	APP_ICON_PERSON,
	APP_ICON_PRINT,
	APP_ICON_CLEAR,
	APP_ICON_FIND,
	APP_ICON_HANGUP
};

struct icons {
	/* Preferred symbolic image */
	gchar *primary;
	/* Old icon theme image */
	gchar *secondary;
};

GtkWidget *get_icon(gint type, gint size);

G_END_DECLS

#endif
