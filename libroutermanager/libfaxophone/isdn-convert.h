/*******************************************************************
 * libfaxophone                                                    *
 * Created by Jan-Michael Brummer                                  *
 * All parts are distributed under the terms of GPLv2. See COPYING *
 *******************************************************************/

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

static inline guchar bit_inverse(guchar chr) {
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
