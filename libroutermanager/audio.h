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

#ifndef LIBROUTERMANAGER_AUDIO_H
#define LIBROUTERMANAGER_AUDIO_H

G_BEGIN_DECLS

#define AUDIO_OUTPUT 0
#define AUDIO_INPUT  1

/** Audio device structure */
struct audio {
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
	gboolean (*close)(gpointer priv, gboolean force);
	/* Shutdown audio device */
	gboolean (*deinit)(void);
	/* Get possible audio input/output devices */
	GSList *(*get_devices)(void);
};

struct audio_device {
	gchar *name;
	gchar *internal_name;
	gchar type;
};

void routermanager_audio_register(struct audio *audio);
struct audio *audio_get_default(void);
gpointer audio_open(void);
gsize audio_read(gpointer audio_priv, guchar *data, gsize size);
gsize audio_write(gpointer audio_priv, guchar *data, gsize size);
gboolean audio_close(gpointer audio_priv);

G_END_DECLS

#endif
