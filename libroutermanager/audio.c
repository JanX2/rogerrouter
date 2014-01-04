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
 * \TODO List
 * - Handle multiple audio plugins
 * - Move audio plugin information into audio private data, in order to switch device while playing
 */

#include <glib.h>

#include <libroutermanager/audio.h>

/** global pointer to current used audio plugin */
static struct audio *internal_audio = NULL;

/**
 * \brief Open current audio plugin
 * \return private audio data pointer or NULL on error
 */
gpointer audio_open(void)
{
	if (!internal_audio) {
		return NULL;
	}

	return internal_audio->open();
}

/**
 * \brief Read of audio plugin
 * \param audio_priv private audio data (see audio_open)
 * \param data data pointer
 * \param size number of bytes to read
 * \return number of bytes read or -1 on error
 */
gsize audio_read(gpointer audio_priv, guchar *data, gsize size)
{
	if (!internal_audio) {
		return -1;
	}

	return internal_audio->read(audio_priv, data, size);
}

/**
 * \brief Write data to audio plugin
 * \param audio_priv private audio data (see audio_open)
 * \param data data to write to audio device
 * \param size number of bytes to write
 * \return number of bytes written or -1 on error
 */
gsize audio_write(gpointer audio_priv, guchar *data, gsize size)
{
	if (!internal_audio) {
		return -1;
	}

	return internal_audio->write(audio_priv, data, size);
}

/**
 * \brief Close current audio device
 * \param audio_priv private audio data (see audio_open)
 * \return TRUE on success, otherwise FALSE
 */
gboolean audio_close(gpointer audio_priv)
{
	if (!internal_audio) {
		return FALSE;
	}

	return internal_audio->close(audio_priv, FALSE);
}

/**
 * \brief Register new audio plugin - set it as internal audio device
 * \param audio audio plugin structure
 */
void routermanager_audio_register(struct audio *audio)
{
	internal_audio = audio;
	internal_audio->init(1, 8000, 16);
}

/**
 * \brief Get default audio device plugin
 * \return default audio device plugin
 */
struct audio *audio_get_default(void)
{
	return internal_audio;
}
