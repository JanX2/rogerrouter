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
 * \file isdn-convert.c
 * \brief Audio conversion function
 */

#include "isdn-convert.h"
#include "phone.h"

static unsigned char *lut_in = NULL;
static unsigned char *lut_out = NULL;
static unsigned char *lut_analyze = NULL;
static short *lut_a2s = NULL;
signed char linear16_2_law[65536];
unsigned short law_2_linear16[256];

/**
 * \brief Get sign and magnitude
 * \param sample alaw sample
 * \param sign sign
 * \param mag magnitude
 */
static inline void alaw_get_sign_mag(short sample, unsigned *sign, unsigned *mag)
{
	if (sample < 0) {
		*mag = -sample;
		*sign = 0;
	} else {
		*mag = sample;
		*sign = 0x80;
	}
}

/**
 * \brief Convert linear to alaw value
 * \param sample linear sample value
 * \return alaw sample value
 */
static unsigned char linear2alaw(short sample)
{
	unsigned sign, exponent, mantissa, mag;
	unsigned char alaw_byte;
	static const unsigned exp_lut[128] = {
		1, 1, 2, 2, 3, 3, 3, 3,
		4, 4, 4, 4, 4, 4, 4, 4,
		5, 5, 5, 5, 5, 5, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5,
		6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6,
		7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7
	};

	alaw_get_sign_mag(sample, &sign, &mag);
	if (mag > 32767) {
		mag = 32767;
	}

	exponent = exp_lut[(mag >> 8) & 0x7F];
	mantissa = (mag >> (exponent + 3)) & 0x0F;

	if (mag < 0x100) {
		exponent = 0;
	}

	alaw_byte = (unsigned char)(sign | (exponent << 4) | mantissa);
	alaw_byte ^= 0x55;

	return alaw_byte;
}

/**
 * \brief Convert alaw value to linear value
 * \param alaw_byte alaw value
 * \return linear value
 */
static short alaw2linear(unsigned char alaw_byte)
{
	int t;
	int seg;

	alaw_byte ^= 0x55;
	t = alaw_byte & 0x7F;

	if (t < 16) {
		t = (t << 4) + 8;
	} else {
		seg = (t >> 4) & 0x07;
		t = ((t & 0x0F) << 4) + 0x108;
		t <<= seg - 1;
	}

	return ((alaw_byte & 0x80) ? t : - t);
}

/**
 * \brief Create lookup table buffer
 */
void create_table_buffer(void)
{
	signed char *_linear16_2_law = (signed char *)&linear16_2_law[32768];
	long index;
	int buf_size_in = 0;
	int buf_size_out = 0;
	int sample;
	unsigned int audio_sample_size_in = 2;
	unsigned int audio_sample_size_out = 2;

	if (lut_in != NULL) {
		return;
	}

	for (index = 0; index < 65535; index++) {
		_linear16_2_law[index - 32768] = bit_inverse(linear2alaw((short) index - 32768));
	}

	for (index = 0; index < 256; index++) {
		law_2_linear16[index] = alaw2linear(bit_inverse(index)) & 0xFFFF;
	}

	buf_size_in = audio_sample_size_in * 256;
	lut_in = malloc(buf_size_in);

	for (index = 0; index < buf_size_in; index += audio_sample_size_in) {
		sample = alaw2linear(bit_inverse((unsigned char)(index / 2)));
		lut_in[index + 0] = (unsigned char)(sample & 0xFF);
		lut_in[index + 1] = (unsigned char)(sample >> 8 & 0xFF);
	}

	buf_size_out = (1 + (audio_sample_size_out - 1) * 255) * 256;
	lut_out = malloc(buf_size_out);

	for (index = 0; index < buf_size_out; index++) {
		lut_out[index] = bit_inverse(linear2alaw((int)(signed char)(index >> 8) << 8 | (int)(index & 0xFF)));
	}

	lut_analyze = malloc(256);
	lut_a2s = malloc(256 * sizeof(short));

	for (index = 0; index < 256; index++) {
		lut_analyze[index] = (unsigned char)((alaw2linear((unsigned char)bit_inverse(index)) / 256 & 0xFF) ^ 0x80);

		lut_a2s[index] = alaw2linear(bit_inverse(index));
	}
}

/**
 * \brief Get line level input value
 * \return line level input state
 */
double get_line_level_in(struct capi_connection *connection)
{
	return connection->line_level_in_state;
}

