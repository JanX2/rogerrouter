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

#include <string.h>

#include <glib.h>

#include <rm/rmaudio.h>
#include <rm/rmprofile.h>

/**
 * SECTION:rmaudio
 * @title: RmAudio
 * @short_description: Audio handling functions
 *
 * Audio contains handling of plugins and common audio functions.
 */

static GSList *rm_audio_plugins = NULL;

/**
 * rm_audio_get:
 * @name: name of audio to lookup
 *
 * Find audio as requested by name.
 *
 * Returns: a #RmAudio, or %NULL on error
 */
RmAudio *rm_audio_get(gchar *name)
{
	GSList *list;
	RmAudio *audio;

	for (list = rm_audio_plugins; list != NULL; list = list->next) {
		audio = list->data;

		if (audio && audio->name && name && !strcmp(audio->name, name)) {
			return audio;
		}
	}

	/* In case no internal audio is set yet, set it to the first one */
	if (rm_audio_plugins) {
		audio = rm_audio_plugins->data;

		g_warning("%s(): Using fallback audio plugin '%s'", __FUNCTION__, audio->name);

		return audio;
	}

	return NULL;
}

/**
 * rm_audio_open:
 * @audio: a #RmAudio
 * @device_name: device name
 *
 * Open current audio plugin.
 *
 * Returns: private audio data pointer or %NULL% on error
 */
gpointer rm_audio_open(RmAudio *audio, gchar *device_name)
{
	RmProfile *profile = rm_profile_get_active();

	if (!audio) {
		return NULL;
	}

	if (!device_name) {
		device_name = g_settings_get_string(profile->settings, "audio-output");
	}

	return audio->open(device_name);
}

/**
 * rm_audio_read:
 * @audio: a #RmAudio
 * @audio_priv: private audio data (see #rm_audio_open)
 * @data: data pointer
 * @size: number of bytes to read
 *
 * Read of audio plugin.
 *
 * Returns: number of bytes read or -1 on error
 */
gsize rm_audio_read(RmAudio *audio, gpointer audio_priv, guchar *data, gsize size)
{
	return audio ? audio->read(audio_priv, data, size) : -1;
}

/**
 * rm_audio_write:
 * @audio: a #RmAudio
 * @audio_priv: private audio data (see #audio_open)
 * @data: data to write to audio device
 * @size: number of bytes to write
 *
 * Write data to audio plugin.
 *
 * Returns: number of bytes written or -1 on error
 */
gsize rm_audio_write(RmAudio *audio, gpointer audio_priv, guchar *data, gsize size)
{
	return audio ? audio->write(audio_priv, data, size) : -1;
}

/**
 * rm_audio_close:
 * @audio: a #RmAudio
 * @audio_priv: private audio data (see audio_open)
 *
 * Close current audio device
 *
 * Returns: %TRUE% on success, otherwise %FALSE%
 */
gboolean rm_audio_close(RmAudio *audio, gpointer audio_priv)
{
	return audio ? audio->close(audio_priv) : TRUE;
}

/**
 * rm_audio_register:
 * @audio: a #RmAudio
 *
 * Register new audio plugin - set it as internal audio device.
 */
void rm_audio_register(RmAudio *audio)
{
	audio->init(1, 8000, 16);

	rm_audio_plugins = g_slist_prepend(rm_audio_plugins, audio);
}

/**
 * rm_audio_unregister:
 * @audio: a #RmAudio
 *
 * Unregister audio plugin
 */
void rm_audio_unregister(RmAudio *audio)
{
	rm_audio_plugins = g_slist_remove(rm_audio_plugins, audio);
}

/**
 * rm_audio_get_plugins:
 *
 * Get list of audio plugins.
 *
 * Returns: audio plugin list
 */
GSList *rm_audio_get_plugins(void)
{
	return rm_audio_plugins;
}

/**
 * rm_audio_get_name:
 * @audio: a #RmAudio
 *
 * Get audio name of @audio.
 *
 * Returns: audio name
 */
gchar *rm_audio_get_name(RmAudio *audio)
{
	return g_strdup(audio->name);
}
