/**
 * The libroutermanager project
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#ifndef LIBROUTERMANAGER_RMAUDIO_H
#define LIBROUTERMANAGER_RMAUDIO_H

#include <libroutermanager/rmprofile.h>

G_BEGIN_DECLS

#define RM_AUDIO_OUTPUT 0
#define RM_AUDIO_INPUT  1

/** Audio device structure */
typedef struct audio {
	/* Name of plugin */
	const gchar *name;
	/* Initialize function */
	gboolean (*init)(guchar channels, gushort rate, guchar bits);
	/* Open device for playback */
	gpointer (*open)(void);
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

typedef struct audio_device {
	gchar *name;
	gchar *internal_name;
	gchar type;
} RmAudioDevice;

void rm_audio_register(RmAudio *audio);
RmAudio *rm_audio_get_default(void);
gpointer rm_audio_open(void);
gsize rm_audio_read(gpointer audio_priv, guchar *data, gsize size);
gsize rm_audio_write(gpointer audio_priv, guchar *data, gsize size);
gboolean rm_audio_close(gpointer audio_priv);
GSList *rm_audio_get_plugins(void);
void rm_audio_set_default(gchar *name);
void rm_audio_init(RmProfile *profile);

G_END_DECLS

#endif
