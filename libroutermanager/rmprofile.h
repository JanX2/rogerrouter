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

#ifndef LIBROUTERMANAGER_RMPROFILE_H
#define LIBROUTERMANAGER_RMPROFILE_H

#include <gio/gio.h>

#include <libroutermanager/rmaddressbook.h>
#include <libroutermanager/rmaudio.h>

G_BEGIN_DECLS

typedef struct profile {
	/*< private >*/
	gchar *name;
	struct router_info *router_info;

	GSettings *settings;

	GSList *action_list;
} RmProfile;

gboolean rm_profile_init(void);
void rm_profile_shutdown(void);
GSList *rm_profile_get_list(void);
RmProfile *rm_profile_add(const gchar *name);
void rm_profile_remove(RmProfile *profile);

RmProfile *rm_profile_get_active(void);
void rm_profile_set_active(RmProfile *profile);

gboolean rm_profile_detect_active(void);

const gchar *rm_profile_get_name(RmProfile *profile);
void rm_profile_set_host(RmProfile *profile, const gchar *host);
void rm_profile_set_login_user(RmProfile *profile, const gchar *user);
void rm_profile_set_login_password(RmProfile *profile, const gchar *password);
RmAddressBook *rm_profile_get_addressbook(RmProfile *profile);
RmAudio *rm_profile_get_audio(RmProfile *profile);

G_END_DECLS

#endif
