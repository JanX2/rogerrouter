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

#ifndef LIBROUTERMANAGER_PASSWORD_H
#define LIBROUTERMANAGER_PASSWORD_H

#include <glib.h>

#include <libroutermanager/profile.h>

G_BEGIN_DECLS

struct password_manager {
	const gchar *name;
	void (*set_password)(struct profile *profile, const gchar *name, const gchar *password);
	gchar *(*get_password)(struct profile *profile, const gchar *name);
	gboolean (*remove_password)(struct profile *profile, const gchar *name);
};

void password_manager_set_password(struct profile *profile, const gchar *name, const gchar *password);
gchar *password_manager_get_password(struct profile *profile, const gchar *name);
gboolean password_manager_remove_password(struct profile *profile, const gchar *name);
void password_manager_register(struct password_manager *manager);
GSList *password_manager_get_plugins(void);
void password_manager_init(void);

G_END_DECLS

#endif
