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

#define APP_ICON_ADD	"list-add-symbolic"
#define APP_ICON_REMOVE	"list-remove-symbolic"
#define APP_ICON_TRASH	"user-trash-symbolic"
#define APP_ICON_CALL	"call-start-symbolic"

#if GTK_CHECK_VERSION(3,12,0)
#define APP_ICON_HANGUP	"call-stop-symbolic"
#else
#define APP_ICON_HANGUP	"call-end-symbolic"
#endif

GdkPixbuf *image_get_scaled(GdkPixbuf *image, gint req_width, gint req_height);

G_END_DECLS

#endif
