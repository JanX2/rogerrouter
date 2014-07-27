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

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <gio/gio.h>

#include <speex/speex_callbacks.h>

#include <libroutermanager/voxplay.h>
#include <libroutermanager/audio.h>
#include <libroutermanager/appobject-emit.h>

#define MAX_FRAME_SIZE 2000

/** Private vox playback structure */
struct vox_playback {
	/** Vox data buffer */
	gchar *data;
	/** Length of vox data */
	gsize len;
	/** Pointer to thread structure */
	GThread *thread;
	/** Speex structure */
	gpointer speex;
	/** audio device */
	struct audio *audio;
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
	/** UI callback function */
	void (*ui_cb)(gpointer priv, gpointer fraction);
	/** UI private data */
	gpointer ui_cb_data;
};

/**
 * \brief Main playback thread
 * \param user_data audio private pointer
 * \return NULL
 */
static gpointer playback_thread(gpointer user_data)
{
	struct vox_playback *playback = user_data;
	spx_int32_t frame_size;
	SpeexBits bits;
	gshort output[MAX_FRAME_SIZE];
	gint j;
	gint max_cnt = 0;
	guchar bytes = 0;

	speex_bits_init(&bits);

	/* Get frame rate */
	speex_decoder_ctl(playback->speex, SPEEX_GET_FRAME_SIZE, &frame_size);

	/* Loop through data in order to calculate frame counts */
	playback->offset = 0;
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
	g_debug("cnt: %d", playback->num_cnt);
	g_debug("Seconds: %f", (float)(frame_size * playback->cnt) / (float)8000);
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
				g_warning("Decoding error: corrupted stream?");
				break;
			}

			if (speex_bits_remaining(&bits) < 0) {
				g_warning("Decoding overflow: corrupted stream?");
				break;
			}
		}

		/* Write data to audio device */
		playback->audio->write(playback->audio_priv, (guchar *) output, frame_size * sizeof(gshort));

		/* Increment current frame count and update ui */
		playback->cnt++;
		playback->ui_cb(playback->ui_cb_data, GINT_TO_POINTER(playback->cnt * 100 / max_cnt));
	}

	speex_bits_destroy(&bits);


	return NULL;
}

/**
 * \brief Play voicebox message file
 * \param data voice data
 * \param len length of voice data
 * \return vox play structure
 */
gpointer vox_play(gchar *data, gsize len, void (*vox_cb)(gpointer priv, gpointer fraction), gpointer priv)
{
	struct vox_playback *playback;
	const SpeexMode *mode;
	spx_int32_t rate = 0;

	/* Safety check */
	g_return_val_if_fail(vox_cb != NULL, NULL);

	/* Create internal structue and store data pointer and data length */
	playback = g_slice_new(struct vox_playback);
	playback->data = data;
	playback->len = len;

	/* Get default audio device */
	playback->audio = audio_get_default();
	if (!playback->audio) {
		g_warning("No audio device");
		g_slice_free(struct vox_playback, playback);
		emit_message(0, "No audio device");

		return NULL;
	}

	/* open audio device */
	playback->audio_priv = playback->audio->open();
	if (!playback->audio_priv) {
		g_debug("Could not open audio device");
		g_slice_free(struct vox_playback, playback);
		emit_message(0, "Could not open audio device");

		return NULL;
	}

	/* Initialize speex decoder */
	mode = speex_lib_get_mode(0);

	playback->speex = speex_decoder_init(mode);
	if (!playback->speex) {
		g_warning("Decoder initialization failed.");
		playback->audio->close(playback->audio_priv, FALSE);

		g_slice_free(struct vox_playback, playback);
		emit_message(0, "Decoder initialization failed.");

		return NULL;
	}

	rate = 8000;
	speex_decoder_ctl(playback->speex, SPEEX_SET_SAMPLING_RATE, &rate);

	/* Create cancellable */
	playback->cancel = g_cancellable_new();

	/* Store callback information */
	playback->ui_cb = vox_cb;
	playback->ui_cb_data = priv;

	/* Start playback thread */
	playback->state = FALSE;
	playback->thread = g_thread_new("play vox", playback_thread, playback);

	return playback;
}

/**
 * \brief Toggle play/pause state
 * \param vox_data pointer to vox playback data
 * \return TRUE if pause, FALSE on playing
 */
gboolean vox_playpause(gpointer vox_data)
{
	struct vox_playback *playback = vox_data;

	if (!playback) {
		return FALSE;
	}

	playback->state = !playback->state;

	return !playback->state;
}

/**
 * \brief Stop vox playback if it is still running
 * \param vox_data pointer to vox playback data
 */
void vox_stop(gpointer vox_data)
{
	struct vox_playback *playback = vox_data;

	/* Safety check */
	if (!playback) {
		return;
	}

	/* Pause music, cancel cancellable and join thread */
	playback->state = TRUE;
	g_cancellable_cancel(playback->cancel);
	g_thread_join(playback->thread);

	/* Destroy speex decoder */
	speex_decoder_destroy(playback->speex);

	/* Close audio device */
	playback->audio->close(playback->audio_priv, FALSE);
	playback->audio = NULL;

	/* Unref cancellable and free structure */
	g_object_unref(playback->cancel);
	g_slice_free(struct vox_playback, playback);
}

/**
 * \brief Seek within vox stream
 * \param vox_data pointer to vox playback data
 * \param pos position fraction
 */
void vox_seek(gpointer vox_data, gdouble pos)
{
	struct vox_playback *playback = vox_data;
	spx_int32_t frame_size;
	gint cnt;
	gint cur_cnt = 0;
	gsize offset = 0;
	guchar bytes = 0;

	if (!playback) {
		return;
	}

	/* Get frame rate */
	speex_decoder_ctl(playback->speex, SPEEX_GET_FRAME_SIZE, &frame_size);

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

		/* If we have the requested count, overwrite playback data */
		if (cur_cnt == cnt) {
			playback->offset = offset;
			playback->cnt = cnt;
			break;
		}
	}
}
