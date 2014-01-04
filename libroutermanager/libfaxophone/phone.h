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

/**
 * \file phone.h
 * \brief Phone handling header
 */

#ifndef FAXOPHONE_PHONE_H
#define FAXOPHONE_PHONE_H

#define PHONE_CIP 0x04

struct capi_connection *phone_call(guchar controller, const gchar *source, const gchar *target, gboolean anonymous);
void phone_mute(struct capi_connection *connection, guchar mute);
void phone_hold(struct capi_connection *connection, guchar record);
void phone_record(struct capi_connection *connection, guchar hold, const gchar *dir);
void phone_send_dtmf_code(struct capi_connection *connection, guchar code);
void phone_hangup(struct capi_connection *connection);
gint phone_pickup(struct capi_connection *connection);
void phone_init_data(struct capi_connection *connection);
void phone_transfer(struct capi_connection *connection, _cmsg message);
void phone_conference(struct capi_connection *active, struct capi_connection *hold);
void phone_flush(struct capi_connection *connection);
gint recording_write(struct recorder *recorder, short *buf, gint size, gint channel);

#endif
