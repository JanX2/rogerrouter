/**
 * The libroutermanager project
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

#ifndef LIBROUTERMANAGER_ROUTERMANAGER_H
#define LIBROUTERMANAGER_ROUTERMANAGER_H

#include <libroutermanager/ui_ops.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

G_BEGIN_DECLS

#define _(text) gettext(text)

#define ROUTERMANAGER_SCHEME "org.tabos.routermanager"
#define ROUTERMANAGER_SCHEME_PROFILE "org.tabos.routermanager.profile"
#define ROUTERMANAGER_SCHEME_PROFILE_ACTION "org.tabos.routermanager.profile.action"

gboolean routermanager_init(struct ui_ops *ui_ops, gboolean debug);
void routermanager_shutdown(void);
gchar *get_directory(gchar *type);
gchar *get_plugin_dir(void);
void init_directory_paths(void);

G_END_DECLS

#endif
