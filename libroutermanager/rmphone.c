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
#include <libroutermanager/rmstring.h>

/** Internal phone list */
static GSList *rm_phone_plugins = NULL;

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

	return phone->dial(target, anonymous);
}

gint rm_phone_pickup(RmPhone *phone, RmConnection *connection)
{
	if (!phone || !phone->pickup) {
		g_warning("%s(): No phone or pickup function", __FUNCTION__);
		return -1;
	}

	return phone->pickup(connection);
}

void rm_phone_hangup(RmPhone *phone, RmConnection *connection)
{
	if (!phone || !phone->hangup) {
		g_warning("%s(): No phone or hangup function", __FUNCTION__);
		return;
	}

	phone->hangup(connection);
}

void rm_phone_hold(RmPhone *phone, RmConnection *connection, gboolean hold)
{
	if (!phone || !phone->hold) {
		g_warning("%s(): No phone or hold function", __FUNCTION__);
		return;
	}

	phone->hold(connection, hold);
}

void rm_phone_dtmf(RmPhone *phone, RmConnection *connection, guchar chr)
{
	if (!phone || !phone->send_dtmf_code) {
		g_warning("%s(): No phone or send dtmf code function", __FUNCTION__);
		return;
	}

	phone->send_dtmf_code(connection, chr);
}

void rm_phone_mute(RmPhone *phone, RmConnection *connection, gboolean mute)
{
	if (!phone || !phone->mute) {
		g_warning("%s(): No phone or mute function", __FUNCTION__);
		return;
	}

	phone->mute(connection, mute);
}

void rm_phone_record(RmPhone *phone, RmConnection *connection, guchar record, const char *dir)
{
}

/**
 * \brief Register phone plugin
 * \param phone phone plugin
 */
void rm_phone_register(RmPhone *phone)
{
	g_debug("%s(): Registering %s", __FUNCTION__, phone->name);
	rm_phone_plugins = g_slist_prepend(rm_phone_plugins, phone);
}

GSList *rm_phone_get_plugins(void)
{
	return rm_phone_plugins;
}
