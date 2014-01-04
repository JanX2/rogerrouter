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

#include <glib.h>

#include <libroutermanager/audio.h>
#include <libroutermanager/call.h>
#include <libroutermanager/connection.h>

#include "isdn-convert.h"

#define QUANT_MASK (0xF)
#define SEG_SHIFT (4)
#define SEG_MASK (0x70)
#define SIGN_BIT (0x80)

#define RING_PERIOD 4
#define RING_SHORT_PERIOD 0.044
#define RING_FADE_LENGTH 0.003
#define RING_LENGTH 1.3
#define RING_SHORT_LENGTH (RING_SHORT_PERIOD/2)
#define RING_FREQUENCY 1300
#define RINGING_FREQUENCY 425

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};
static unsigned char *audio_lut_generate = NULL;
static gboolean effect_thread_stop = TRUE;

/**
 * \brief Small index search
 * \return index
 */
static int search(int val, short *table, int size)
{
	int index;

	for (index = 0; index < size; index++) {
		if (val <= *table++) {
			return index;
		}
	}

	return size;
}

/**
 * \brief Convert linear to alaw value
 * \param val linear value
 * \return alaw value
 */
static unsigned char linear2alaw(int val)
{
	int mask;
	int seg;
	unsigned char val_a;

	if (val >= 0) {
		mask = 0xD5;
	} else {
		mask = 0x55;
		val = -val - 8;
	}

	seg = search(val, seg_end, 8);

	if (seg >= 8) {
		return (0x7F ^ mask);
	} else {
		val_a = seg << SEG_SHIFT;
		if (seg < 2) {
			val_a |= (val >> 4) & QUANT_MASK;
		} else {
			val_a |= (val >> (seg + 3)) & QUANT_MASK;
		}

		return (val_a ^ mask);
	}
}

/**
 * \brief Generate fx sound
 * \param type sound type
 * \param index index
 * \param seconds seconds
 * \return current byte
 */
unsigned char fx_generate(gint type, int index, double seconds)
{
	double rest;
	double rest2;
	double factor;
	double factor2;
	unsigned char x = 0;

	switch (type) {
	case CONNECTION_TYPE_INCOMING:
		/* 4 sec period */
		rest = fmod(seconds, RING_PERIOD);
		/* short period */
		rest2 = fmod(seconds, RING_SHORT_PERIOD);

		if (rest < RING_FADE_LENGTH) {
			/* Fade in */
			factor = -cos(rest * M_PI / RING_FADE_LENGTH) / 2 + 0.5;
		} else if (rest > RING_LENGTH - RING_FADE_LENGTH && rest < RING_LENGTH) {
			/* Fade out */
			factor = -cos((RING_LENGTH - rest) * 2 * M_PI / (RING_FADE_LENGTH * 2)) / 2 + 0.5;
		} else if (rest >= RING_LENGTH) {
			/* Pause */
			factor = 0;
		} else {
			/* Beep */
			factor = 1;
		}

		if (rest2 > RING_SHORT_PERIOD - 0.5 * RING_FADE_LENGTH) {
			/* Fade in short (1/1) */
			factor2 = -sin((RING_SHORT_PERIOD - rest2) * 2 * M_PI / (RING_FADE_LENGTH * 2)) / 2 + 0.5;
		} else if (rest2 < 0.5 * RING_FADE_LENGTH) {
			/* Fade in short (2/2) */
			factor2 = sin(rest2 * 2 * M_PI / (RING_FADE_LENGTH * 2)) / 2 + 0.5;
		} else if (rest2 > RING_SHORT_LENGTH - 0.5 * RING_FADE_LENGTH && rest2 < RING_SHORT_LENGTH + 0.5 * RING_FADE_LENGTH) {
			factor2 = -sin((rest2 - RING_SHORT_LENGTH) * 2 * M_PI / (RING_FADE_LENGTH * 2)) / 2 + 0.5;
		} else if (rest2 <= RING_SHORT_LENGTH) {
			/* Beep */
			factor2 = 1;
		} else {
			/* Pause */
			factor2 = 0;
		}

		factor = factor * factor2;

		x = audio_lut_generate[(int)(sin(seconds * 2 * M_PI * RING_FREQUENCY) * factor * 127.5 + 127.5)];
		break;
	case CONNECTION_TYPE_OUTGOING:
		/* waiting for the other end to pick up the phone */
		rest = fmod(seconds, 5);

		if (rest >= 2 && rest < 3) {
			/* beep */
			x = audio_lut_generate[(int)(sin(seconds * 2 * M_PI * RINGING_FREQUENCY) * 127.5 + 127.5)];
		} else {
			/* pause */
			x = audio_lut_generate[128];
		}
		break;
	default:
		g_warning("Unknown effect %d", type);
		x = 0;
		break;
	}

	return x;
}

/**
 * \brief Ringtone thread
 * \param user_data user data
 * \return error code
 */
static gpointer ringtone_thread(gpointer user_data)
{
	gint type = GPOINTER_TO_INT(user_data);
	unsigned int index;
	unsigned char alaw_buffer[4096];
	unsigned int alaw_count = 0;
	unsigned char snd_buffer[12 * 4096];
	unsigned int snd_count = 0;
	unsigned long effect_pos = 0;
	void *priv = NULL;
	struct audio *audio = audio_get_default();
	int counter = 0;

	if (audio == NULL) {
		return NULL;
	}

	priv = audio->open();
	if (!priv) {
		return NULL;
	}

	while (!effect_thread_stop) {
		for (index = 0; index < sizeof(alaw_buffer) / 4; ++index) {
			alaw_buffer[index] = bit_inverse(fx_generate(type, 0, effect_pos++ / 8000.0));
		}
		alaw_count = sizeof(alaw_buffer) / 4;

		counter++;
		//if (counter == 0x20) {
		//	alaw_count = 0;
		//}

		if (alaw_count == 0) {
			g_debug("End-Of-File reached, stopping playback");

			effect_thread_stop = TRUE;
			break;
		}

		convert_isdn_to_audio(NULL, alaw_buffer, alaw_count, snd_buffer, &snd_count, NULL);
		audio->write(priv, snd_buffer, snd_count);
	}

	audio->close(priv, FALSE);

	return NULL;
}

/**
 * \brief Create look-up table
 * \param lut_generate fx generate lut
 * \return error code
 */
static int make_lut(unsigned char **lut_generate)
{
	int index;

	if (!(*lut_generate = (unsigned char *) malloc(256))) {
		return -1;
	}

	/* Calculation */
	for (index = 0; index < 256; index++) {
		(*lut_generate)[index] = linear2alaw((index - 128) * 256);
	}

	return 0;
}

/**
 * \brief Play sounds
 * \param type call type
 */
void ringtone_play(gint type)
{
	if (audio_lut_generate == NULL) {
		make_lut(&audio_lut_generate);
	}

	effect_thread_stop = FALSE;
	g_thread_new("ringtone thread", ringtone_thread, GINT_TO_POINTER(type));
}

/**
 * \brief Stop effect handler
 */
void ringtone_stop(void)
{
	effect_thread_stop = TRUE;
}
