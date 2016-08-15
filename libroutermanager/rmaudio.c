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
 * - Move audio plugin information into audio private data, in order to switch device while playing
 */

#include <string.h>

#include <glib.h>

#include <libroutermanager/rmprofile.h>
#include <libroutermanager/rmaudio.h>

/** global pointer to current used audio plugin */
static struct audio *rm_internal_audio = NULL;

static GSList *rm_audio_list = NULL;

/**
 * rm_audio_open:
 *
 * Open current audio plugin.
 *
 * Returns: private audio data pointer or %NULL% on error
 */
gpointer rm_audio_open(void)
{
	if (!rm_internal_audio) {
		return NULL;
	}

	return rm_internal_audio->open();
}

/**
 * rm_audio_read:
 * @audio_priv: private audio data (see #rm_audio_open)
 * @data: data pointer
 * @size: number of bytes to read
 *
 * Read of audio plugin.
 *
 * Returns: number of bytes read or -1 on error
 */
gsize rm_audio_read(gpointer audio_priv, guchar *data, gsize size)
{
	if (!rm_internal_audio) {
		return -1;
	}

	return rm_internal_audio->read(audio_priv, data, size);
}

/**
 * rm_audio_write:
 * @audio_priv: private audio data (see #audio_open)
 * @data: data to write to audio device
 * @size: number of bytes to write
 *
 * Write data to audio plugin.
 *
 * Returns: number of bytes written or -1 on error
 */
gsize rm_audio_write(gpointer audio_priv, guchar *data, gsize size)
{
	if (!rm_internal_audio) {
		return -1;
	}

	return rm_internal_audio->write(audio_priv, data, size);
}

/**
 * rm_audio_close:
 * @audio_priv: private audio data (see audio_open)
 *
 * Close current audio device
 *
 * Returns: %TRUE% on success, otherwise %FALSE%
 */
gboolean rm_audio_close(gpointer audio_priv)
{
	if (!rm_internal_audio) {
		return FALSE;
	}

	return rm_internal_audio->close(audio_priv);
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

	rm_audio_list = g_slist_prepend(rm_audio_list, audio);
}

/**
 * rm_audio_get_default:
 *
 * Get default audio device plugin.
 *
 * Returns: default audio device plugin
 */
RmAudio *rm_audio_get_default(void)
{
	return rm_internal_audio;
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
	return rm_audio_list;
}

/**
 * rm_audio_set_default:
 * @name: audio plugin name
 *
 * Select default audio plugin.
 */
void rm_audio_set_default(gchar *name)
{
	GSList *list;

	for (list = rm_audio_list; list != NULL; list = list->next) {
		struct audio *audio = list->data;

		if (!strcmp(audio->name, name)) {
			rm_internal_audio = audio;
		}
	}
}

/**
 * rm_audio_init:
 * @profile: a #RmProfile
 *
 * Initialize audio subsystem.
 */
void rm_audio_init(RmProfile *profile)
{
	gchar *name = g_settings_get_string(profile->settings, "audio-plugin");

	rm_audio_set_default(name);

	/* In case no internal audio is set yet, set it to the first one */
	if (!rm_internal_audio && rm_audio_list) {
		rm_internal_audio = rm_audio_list->data;
	}
}
