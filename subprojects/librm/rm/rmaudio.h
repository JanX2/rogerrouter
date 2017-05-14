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

#ifndef __RM_AUDIO_H_
#define __RM_AUDIO_H_

//#if !defined (__RM_H_INSIDE__) && !defined(RM_COMPILATION)
//#error "Only <rm/rm.h> can be included directly."
//#endif

G_BEGIN_DECLS

#define RM_AUDIO_OUTPUT 0
#define RM_AUDIO_INPUT  1

/** Audio device structure */
typedef struct {
	/* Name of plugin */
	const gchar *name;
	/* Initialize function */
	gboolean (*init)(guchar channels, gushort rate, guchar bits);
	/* Open device for playback */
	gpointer (*open)(gchar *device_name);
	/* Write data to audio device */
	gsize (*write)(gpointer priv, guchar *buffer, gsize len);
	/* Read data of audio device */
	gsize (*read)(gpointer priv, guchar *buffer, gsize max_len);
	/* Close audio device */
	gboolean (*close)(gpointer priv);
	/* Shutdown audio device */
	gboolean (*shutdown)(void);
	/* Get possible audio input/output devices */
	GSList *(*get_devices)(void);
} RmAudio;

typedef struct {
	gchar *name;
	gchar *internal_name;
	gchar type;
} RmAudioDevice;

void rm_audio_register(RmAudio *audio);
void rm_audio_unregister(RmAudio *audio);
RmAudio *rm_audio_get(gchar *name);
gpointer rm_audio_open(RmAudio *audio, gchar *device_name);
gsize rm_audio_read(RmAudio *audio, gpointer audio_priv, guchar *data, gsize size);
gsize rm_audio_write(RmAudio *audio, gpointer audio_priv, guchar *data, gsize size);
gboolean rm_audio_close(RmAudio *audio, gpointer audio_priv);
GSList *rm_audio_get_plugins(void);
gchar *rm_audio_get_name(RmAudio *audio);

G_END_DECLS

#endif
