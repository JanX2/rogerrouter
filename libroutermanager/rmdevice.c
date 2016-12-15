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

#include <config.h>

#include <glib.h>
#include <gio/gio.h>

#include <libroutermanager/rmdevice.h>
#include <libroutermanager/rmstring.h>
#include <libroutermanager/rmsettings.h>
#include <libroutermanager/rmprofile.h>

/**
 * SECTION:rmdevice
 * @title: RmDevice
 * @short_description: Device (phone/fax) handling functions
 *
 * Abstraction level for device (phone/fax).
 */

/**
 * rm_device_handles_number:
 * @number: check if number is handled by a device
 *
 * Returns: %TRUE if number is handled by a phone device, otherwise %FALSE
 */
gboolean rm_device_handles_number(RmDevice *device, gchar *number)
{
	GSettings *settings;
	gboolean ret = FALSE;

	if (!device) {
		g_debug("%s(): No device", __FUNCTION__);
		return FALSE;
	}

	settings = rm_settings_new_profile("org.tabos.routermanager.profile.devicenumbers", device->settings_name, (gchar*)rm_profile_get_name(rm_profile_get_active()));

	if (settings) {
		gchar **numbers = g_settings_get_strv(settings, "numbers");

		ret = rm_strv_contains((const gchar *const*)numbers, number);

		g_object_unref(settings);
	}

	return ret;
}

void rm_device_set_numbers(RmDevice *device, gchar **numbers)
{
	GSettings *settings;

	if (!device) {
		g_debug("%s(): No device", __FUNCTION__);
		return;
	}

	settings = rm_settings_new_profile("org.tabos.routermanager.profile.devicenumbers", device->settings_name, (gchar*) rm_profile_get_name(rm_profile_get_active()));

	if (settings) {
		g_settings_set_strv(settings, "numbers", (const gchar * const *)numbers);

		g_object_unref(settings);
	}
}

gchar **rm_device_get_numbers(RmDevice *device)
{
	GSettings *settings;
	gchar **numbers = NULL;

	if (!device) {
		g_debug("%s(): No device", __FUNCTION__);
		return numbers;
	}

	settings = rm_settings_new_profile("org.tabos.routermanager.profile.devicenumbers", device->settings_name, (gchar*)rm_profile_get_name(rm_profile_get_active()));

	if (settings) {
		numbers = g_settings_get_strv(settings, "numbers");

		g_object_unref(settings);
	}

	return numbers;
}

/** Internal device list */
static GSList *rm_device_plugins = NULL;

/**
 * rm_device_get:
 * @name: name of device to lookup
 *
 * Find device as requested by name.
 *
 * Returns: a #RmDevice, or %NULL on error
 */
RmDevice *rm_device_get(gchar *name)
{
	GSList *list;

	for (list = rm_device_plugins; list != NULL; list = list->next) {
		RmDevice *device = list->data;

		if (device && device->name && name && !strcmp(device->name, name)) {
			return device;
		}
	}

	return NULL;
}

RmDevice *rm_device_register(gchar *name)
{
	RmDevice *device = g_slice_alloc0(sizeof(RmDevice));

	device->name = g_strdup(name);
	device->settings_name = g_strconcat(device->name, "-numbers", NULL);

	g_debug("%s(): Registering %s", __FUNCTION__, device->name);
	rm_device_plugins = g_slist_prepend(rm_device_plugins, device);

	return device;
}

void rm_device_unregister(RmDevice *device)
{
	g_debug("%s(): Unregister %s", __FUNCTION__, device->name);
	rm_device_plugins = g_slist_remove(rm_device_plugins, device);
	g_free(device->settings_name);
	g_free(device->name);
}

GSList *rm_device_get_plugins(void)
{
	return rm_device_plugins;
}

gchar *rm_device_get_name(RmDevice *device)
{
	return g_strdup(device->name);
}
