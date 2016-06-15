/**
 * The libroutermanager project
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

#include <config.h>

#include <string.h>
#include <glib.h>

#include <libroutermanager/device_phone.h>
#include <libroutermanager/gstring.h>

/** Internal phone list */
static GSList *phone_plugins = NULL;

gpointer phone_dial(const gchar *target, gboolean anonymous)
{
	struct device_phone *phone = phone_plugins->data;

	if (!phone) {
		g_warning("%s(): No phone plugin", __FUNCTION__);
		return NULL;
	} else if (!phone->dial) {
		g_warning("%s(): No dial function in phone plugin", __FUNCTION__);
		return NULL;
	} else if (EMPTY_STRING(target)) {
		g_warning("%s(): target is empty", __FUNCTION__);
		return NULL;
	}

	return phone->dial(target, anonymous);
}

gint phone_pickup(gpointer connection)
{
	struct device_phone *phone = phone_plugins->data;

	if (!phone || !phone->pickup) {
		g_warning("%s(): No phone or pickup function", __FUNCTION__);
		return -1;
	}

	return phone->pickup(connection);
}

void phone_hangup(gpointer connection)
{
	struct device_phone *phone = phone_plugins->data;

	if (!phone || !phone->hangup) {
		g_warning("%s(): No phone or hangup function", __FUNCTION__);
		return;
	}

	phone->hangup(connection);
}

void phone_hold(gpointer connection, gboolean hold)
{
	struct device_phone *phone = phone_plugins->data;

	if (!phone || !phone->hold) {
		g_warning("%s(): No phone or hold function", __FUNCTION__);
		return;
	}

	phone->hold(connection, hold);
}

void phone_send_dtmf_code(gpointer connection, guchar chr)
{
	struct device_phone *phone = phone_plugins->data;

	if (!phone || !phone->send_dtmf_code) {
		g_warning("%s(): No phone or send dtmf code function", __FUNCTION__);
		return;
	}

	phone->send_dtmf_code(connection, chr);
}

void phone_mute(gpointer connection, gboolean mute)
{
	struct device_phone *phone = phone_plugins->data;

	if (!phone || !phone->mute) {
		g_warning("%s(): No phone or mute function", __FUNCTION__);
		return;
	}

	phone->mute(connection, mute);
}

/**
 * \brief Register phone plugin
 * \param phone phone plugin
 */
void phone_register(struct device_phone *phone)
{
	g_debug("%s(): Registering %s", __FUNCTION__, phone->name);
	phone_plugins = g_slist_prepend(phone_plugins, phone);
}

GSList *phone_get_plugins(void)
{
	return phone_plugins;
}
