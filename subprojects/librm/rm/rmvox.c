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

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <gio/gio.h>

#include <speex/speex_callbacks.h>
#include <sndfile.h>

#include <rm/rmvox.h>
#include <rm/rmaudio.h>
#include <rm/rmobjectemit.h>
#include <rm/rmmain.h>
#include <rm/rmprofile.h>
#include <rm/rmfile.h>

/**
 * SECTION:rmvox
 * @title: RmVox
 * @short_description: Vox playback
 * @stability: Stable
 *
 * FRITZ!Box voice box playback function (speex)
 */

#define MAX_FRAME_SIZE 2000

/** Private vox playback structure */
typedef struct _RmVoxPlayback {
	/** Vox data buffer */
	gchar *data;
	/** Length of vox data */
	gsize len;
	/** Pointer to thread structure */
	GThread *thread;
	/** Speex structure */
	gpointer speex;
	/** audio device */
	RmAudio *audio;
	/** audio private data */
	gpointer audio_priv;
	/** cancellable object for playback thread */
	GCancellable *cancel;
	/** playback state (pause/playing) */
	gboolean state;
	/** number of frame count */
	gint num_cnt;
	/** current playback data offset */
	gsize offset;
	/** current playback frame count */
	gint cnt;
	/** Current fraction */
	gint fraction;
	/** Current seconds */
	gfloat seconds;

	SNDFILE *sf;
	SF_INFO info;
} RmVoxPlayback;

/**
 * rm_vox_playback_thread:
 * @user_data audio private pointer:
 *
 * Main playback thread
 *
 * Returns: %NULL
 */
static gpointer rm_vox_playback_thread(gpointer user_data)
{
	RmVoxPlayback *playback = user_data;
	spx_int32_t frame_size;
	SpeexBits bits;
	gshort output[MAX_FRAME_SIZE];
	gint j;
	gint max_cnt = 0;
	guchar bytes = 0;

	/* open audio device */
	playback->audio_priv = rm_audio_open(playback->audio, NULL);
	if (!playback->audio_priv) {
		g_debug("%s(): Could not open audio device", __FUNCTION__);
		g_slice_free(RmVoxPlayback, playback);

		//g_set_error(error, RM_ERROR, RM_ERROR_AUDIO, "%s", "Could not open audio device");


		return NULL;
	}

	speex_bits_init(&bits);

	/* Get frame rate */
	speex_decoder_ctl(playback->speex, SPEEX_GET_FRAME_SIZE, &frame_size);

	/* Loop through data in order to calculate frame counts */
	playback->offset = 0;
	playback->cnt = 0;

	while (playback->offset < playback->len && !g_cancellable_is_cancelled(playback->cancel)) {
		bytes = playback->data[playback->offset];
		playback->offset++;

		if (bytes != 0x26) {
			continue;
		}

		playback->offset += bytes;
		playback->cnt++;
	}

	playback->num_cnt = playback->cnt;

#ifdef VOX_DEBUG
	g_debug("%s(): cnt = %d, seconds = %f", __FUNCTION__, playback->num_cnt, (float)(frame_size * playback->cnt) / (float)8000);
#endif

	max_cnt = playback->cnt;
	playback->offset = 0;
	playback->cnt = 0;

	/* Start playback */
	while (playback->offset < playback->len && !g_cancellable_is_cancelled(playback->cancel)) {
		if (playback->state) {
			/* We are in pause state, delay the loop to prevent high cpu load */
			g_usleep(100);
			continue;
		}

		/* Read number of bytes */
		bytes = playback->data[playback->offset];
		playback->offset++;

		if (bytes != 0x26) {
			continue;
		}

		/* initializes bit stream */
		speex_bits_read_from(&bits, (char *) playback->data + playback->offset, bytes);
		playback->offset += bytes;

		/* Deocde data */
		for (j = 0; j != 2; j++) {
			gint ret;

			ret = speex_decode_int(playback->speex, &bits, output);
			if (ret == -1) {
				break;
			} else if (ret == -2) {
				g_warning("%s(): Decoding error: corrupted stream?", __FUNCTION__);
				break;
			}

			if (speex_bits_remaining(&bits) < 0) {
				g_warning("%s(): Decoding overflow: corrupted stream?", __FUNCTION__);
				break;
			}
		}

		/* Write data to audio device */
		rm_audio_write(playback->audio, playback->audio_priv, (guchar *) output, frame_size * sizeof(gshort));

		/* Increment current frame count and update ui */
		playback->cnt++;

		playback->fraction = playback->cnt * 100 / max_cnt;
		playback->seconds = (gfloat)((gfloat)(frame_size * playback->cnt) / (gfloat)8000);

	}
#ifdef VOX_DEBUG
	g_debug("%s(): End of vox", __FUNCTION__);
#endif

	speex_bits_destroy(&bits);
	playback->thread = NULL;
	rm_audio_close(playback->audio, playback->audio_priv);

	return NULL;
}

