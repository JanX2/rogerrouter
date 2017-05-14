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

#ifndef __RM_PROFILE_H
#define __RM_PROFILE_H

#include <gio/gio.h>

#include <rm/rmaddressbook.h>
#include <rm/rmnotification.h>
#include <rm/rmaudio.h>
#include <rm/rmlookup.h>
#include <rm/rmrouterinfo.h>
#include <rm/rmphone.h>
#include <rm/rmfax.h>

G_BEGIN_DECLS

typedef struct {
	/*< private >*/
	gchar *name;
	RmRouterInfo *router_info;

	GSettings *settings;

	GSList *action_list;
	GSList *filter_list;
} RmProfile;

gboolean rm_profile_init(void);
void rm_profile_shutdown(void);
GSList *rm_profile_get_list(void);
RmProfile *rm_profile_add(const gchar *name);
void rm_profile_remove(RmProfile *profile);

RmProfile *rm_profile_get_active(void);
void rm_profile_set_active(RmProfile *profile);

RmProfile *rm_profile_detect(void);

const gchar *rm_profile_get_name(RmProfile *profile);
void rm_profile_set_host(RmProfile *profile, const gchar *host);
void rm_profile_set_login_user(RmProfile *profile, const gchar *user);
void rm_profile_set_login_password(RmProfile *profile, const gchar *password);
RmAddressBook *rm_profile_get_addressbook(RmProfile *profile);
void rm_profile_set_addressbook(RmProfile *profile, RmAddressBook *book);
RmAudio *rm_profile_get_audio(RmProfile *profile);
gchar *rm_profile_get_audio_ringtone(RmProfile *profile);
RmNotification *rm_profile_get_notification(RmProfile *profile);
gchar **rm_profile_get_notification_incoming_numbers(RmProfile *profile);
gchar **rm_profile_get_notification_outgoing_numbers(RmProfile *profile);
void rm_profile_set_notification_incoming_numbers(RmProfile *profile, const gchar * const* numbers);
void rm_profile_set_notification_outgoing_numbers(RmProfile *profile, const gchar * const* numbers);
gboolean rm_profile_get_notification_ringtone(RmProfile *profile);
RmPhone *rm_profile_get_phone(RmProfile *profile);
void rm_profile_set_phone(RmProfile *profile, gchar *name);
RmFax *rm_profile_get_fax(RmProfile *profile);
void rm_profile_set_fax(RmProfile *profile, gchar *name);

G_END_DECLS

#endif
