/**
 * The libroutermanager project
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

#include <string.h>

#include <glib.h>

#include <libroutermanager/bluetooth.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/gstring.h>
#include <config.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

/** dbus connection - global because we want to easily access the function */
static DBusGConnection *dbus_connection = NULL;

/**
 * \brief Get bluetooth device name
 * \param device device proxy
 * \return device name or NULL
 */
static gchar *bluetooth_get_name(DBusGProxy *device) {
	GHashTable *hash;
	gchar *name = NULL;

	if (dbus_g_proxy_call(device, "GetProperties", NULL, G_TYPE_INVALID, dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), &hash, G_TYPE_INVALID) != FALSE) {
		GValue *value;
		guint class;

		/* We are looking for 0x240404 Audio/Wearable Headset Device */
		value = g_hash_table_lookup(hash, "Class");
		class = g_value_get_uint(value);

		//g_debug("Class: '%x'", class);

		if (class == 0x240404 || class == 0x200404) {
			value = g_hash_table_lookup(hash, "Name");
			name = value ? g_value_dup_string(value) : NULL;
		} else {
			g_debug("Unhandled class '%x'", g_value_get_uint(value));
			value = g_hash_table_lookup(hash, "Name");
			name = value ? g_value_dup_string(value) : NULL;
			g_debug("Name: '%s'", name);
			name = NULL;
		}

		g_hash_table_destroy(hash);
	}

	return name;
}

/**
 * \brief Scan all bluetooth adapters for device and create list
 * \return error code
 */
GSList *bluetooth_create_list(void) {
	GError *error = NULL;
	DBusGProxy *manager = NULL;
	DBusGProxy *adapter = NULL;
	DBusGProxy *device = NULL;
	GPtrArray *adapters = NULL;
	GPtrArray *devices = NULL;
	struct bluetooth_device_info *bluetooth_device_info = NULL;
	gchar *device_path = NULL;
	gint adapter_index = 0;
	gint device_index = 0;
	GSList *bluetooth_device_list = NULL;

	if (!dbus_connection) {
		return NULL;
	}

	/* Get dbus bluez manager */
	manager = dbus_g_proxy_new_for_name(dbus_connection, "org.bluez", "/", "org.bluez.Manager");
	if (manager == NULL) {
		return NULL;
	}

	/* Get adapter list */
	dbus_g_proxy_call(manager, "ListAdapters", &error, G_TYPE_INVALID, dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH), &adapters, G_TYPE_INVALID);
	g_object_unref(manager);
	if (error) {
		g_warning("Couldn't get adapter list: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	for (adapter_index = 0; adapter_index < adapters->len; adapter_index++) {
		/* Create adapter proxy */
		adapter = dbus_g_proxy_new_for_name(dbus_connection, "org.bluez", g_ptr_array_index(adapters, adapter_index), "org.bluez.Adapter");
		if (adapter == NULL) {
			g_warning("Couldn't get adapter proxy!!");
			continue;
		}

		/* List all devices on this adapter */
		dbus_g_proxy_call(adapter, "ListDevices", &error, G_TYPE_INVALID, dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH), &devices, G_TYPE_INVALID);
		g_object_unref(adapter);

		if (error) {
			g_warning("Couldn't get device list: %s", error->message);
			g_error_free(error);
			return NULL;
		}
		
		for (device_index = 0; device_index < devices->len; device_index++) {
			gchar *name;

			device_path = g_ptr_array_index(devices, device_index);
			device = dbus_g_proxy_new_for_name(dbus_connection, "org.bluez", device_path, "org.bluez.Device");

			name = bluetooth_get_name(device);
			if (name) {
				bluetooth_device_info = g_slice_new0(struct bluetooth_device_info);

				if (bluetooth_device_info) {
					bluetooth_device_info->name = name;
					bluetooth_device_info->path = g_strdup(device_path);

					bluetooth_device_list = g_slist_prepend(bluetooth_device_list, bluetooth_device_info);
				}
			}

			g_object_unref(device);
		}
	}

	g_ptr_array_free(adapters, TRUE);

	return bluetooth_device_list;
}

//#if 0
/**
 * \brief Indicate call on headset
 * \return 0 on success, otherwise error
 */
gboolean bluetooth_indicate_call(gpointer headset) {
	GError *error = NULL;

	dbus_g_proxy_call(headset, "IndicateCall", &error, G_TYPE_INVALID, G_TYPE_INVALID);
	if (error) {
		g_warning("Couldn't call IndicateCall: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	return TRUE;
}
//#endif

/**
 * \brief Cancel an incoming call indication
 * \return 0 on success, otherwise error
 */
static gboolean bluetooth_cancel_call(DBusGProxy *headset) {
	GError *error = NULL;

	dbus_g_proxy_call(headset, "CancelCall", &error, G_TYPE_INVALID, G_TYPE_INVALID);
	if (error) {
		g_warning("Couldn't call CancelCall: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	return TRUE;
}

/**
 * \brief Callback function called when bluetooth headset answer button is pressed
 * \param object current dbus object proxy
 * \param user_data headset pointer
 */
static void bluetooth_answer_requested(DBusGProxy *object, gpointer user_data) {
	DBusGProxy *headset = user_data;

	g_debug("Got Signal: AnswerRequested!");
	bluetooth_cancel_call(headset);

	//softphone_handle_bluetooth();
}

/**
 * \brief Set active bluetooth headset
 * \param path bluetooth device path
 * \return TRUE on success, otherwise error
 */
gboolean bluetooth_set_headset(gchar *path) {
	DBusGProxy *headset;

	headset = dbus_g_proxy_new_for_name(dbus_connection, "org.bluez", path, "org.bluez.Headset");
	if (headset == NULL) {
		g_warning("Could not create bluetooth headset proxy!");
		return FALSE;
	}

	dbus_g_proxy_add_signal(headset, "AnswerRequested", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(headset, "AnswerRequested", G_CALLBACK(bluetooth_answer_requested), headset, NULL);

	return TRUE;
}

/**
 * \brief Initialize bluetooth dbus_connection and create device list
 * \return error code
 */
gboolean bluetooth_init(void)
{
	GError *error = NULL;
	struct profile *profile;
	gchar *path;

	/* Get dbus_connection */
	dbus_connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (dbus_connection == NULL) {
		g_printerr("Error: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	/* Set headset */
	profile = profile_get_active();
	if (profile) {
		path = g_settings_get_string(profile->settings, "bluetooth-headset");
		if (!EMPTY_STRING(path)) {
			bluetooth_set_headset(path);
		}
	}

	return TRUE;
}
