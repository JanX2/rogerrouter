#include <config.h>

#include <glib.h>

#include <libroutermanager/rmphone.h>

gboolean device_number_is_handled(gchar *number)
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
