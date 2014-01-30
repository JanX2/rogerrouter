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
 * TODO: Allow seeking/pause/resume, UI support
 */

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <gio/gio.h>

#include <speex/speex_callbacks.h>

#include <libroutermanager/voxplay.h>
#include <libroutermanager/audio.h>

#define MAX_FRAME_SIZE 2000

struct vox_playback {
	gchar *data;
	gsize len;

	GThread *thread;
	void *speex;
	SpeexBits bits;
	struct audio *audio;
	gpointer priv;
	GCancellable *cancel;
	gboolean pause;

	void (*cb)(gpointer priv, gpointer fraction);
	gpointer cb_data;
};

/**
 * \brief Main playback thread
 * \param user_data audio private pointer
 * \return NULL
 */
static gpointer playback_thread(gpointer user_data)
{
	struct vox_playback *playback = user_data;
	short output[MAX_FRAME_SIZE];
	spx_int32_t frame_size;
	int j;
	unsigned char bytes = 0;
	gsize offset = 0;
	int cnt = 0;
	int len_cnt = 0;

	speex_decoder_ctl(playback->speex, SPEEX_GET_FRAME_SIZE, &frame_size);
	g_debug("Frame Size: %d", frame_size);
	g_debug("Playback len: %" G_GSIZE_FORMAT, playback->len);

	while (offset < playback->len && !g_cancellable_is_cancelled(playback->cancel)) {
		bytes = playback->data[offset];
		offset++;

		if (bytes != 0x26) {
			continue;
		}

		offset += bytes;
		cnt++;
	}

	g_debug("cnt: %d", cnt);
	g_debug("Seconds: %f", (float)(frame_size * cnt) / (float)8000);

	len_cnt = cnt;
	offset = 0;
	cnt = 0;

	while (offset < playback->len && !g_cancellable_is_cancelled(playback->cancel)) {
		if (playback->pause) {
			g_usleep(100);
			continue;
		}

		bytes = playback->data[offset];
		offset++;

		if (bytes != 0x26) {
			continue;
		}

		speex_bits_read_from(&playback->bits, (char *) playback->data + offset, bytes);
		offset += bytes;

		for (j = 0; j != 2; j++) {
			int ret;

			ret = speex_decode_int(playback->speex, &playback->bits, output);
			if (ret == -1) {
				break;
			} else if (ret == -2) {
				g_warning("Decoding error: corrupted stream?");
				break;
			}

			if (speex_bits_remaining(&playback->bits) < 0) {
				g_warning("Decoding overflow: corrupted stream?");
				break;
			}
		}

		playback->audio->write(playback->priv, (unsigned char *) output, frame_size * sizeof(short));
		cnt++;
		playback->cb(playback->cb_data, GINT_TO_POINTER(cnt * 100 / len_cnt));//(float)cnt / (float)len_cnt);
	}

	speex_decoder_destroy(playback->speex);
	speex_bits_destroy(&playback->bits);

	playback->audio->close(playback->priv, FALSE);
	playback->audio = NULL;
	playback->pause = FALSE;

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

	playback = g_slice_new(struct vox_playback);
	playback->data = data;
	playback->len = len;

	playback->audio = audio_get_default();
	if (!playback->audio) {
		g_warning("No audio device");
		g_slice_free(struct vox_playback, playback);
		return NULL;
	}

	/* open audio device */
	playback->priv = playback->audio->open();
	if (!playback->priv) {
		g_debug("Could not open audio device");
		g_slice_free(struct vox_playback, playback);
		return NULL;
	}

	/* Initialize speex */
	speex_bits_init(&playback->bits);

	mode = speex_lib_get_mode(0);

	playback->speex = speex_decoder_init(mode);
	if (!playback->speex) {
		g_warning("Decoder initialization failed.");
		playback->audio->close(playback->priv, FALSE);

		g_slice_free(struct vox_playback, playback);
		return NULL;
	}

	rate = 8000;
	speex_decoder_ctl(playback->speex, SPEEX_SET_SAMPLING_RATE, &rate);

	playback->cancel = g_cancellable_new();

	playback->cb = vox_cb;
	playback->cb_data = priv;

	/* Start playback thread */
	playback->pause = FALSE;
	playback->thread = g_thread_new("play vox", playback_thread, playback);

	return playback;
}

gboolean vox_playpause(gpointer vox_data)
{
	struct vox_playback *playback = vox_data;

	if (!playback) {
		return FALSE;
	}

	playback->pause = !playback->pause;

	return !playback->pause;
}

/**
 * \brief Stop vox playback if it is still running
 * \param vox_data pointer to vox playback data
 */
void vox_stop(gpointer vox_data)
{
	struct vox_playback *playback = vox_data;

	if (!playback) {
		return;
	}

	playback->pause = TRUE;
	g_cancellable_cancel(playback->cancel);
	g_thread_join(playback->thread);

	g_object_unref(playback->cancel);

	g_slice_free(struct vox_playback, playback);
}
