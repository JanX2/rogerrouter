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
 * \file isdn-convert.h
 * \brief audio conversation header
 */

#ifndef ISDN_CONVERT_H
#define ISDN_CONVERT_H

#include <string.h>
#include <math.h>

#include <libroutermanager/libfaxophone/faxophone.h>

// 16bit PCM, Mono, 8000 hz -> alaw
extern signed char linear16_2_law[65536];
// alaw -> 16bit PCM, Mono, 8000 hz
extern unsigned short law_2_linear16[256];

void create_table_buffer(void);
double get_line_level_in(struct capi_connection *connection);
double get_line_level_out(struct capi_connection *connection);
void convert_isdn_to_audio(struct capi_connection *connection, guchar *in_buf, guint in_buf_len, guchar *out_buf, guint *out_buf_len, short *rec_buf);
void convert_audio_to_isdn(struct capi_connection *connection, guchar *in_buf, guint in_buf_len, guchar *out_buf, guint *out_buf_len, short *rec_buf);

static inline guchar bit_inverse(guchar chr)
{
	return ((chr >> 7) & 0x1) |
	       ((chr >> 5) & 0x2) |
	       ((chr >> 3) & 0x4) |
	       ((chr >> 1) & 0x8) |
	       ((chr << 1) & 0x10) |
	       ((chr << 3) & 0x20) |
	       ((chr << 5) & 0x40) |
	       ((chr << 7) & 0x80);
}

#endif
