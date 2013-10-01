/*******************************************************************
 * libfaxophone                                                    *
 * Created by Jan-Michael Brummer                                  *
 * All parts are distributed under the terms of GPLv2. See COPYING *
 *******************************************************************/

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
