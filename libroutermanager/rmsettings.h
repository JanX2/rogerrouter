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

#ifndef LIBROUTERMANAGER_RMSETTINGS_H
#define LIBROUTERMANAGER_RMSETTINGS_H

G_BEGIN_DECLS

GSettings *rm_settings_new(gchar *scheme);
GSettings *rm_settings_new_with_path(gchar *scheme, gchar *path);
GSettings *rm_settings_new_profile(gchar *scheme, gchar *name, gchar *profile_name);
gboolean rm_settings_backend_is_dconf(void);

G_END_DECLS

#endif
