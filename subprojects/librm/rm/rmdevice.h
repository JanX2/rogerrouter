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

#ifndef __RM_DEVICE_H
#define __RM_DEVICE_H

G_BEGIN_DECLS

typedef struct _RmDevice RmDevice;

typedef enum _RmDeviceType {
	RM_DEVICE_TYPE_PHONE,
	RM_DEVICE_TYPE_FAX
} RmDeviceType;

struct _RmDevice {
	gchar *name;
	gchar *settings_name;
};

struct _RmDeviceCast {
	RmDevice *device;
	RmDeviceType type;

	gpointer priv;
};

#define RM_DEVICE(x) (((struct _RmDeviceCast *)(x))->device)

gboolean rm_device_handles_number(RmDevice *device, gchar *number);
void rm_device_set_numbers(RmDevice *device, gchar **numbers);
gchar **rm_device_get_numbers(RmDevice *device);
RmDevice *rm_device_get(gchar *name);
gchar *rm_device_get_name(RmDevice *device);
RmDevice *rm_device_register(gchar *name);
void rm_device_unregister(RmDevice *device);
GSList *rm_device_get_plugins(void);

G_END_DECLS

#endif

