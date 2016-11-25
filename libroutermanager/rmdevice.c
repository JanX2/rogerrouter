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

#include <libroutermanager/rmphone.h>

/**
 * SECTION:rmdevice
 * @title: RmDevice
 * @short_description: Device (phone/fax) handling functions
 *
 * Abstraction level for device (phone/fax).
 */

/**
 * rm_device_number_is_handled:
 * @number: check if number is handled by a device
 *
 * Returns: %TRUE if number is handled by a phone device, otherwise %FALSE
 */
gboolean rm_device_number_is_handled(gchar *number)
{
	GSList *devices;

	for (devices = rm_phone_get_plugins(); devices != NULL; devices = devices->next) {
		RmPhone *phone = devices->data;

		if (phone->number_is_handled && phone->number_is_handled(number)) {
			return TRUE;
		}
	}

	return FALSE;
}
