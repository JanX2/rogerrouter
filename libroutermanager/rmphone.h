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

#ifndef LIBROUTERMANAGER_PHONE_H
#define LIBROUTERMANAGER_PHONE_H

#include <glib.h>

#include <libroutermanager/rmconnection.h>

G_BEGIN_DECLS

typedef struct {
	gchar *name;
	RmConnection *(*dial)(const gchar *target, gboolean anonymous);
	gint (*pickup)(RmConnection *connection);
	void (*hangup)(RmConnection *connection);
	void (*hold)(RmConnection *connection, gboolean hold);
	void (*send_dtmf_code)(RmConnection *connection, guchar code);

	gboolean (*number_is_handled)(gchar *number);

	void (*mute)(RmConnection *connection, gboolean mute);
	/*void (*record)(gpointer connection, guchar hold, const gchar *dir);*/
} RmPhone;

void rm_phone_register(RmPhone *phone);
void rm_phone_unregister(RmPhone *phone);

GSList *rm_phone_get_plugins(void);
GSList *rm_phone_get_devices(RmPhone *phone);
void rm_phone_set_device(RmPhone *phone, gchar *name);

void rm_phone_mute(RmPhone *phone, RmConnection *connection, gboolean mute);
void rm_phone_record(RmPhone *phone, RmConnection *connection, guchar record, const char *dir);
void rm_phone_hold(RmPhone *phone, RmConnection *connection, gboolean hold);
void rm_phone_dtmf(RmPhone *phone, RmConnection *connection, guchar code);
void rm_phone_hangup(RmPhone *phone, RmConnection *connection);
gint rm_phone_pickup(RmPhone *phone, RmConnection *connection);
RmConnection *rm_phone_dial(RmPhone *phone, const gchar *target, gboolean anonymous);

G_END_DECLS

#endif