/**
 * rm_vox_sf_playback_thread:
 * @user_data audio private pointer:
 *
 * Main playback thread
 *
 * Returns: %NULL
 */
static gpointer rm_vox_sf_playback_thread(gpointer user_data)
{
	RmVoxPlayback *playback = user_data;
	gint num_read;
	gshort buffer[80];

	/* open audio device */
	playback->audio_priv = rm_audio_open(playback->audio, NULL);
	if (!playback->audio_priv) {
		g_debug("%s(): Could not open audio device", __FUNCTION__);
		g_slice_free(RmVoxPlayback, playback);

		return NULL;
	}

	playback->offset = 0;
	playback->cnt = 0;
	playback->num_cnt = playback->info.frames;

	gint len = 80;

	sf_seek(playback->sf, 0, SEEK_SET);

	/* Start playback */
	while (playback->cnt < playback->num_cnt && !g_cancellable_is_cancelled(playback->cancel)) {
		if (playback->state) {
			/* We are in pause state, delay the loop to prevent high cpu load */
			g_usleep(100);
			continue;
		}

		num_read = sf_read_short(playback->sf, buffer, len);
		/* Write data to audio device */
		rm_audio_write(playback->audio, playback->audio_priv, (guchar *) buffer, num_read * 2);

		playback->cnt += 80;

		playback->fraction = playback->cnt * 100 / playback->num_cnt;
		playback->seconds = (gfloat)((gfloat)(playback->cnt) / (gfloat)8000);
	}

	playback->thread = NULL;
	rm_audio_close(playback->audio, playback->audio_priv);

	return NULL;
}

/**
 * rm_vox_play:
 * @vox_data: pointer to vox playback data
 *
 * Play voicebox message file.
 *
 * Returns: %TRUE on playback started, %FALSE otherwise
 */
gboolean rm_vox_play(gpointer vox_data)
{
	RmVoxPlayback *playback = vox_data;

	if (!playback) {
		return FALSE;
	}

	/* Pause music, cancel cancellable and join thread */
	if (playback->thread) {
		g_cancellable_cancel(playback->cancel);
		g_thread_join(playback->thread);
	}

	g_cancellable_reset(playback->cancel);

	/* Reset internal values */
	playback->fraction = 0;
	playback->seconds = 0;

	/* Start playback thread */
	playback->state = FALSE;

	if (playback->speex) {
		playback->thread = g_thread_new("play vox", rm_vox_playback_thread, playback);
	} else {
		playback->thread = g_thread_new("play vox", rm_vox_sf_playback_thread, playback);
	}

	return playback->thread != NULL;
}

/**
 * rm_vox_playpause:
 * @vox_data: pointer to vox playback data
 *
 * Toggle play/pause state.
 *
 * Returns: %TRUE if pause, %FALSE on playing
 */
gboolean rm_vox_playpause(gpointer vox_data)
{
	RmVoxPlayback *playback = vox_data;

	if (!playback) {
		return FALSE;
	}

	playback->state = !playback->state;

	return !playback->state;
}

/**
 * rm_vox_shutdown:
 * @vox_data: pointer to vox playback data
 *
 * Stop vox playback if it is still running
 *
 * Returns: %TRUE if playback has been stop, %FALSE on error
 */
gboolean rm_vox_shutdown(gpointer vox_data)
{
	RmVoxPlayback *playback = vox_data;

	/* Safety check */
	if (!playback) {
		return FALSE;
	}

	/* Pause music, cancel cancellable and join thread */
	playback->state = TRUE;

	if (playback->thread) {
		g_cancellable_cancel(playback->cancel);
		g_thread_join(playback->thread);
	}

	if (playback->speex) {
		/* Destroy speex decoder */
		speex_decoder_destroy(playback->speex);
	}
	if (playback->sf) {
		sf_close(playback->sf);
		playback->sf = NULL;
	}

	/* Close audio device */
	playback->audio = NULL;

	/* Unref cancellable and free structure */
	g_object_unref(playback->cancel);
	g_slice_free(RmVoxPlayback, playback);

	return TRUE;
}