/**
 * \brief Get line level output value
 * \return line level output state
 */
double get_line_level_out(struct capi_connection *connection)
{
	return connection->line_level_out_state;
}

/**
 * \brief Convert isdn format to audio format
 * \param in_buf input buffer
 * \param in_buf_len length of input buffer
 * \param out_buffer output buffer
 * \param out_buf_len pointer to output buffer len
 */
void convert_isdn_to_audio(struct capi_connection *connection, unsigned char *in_buf, unsigned int in_buf_len, unsigned char *out_buf, unsigned int *out_buf_len, short *rec_buf)
{
	struct recorder *recorder = &connection->recorder;
	unsigned int index;
	unsigned int to_process;
	unsigned int out_ptr = 0;
	unsigned int j;
	unsigned char in_byte;
	int sample;
	double ratio_in = 1.0f;
	double ll_ratio;
	int max = 0;

	out_ptr = 0;

	for (index = 0; index < in_buf_len; index++) {
		in_byte = in_buf[index];

		if (recorder != NULL && rec_buf != NULL) {
			rec_buf[index] = recorder->file ? lut_a2s[in_byte] : 0;
		}

		sample = lut_analyze[in_byte];
		if (abs((int) sample - 128) > max) {
			max = abs((int) sample - 128);
		}

		to_process = (int) floor((double)(index + 1) * ratio_in) - (int) floor((double) index * ratio_in);

		for (j = 0; j < to_process; j++) {
			out_buf[out_ptr++] = lut_in[(int) in_byte * 2];
			out_buf[out_ptr++] = lut_in[(int) in_byte * 2 + 1];
		}
	}

	/* Record data */
	if (recorder != NULL && rec_buf != NULL) {
		recording_write(recorder, rec_buf, in_buf_len, RECORDING_REMOTE);
	}

	ll_ratio = in_buf_len / 400.0f;
	if (ll_ratio > 1.0) {
		ll_ratio = 1.0;
	}

	if (connection) {
		connection->line_level_in_state = connection->line_level_in_state * (1.0 - ll_ratio) + ((double) max / 128) * ll_ratio;
	}

	*out_buf_len = out_ptr;
}

/**
 * \brief Convert audio format to isdn format
 * \param connection active capi connection
 * \param in_buf input buffer
 * \param in_buf_len length of input buffer
 * \param out_buffer output buffer
 * \param out_buf_len pointer to output buffer len
 */
void convert_audio_to_isdn(struct capi_connection *connection, unsigned char *in_buf, unsigned int in_buf_len, unsigned char *out_buf, unsigned int *out_buf_len, short *rec_buf)
{
	unsigned int index;
	unsigned int to_process;
	unsigned int out_ptr = 0;
	unsigned int j;
	unsigned char sample;
	double ratio_out = 1.0f;
	double ll_ratio;
	int max = 0;
	unsigned char sample_u8;

	out_ptr = 0;

	for (index = 0; index < in_buf_len; index += 2) {
		to_process = (int) floor((double)(out_ptr + 1) * ratio_out) - (int) floor((double) out_ptr * ratio_out);

		for (j = 0; j < to_process; j++) {
			int tmp = (int)(in_buf[index]) | ((int)(in_buf[index + 1]) << 8);
			sample = lut_out[tmp];

			if (connection != NULL && connection->mute != 0) {
				sample = lut_out[0];
			}

			sample_u8 = lut_analyze[sample];
			if (abs((int) sample_u8 - 128) > max) {
				max = abs((int) sample_u8 - 128);
			}

			if (connection != NULL) {
				rec_buf[out_ptr] = connection->recorder.file ? lut_a2s[sample] : 0;
			} else {
				rec_buf[out_ptr] = 0;
			}

			out_buf[out_ptr] = sample;
			out_ptr++;
		}
	}

	/* Record data */
	if (connection != NULL && connection->recorder.file != NULL && rec_buf != NULL) {
		recording_write(&connection->recorder, rec_buf, out_ptr, RECORDING_LOCAL);
	}

	ll_ratio = out_ptr / 400.0f;
	if (ll_ratio > 1.0) {
		ll_ratio = 1.0;
	}

	connection->line_level_out_state = connection->line_level_out_state * (1.0 - ll_ratio) + ((double) max / 128) * ll_ratio;

	*out_buf_len = out_ptr;
}
