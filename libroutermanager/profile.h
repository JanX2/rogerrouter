/**
 * The libroutermanager project
 * Copyright (c) 2012-2014 Jan-Michael Brummer
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

#ifndef LIBROUTERMANAGER_PROFILE_H
#define LIBROUTERMANAGER_PROFILE_H

#include <gio/gio.h>

G_BEGIN_DECLS

struct profile {
	gchar *name;
	struct router_info *router_info;

	GSettings *settings;

	GSList *action_list;
};

gboolean profile_init(void);
void profile_shutdown(void);

struct profile *profile_new(const gchar *name, const gchar *host, const gchar *user, const gchar *password);
struct profile *profile_get_active(void);
GSList *profile_get_list(void);
void profile_add(struct profile *profile);
void profile_remove(struct profile *profile);
void profile_set_active(struct profile *profile);
void profile_commit(void);
gboolean profile_detect_active(void);

G_END_DECLS

#endif
