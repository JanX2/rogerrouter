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

#ifndef LIBROUTERMANAGER_RM_H
#define LIBROUTERMANAGER_RM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <libintl.h>

G_BEGIN_DECLS

#define _(text) gettext(text)

#define RM_SCHEME "org.tabos.routermanager"
#define RM_SCHEME_PROFILE "org.tabos.routermanager.profile"
#define RM_SCHEME_PROFILE_ACTION "org.tabos.routermanager.profile.action"

#define RM_PATH "/org/tabos/routermanager/"

#define RM_ERROR rm_print_error_quark()

typedef enum {
	RM_ERROR_FAX,
	RM_ERROR_ROUTER,
	RM_ERROR_AUDIO,
} rm_error;

GQuark rm_print_error_quark(void);
gboolean rm_new(gboolean debug, GError **error);
gboolean rm_init(GError **error);
void rm_shutdown(void);
gchar *rm_get_directory(gchar *type);
void rm_set_requested_profile(gchar *name);
gchar *rm_get_requested_profile(void);

G_END_DECLS

#endif
