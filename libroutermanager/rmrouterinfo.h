/**
 * The libroutermanager project
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#ifndef LIBROUTERMANAGER_RMROUTERINFO_H
#define LIBROUTERMANAGER_RMROUTERINFO_H

#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct {
	gchar *host;
	gchar *user;
	gchar *password;
	gchar *name;
	gchar *version;
	gchar *serial;
	gchar *session_id;
	gchar *lang;
	gchar *annex;

	/* Extend */
	gint box_id;
	gint maj_ver_id;
	gint min_ver_id;
	GTimer *session_timer;
} RmRouterInfo;

G_END_DECLS

#endif