/**
 * rm_vox_seek:
 * @vox_data: pointer to vox playback data
 * @pos: position fraction
 *
 * Seek within vox stream.
 *
 * Returns: %TRUE on seek success, %FALSE on error
 */
gboolean rm_vox_seek(gpointer vox_data, gdouble pos)
{
	RmVoxPlayback *playback = vox_data;
	spx_int32_t frame_size;
	gint cnt;
	gint cur_cnt = 0;
	gsize offset = 0;
	guchar bytes = 0;

#ifdef VOX_DEBUG
	g_debug("%s(): seek called", __FUNCTION__);
#endif

	if (!playback) {
		return FALSE;
	}

	if (playback->sf) {
		sf_seek(playback->sf, pos * playback->num_cnt, SEEK_SET);
		playback->cnt = pos;
		return TRUE;
	}

	/* Get frame rate */
	speex_decoder_ctl(playback->speex, SPEEX_GET_FRAME_SIZE, &frame_size);

	//g_debug("num_cnt: %d, pos: %f", playback->num_cnt, pos);
	cnt = playback->num_cnt * pos;

	/* Loop through data in order to calculate frame counts */
	while (offset < playback->len && !g_cancellable_is_cancelled(playback->cancel)) {
		bytes = playback->data[offset];
		offset++;

		if (bytes != 0x26) {
			continue;
		}

		offset += bytes;
		cur_cnt++;

#ifdef VOX_DEBUG
		g_debug("%s(): cur_cnt = %d, cnt = %d", __FUNCTION__, cur_cnt, cnt);
#endif
		/* If we have the requested count, overwrite playback data */
		if (cur_cnt == cnt) {
			playback->offset = offset;
			playback->cnt = cnt;

			return TRUE;
		}
	}

	return FALSE;
}

/**
 * rm_vox_get_fraction:
 * @vox_data: internal vox playback structure
 *
 * Get current fraction of playback slider.
 *
 * Returns: current fraction
 */
gint rm_vox_get_fraction(gpointer vox_data)
{
	RmVoxPlayback *playback = vox_data;

	return playback->fraction;
}

/**
 * rm_vox_get_seconds:
 * @vox_data: internal vox playback structure
 *
 * Get current seconds of playback.
 *
 * Returns: current seconds
 */
gfloat rm_vox_get_seconds(gpointer vox_data)
{
	RmVoxPlayback *playback = vox_data;

	return playback->seconds;
}

/**
 * rm_vox_init:
 * @data: voice data
 * @len: length of voice data
 * @error: a #GError
 *
 * Initialize vox playback structure.
 *
 * Returns: vox play structure
 */
gpointer rm_vox_init(gchar *data, gsize len, GError **error)
{
	RmVoxPlayback *playback;
	const SpeexMode *mode;
	spx_int32_t rate = 0;

	/* Create internal structue and store data pointer and data length */
	playback = g_slice_new0(RmVoxPlayback);
	playback->data = data;
	playback->len = len;

	/* Get default audio device */
	playback->audio = rm_profile_get_audio(rm_profile_get_active());
	if (!playback->audio) {
		g_warning("%s(): No audio device", __FUNCTION__);
		g_slice_free(RmVoxPlayback, playback);

		g_set_error(error, RM_ERROR, RM_ERROR_AUDIO, "%s", "No audio device");


		return NULL;
	}

	if (!g_ascii_strncasecmp(data, "RIFF", 4)) {
		rm_file_save("vox.wav", data, len);
		playback->sf = sf_open("vox.wav", SFM_READ, &playback->info);

		if (playback->info.format != (SF_FORMAT_WAV | SF_FORMAT_PCM_16)) {
			g_debug("%s(): Not a valid WAVE file", __FUNCTION__);
			return NULL;
		}
	} else {

		/* Initialize speex decoder */
		mode = speex_lib_get_mode(0);

		playback->speex = speex_decoder_init(mode);
		if (!playback->speex) {
			g_warning("%s(): Decoder initialization failed.", __FUNCTION__);

			g_slice_free(RmVoxPlayback, playback);

			g_set_error(error, RM_ERROR, RM_ERROR_AUDIO, "%s", "Decoder initialization failed.");

			return NULL;
		}

		rate = 8000;
		speex_decoder_ctl(playback->speex, SPEEX_SET_SAMPLING_RATE, &rate);
	}

	/* Create cancellable */
	playback->cancel = g_cancellable_new();

	return playback;
}
