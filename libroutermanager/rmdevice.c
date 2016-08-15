#include <config.h>

#include <glib.h>

#include <libroutermanager/rmphone.h>

/**
 * rm_device_number_is_handled:
 * @number: check if number is handled by a device
 *
 * Returns: TRUE if number is handled by a phone device, otherwise FALSE
 */
gboolean rm_device_number_is_handled(gchar *number)
{
	GSList *devices;

	for (devices = rm_phone_get_plugins(); devices != NULL; devices = devices->next) {
		struct device_phone *phone = devices->data;

		if (phone->number_is_handled(number)) {
			return TRUE;
		}
	}

	return FALSE;
}
