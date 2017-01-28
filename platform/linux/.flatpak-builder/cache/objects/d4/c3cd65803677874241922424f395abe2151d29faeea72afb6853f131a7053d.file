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

#ifndef __RM_ROUTERINFO_H
#define __RM_ROUTERINFO_H

#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * RmRouterInfo:
 *
 * Router information structure
 */
typedef struct {
	/* Host name */
	gchar *host;
	/* User */
	gchar *user;
	/* User password */
	gchar *password;
	/* Name of router */
	gchar *name;
	/* Router version */
	gchar *version;
	/* Serial number */
	gchar *serial;
	/* Session id */
	gchar *session_id;
	/* Language */
	gchar *lang;
	/* Annex */
	gchar *annex;

	/* Box id */
	gint box_id;
	/* Major number */
	gint maj_ver_id;
	/* Minor number */
	gint min_ver_id;
	/* Session timer */
	GTimer *session_timer;
} RmRouterInfo;

G_END_DECLS

#endif
