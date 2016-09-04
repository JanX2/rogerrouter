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

#ifndef LIBROUTERMANAGER_RMNOTIFICATION_H
#define LIBROUTERMANAGER_RMNOTIFICATION_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	gpointer priv;
	RmConnection *connection;
} RmNotification;

typedef struct {
	void (*show)(RmNotification *notification);
	void (*update)(RmNotification *notification);
	void (*close)(RmNotification *notification);
} RmNotificationPlugin;

RmNotification *rm_notification_new(void);
void rm_notification_remove(RmNotification *notification);
void rm_notification_add_connection(RmNotification *notification, RmConnection *connection);
void rm_notification_set_uid(RmNotification *notification, gpointer uid);

void rm_notification_register(RmNotificationPlugin *plugin);
void rm_notification_unregister(RmNotificationPlugin *plugin);

G_END_DECLS

#endif
