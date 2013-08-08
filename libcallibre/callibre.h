/**
 * The libcallibre project
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

#ifndef LIBCALLIBRE_CALLIBRE_H
#define LIBCALLIBRE_CALLIBRE_H

#include <libcallibre/ui_ops.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

G_BEGIN_DECLS

#define _(text) gettext(text)

#define CALLIBRE_SCHEME "org.tabos.callibre"
#define CALLIBRE_SCHEME_PROFILE "org.tabos.callibre.profile"
#define CALLIBRE_SCHEME_PROFILE_ACTION "org.tabos.callibre.profile.action"

gboolean callibre_init(struct ui_ops *ui_ops, gboolean debug);
void callibre_shutdown(void);
gchar *get_directory(gchar *type);
gchar *get_plugin_dir(void);

G_END_DECLS

#endif
