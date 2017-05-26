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

#ifndef __RM_MAIN_H
#define __RM_MAIN_H

#include <rmconfig.h>

#include <glib.h>
#include <libintl.h>

G_BEGIN_DECLS

#define R_(text) dgettext(RM_GETTEXT_PACKAGE, text)

#define RM_SCHEME "org.tabos.rm"
#define RM_SCHEME_PROFILE "org.tabos.rm.profile"
#define RM_SCHEME_PROFILE_ACTION "org.tabos.rm.profile.action"

#define RM_PATH "/org/tabos/rm/"

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

gchar *rm_get_user_config_dir(void);
gchar *rm_get_user_cache_dir(void);
gchar *rm_get_user_data_dir(void);

void rm_use_fax_server(gboolean on);

G_END_DECLS

#endif
