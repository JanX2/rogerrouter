/**
 * The rm project
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

#ifndef FRITZBOX_H
#define FRITZBOX_H

#include <rm/rmrouter.h>

G_BEGIN_DECLS

typedef struct {
	gchar *setting_name;
	gchar *code_name;
	gint type;
	gint number;
} FritzBoxPhonePort;

extern RmPhone dialer_phone;
extern GSettings *fritzbox_settings;

gboolean fritzbox_login(RmProfile *profile);
gboolean fritzbox_get_settings(RmProfile *profile);
gboolean fritzbox_load_journal(RmProfile *profile);
gboolean fritzbox_dial_number(RmProfile *profile, gint port, const gchar *number);
gboolean fritzbox_hangup(RmProfile *profile, gint port, const gchar *number);

RmConnection *fritzbox_phone_dialer_get_connection();

G_END_DECLS

#endif
