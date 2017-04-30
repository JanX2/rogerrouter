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

#include <rmconfig.h>

#include <string.h>
#include <glib.h>

#include <rm/rmphone.h>
#include <rm/rmstring.h>

/** Internal phone list */
static GSList *rm_phone_plugins = NULL;

/**
 * rm_phone_get:
 * @name: name of phone to lookup
 *
 * Find phone as requested by name.
 *
 * Returns: a #RmPhone, or %NULL on error
 */
RmPhone *rm_phone_get(gchar *name)
{
	GSList *list;

	for (list = rm_phone_plugins; list != NULL; list = list->next) {
		RmPhone *phone = list->data;

		if (phone && phone->name && name && !strcmp(phone->name, name)) {
			return phone;
		}
	}

	return NULL;
}

/**
 * rm_phone_dial:
 * @phone: a #RmPhone
 * @target: target number
 * @anonymous: flag to indicate anonymous dial
 *
 * Dial @target phone number.
 *
 * Returns: a #RmConnection or %NULL on error
 */
RmConnection *rm_phone_dial(RmPhone *phone, const gchar *target, gboolean anonymous)
{
	if (!phone) {
		g_warning("%s(): No phone plugin", __FUNCTION__);
		return NULL;
	} else if (!phone->dial) {
		g_warning("%s(): No dial function in phone plugin", __FUNCTION__);
		return NULL;
	} else if (RM_EMPTY_STRING(target)) {
		g_warning("%s(): target is empty", __FUNCTION__);
		return NULL;
	}

	return phone->dial(phone, target, anonymous);
}

/**
 * rm_phone_pickup:
 * @connection: a #RmConnection
 *
 * Pickup incoming call of @connection.
 *
 * Returns: Status flag
 */
gint rm_phone_pickup(RmConnection *connection)
{
	RmPhone *phone = RM_PHONE(connection->device);

	g_debug("%s(): connection %p, device %p phone %p", __FUNCTION__, connection, connection->device, phone);

	if (!phone || !phone->pickup) {
		g_warning("%s(): No phone or pickup function", __FUNCTION__);
		return -1;
	}

	return phone->pickup(connection);
}

/**
 * rm_phone_hangup:
 * @connection: a #RmConnection
 *
 * Hangup an active @connection.
 */
void rm_phone_hangup(RmConnection *connection)
{
	RmPhone *phone = RM_PHONE(connection->device);

	if (!phone || !phone->hangup) {
		g_warning("%s(): No phone or hangup function (%p)", __FUNCTION__, phone);
		if (phone) {
			g_warning("%s(): Phone '%s'", __FUNCTION__, rm_phone_get_name(phone));
		}
		return;
	}

	phone->hangup(connection);
}

/**
 * rm_phone_hold:
 * @phone: a #RmPhone
 * @connection: a #RmConnection
 * @hold: flag to hold/release connection
 *
 * Holds/Releases active @connection.
 */
void rm_phone_hold(RmPhone *phone, RmConnection *connection, gboolean hold)
{
	if (!phone || !phone->hold) {
		g_warning("%s(): No phone or hold function", __FUNCTION__);
		return;
	}

	phone->hold(connection, hold);
}

/**
 * rm_phone_dtmf:
 * @phone: a #RmPhone
 * @connection: a #RmConnection
 * @chr: character to send
 *
 * Send dtmf code on @connection
 * */
void rm_phone_dtmf(RmPhone *phone, RmConnection *connection, guchar chr)
{
	if (!phone || !phone->send_dtmf_code) {
		g_warning("%s(): No phone or send dtmf code function", __FUNCTION__);
		return;
	}

	phone->send_dtmf_code(connection, chr);
}

/**
 * rm_phone_mute:
 * @phone: a #RmPhone
 * @connection: a #RmConnection
 * @mute: flag to un/mute connection
 *
 * Un/Mutes @connection.
 */
void rm_phone_mute(RmPhone *phone, RmConnection *connection, gboolean mute)
{
	if (!phone || !phone->mute) {
		g_warning("%s(): No phone or mute function", __FUNCTION__);
		return;
	}

	phone->mute(connection, mute);
}

/**
 * rm_phone_record:
 * @phone: a #RmPhone
 * @connection: a #RmConnection
 * @record: Flag to start/stop recording
 * @dir: directory name to store recording to
 *
 * Start/Stops recording of active @connection
 */
void rm_phone_record(RmPhone *phone, RmConnection *connection, guchar record, const char *dir)
{
}

/**
 * rm_phone_register:
 * @phone: a #RmPhone
 *
 * Register phone plugin
 */
void rm_phone_register(RmPhone *phone)
{
	g_debug("%s(): Registering %s", __FUNCTION__, phone->name);
	rm_phone_plugins = g_slist_prepend(rm_phone_plugins, phone);
}

/**
 * rm_phone_unregister:
 * @phone: a #RmPhone
 *
 * Unregister phone plugin
 */
void rm_phone_unregister(RmPhone *phone)
{
	g_debug("%s(): Unregister %s", __FUNCTION__, phone->name);
	rm_phone_plugins = g_slist_remove(rm_phone_plugins, phone);
}

/**
 * rm_phone_get_plugins:
 *
 * Retrieves #GSList of phone plugins
 *
 * Returns: a list of phone plugins
 */
GSList *rm_phone_get_plugins(void)
{
	return rm_phone_plugins;
}

/**
 * rm_phone_get_name:
 * @phone: a #RmPhone
 *
 * Get name of @phone device
 *
 * Returns: phone name
 */
gchar *rm_phone_get_name(RmPhone *phone)
{
	return g_strdup(phone->name);
}
