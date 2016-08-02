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

#include <string.h>
#include <glib.h>

#include <libroutermanager/rmphone.h>
#include <libroutermanager/gstring.h>

/** Internal phone list */
static GSList *rm_phone_plugins = NULL;

struct connection *rm_phone_dial(const gchar *target, gboolean anonymous)
{
	struct device_phone *phone = NULL;

	if (rm_phone_plugins) {
		phone = rm_phone_plugins->data;
	}

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

gint rm_phone_pickup(struct connection *connection)
{
	struct device_phone *phone = rm_phone_plugins->data;

	if (!phone || !phone->pickup) {
		g_warning("%s(): No phone or pickup function", __FUNCTION__);
		return -1;
	}

	return phone->pickup(connection);
}

void rm_phone_hangup(struct connection *connection)
{
	struct device_phone *phone = rm_phone_plugins->data;

	if (!phone || !phone->hangup) {
		g_warning("%s(): No phone or hangup function", __FUNCTION__);
		return;
	}

	phone->hangup(connection);
}

void rm_phone_hold(struct connection *connection, gboolean hold)
{
	struct device_phone *phone = rm_phone_plugins->data;

	if (!phone || !phone->hold) {
		g_warning("%s(): No phone or hold function", __FUNCTION__);
		return;
	}

	phone->hold(connection, hold);
}

void rm_phone_dtmf(struct connection *connection, guchar chr)
{
	struct device_phone *phone = rm_phone_plugins->data;

	if (!phone || !phone->send_dtmf_code) {
		g_warning("%s(): No phone or send dtmf code function", __FUNCTION__);
		return;
	}

	phone->send_dtmf_code(connection, chr);
}

void rm_phone_mute(struct connection *connection, gboolean mute)
{
	struct device_phone *phone = rm_phone_plugins->data;

	if (!phone || !phone->mute) {
		g_warning("%s(): No phone or mute function", __FUNCTION__);
		return;
	}

	phone->mute(connection, mute);
}

void rm_phone_record(struct connection *connection, guchar record, const char *dir)
{
}

/**
 * \brief Register phone plugin
 * \param phone phone plugin
 */
void rm_phone_register(struct device_phone *phone)
{
	g_debug("%s(): Registering %s", __FUNCTION__, phone->name);
	rm_phone_plugins = g_slist_prepend(rm_phone_plugins, phone);
}

GSList *rm_phone_get_plugins(void)
{
	return rm_phone_plugins;
}
