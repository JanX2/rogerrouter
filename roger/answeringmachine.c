/*
 * Roger Router
 * Copyright (c) 2012-2017 Jan-Michael Brummer
 *
 * This file is part of Roger Router.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * TODO:
 *  - Check possibility to remove update timer
 */

#include <config.h>

#include <gtk/gtk.h>

#include <rm/rm.h>

#include <roger/journal.h>
#include <roger/main.h>

struct _VoxPlaybackData {
	gpointer vox_data;
	gint fraction;
	gint update_id;
	GMutex seek_lock;
	gboolean playing;
	GtkWidget *media_button;
	GtkWidget *time;
	GtkWidget *scale;
	gint scale_id;
};

typedef struct _VoxPlaybackData VoxPlaybackData;

static gboolean vox_update_ui(gpointer data);

/**
 * vox_start_playback:
 * @playback: a #VoxPlaybackData
 *
 * Initialize internal data and starts voice playback
 */
static void vox_start_playback(VoxPlaybackData *playback)
{
	/* Reset scale range */
	gtk_range_set_value(GTK_RANGE(playback->scale), 0.0f);

	/* Start playback */
	playback->fraction = 0;
	rm_vox_play(playback->vox_data);
	playback->playing = TRUE;

	/* Change button image */
	GtkWidget *media_image = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(playback->media_button), media_image);
	gtk_widget_set_sensitive(playback->scale, TRUE);

	/* Timer which will update the ui every 250ms */
	playback->update_id = g_timeout_add(250, vox_update_ui, playback);
}

/**
 * vox_stop_playback:
 * @playback: a #VoxPlaybackData
 *
 * Stop voice playback and cleanup data
 */
static void vox_stop_playback(VoxPlaybackData *playback)
{
	GtkWidget *media_image = gtk_image_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(playback->media_button), media_image);
	gtk_widget_set_sensitive(playback->scale, FALSE);
	playback->playing = FALSE;

	if (playback->update_id) {
		g_source_remove(playback->update_id);
		playback->update_id = 0;
	}
}

/**
 * vox_update_ui:
 * @data: a #VoxPlayback
 *
 * Update UI timer: Updates time label/slider and button state
 */
static gboolean vox_update_ui(gpointer data)
{
	VoxPlaybackData *vox_playback = data;
	gint fraction;

	/* Get current fraction (position) */
	fraction = rm_vox_get_fraction(vox_playback->vox_data);

	if (vox_playback->fraction != fraction) {
		gchar *tmp;
		gfloat seconds = rm_vox_get_seconds(vox_playback->vox_data);

		vox_playback->fraction = fraction;

		g_signal_handler_block(G_OBJECT(vox_playback->scale), vox_playback->scale_id);
		gtk_range_set_value(GTK_RANGE(vox_playback->scale), (float)vox_playback->fraction / (float)100);
		g_signal_handler_unblock(G_OBJECT(vox_playback->scale), vox_playback->scale_id);
		tmp = g_strdup_printf(_("Time: %2.2d:%2.2d:%2.2d"), (gint)seconds / 3600, (gint)seconds / 60, (gint)seconds % 60);
		gtk_label_set_text(GTK_LABEL(vox_playback->time), tmp);
		g_free(tmp);

		if (vox_playback->fraction == 100) {
			vox_stop_playback(vox_playback);
		}
	}

	return TRUE;
}

/**
 * vox_media_button_clicked_cb:
 * @button: a #GtkWidget
 * @user_data: a pointer to internal voice playback data
 *
 * Pauses/Plays voice data depending on its state and update UI
 */
static void vox_media_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	VoxPlaybackData *vox_playback = user_data;

	/* We are still playing */
	if (vox_playback->fraction != 100) {
		GtkWidget *media_image;

		/* Toggle play/pause and update button symbol */
		rm_vox_set_pause(vox_playback->vox_data, vox_playback->playing);
		vox_playback->playing = !vox_playback->playing;

		if (vox_playback->playing) {
			media_image = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
		} else {
			media_image = gtk_image_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
		}

		gtk_button_set_image(GTK_BUTTON(vox_playback->media_button), media_image);
	} else {
		/* Playback already stopped. Reset UI and start again */
		vox_start_playback(vox_playback);
	}
}

/**
 * vox_scale_change_value_cb:
 * @range: a #GtkRange
 * @scroll: a #GtkScrollType
 * @value: current sclae value
 * @user_data: a pointer to internal vox playback data
 *
 * Called by the widgets results in seeking within voice data.
 *
 * Returns: %FALSE
 */
static gboolean vox_scale_change_value_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	VoxPlaybackData *vox_playback = user_data;

	rm_vox_seek(vox_playback->vox_data, gtk_range_get_value(range));

	return FALSE;
}

/**
 * app_answeringmachine:
 * @name: file name to play
 *
 * Shows answering machine window for playback
 */
void app_answeringmachine(const gchar *name)
{
	GtkWidget *window;
	GtkBuilder *builder;
	GError *error = NULL;
	gsize len = 0;
	gchar *data;
	RmVoxPlayback *vox;
	VoxPlaybackData *vox_playback;

	/* Load voice data */
	data = rm_router_load_voice(rm_profile_get_active(), name, &len);
	if (!data || !len) {
		g_debug("%s(): could not load file '%s'!", __FUNCTION__, name);

		g_free(data);
		return;
	}

	/* Initialize voice playback system */
	vox = rm_vox_init(data, len, &error);
	if (!vox) {
		g_debug("%s(): Could not init rm vox!", __FUNCTION__);

		g_free(data);
		return;
	}

	/* Load ui */
	builder = gtk_builder_new_from_resource("/org/tabos/roger/answeringmachine.glade");
	if (!builder) {
		g_warning("%s(): Could not load answeringmachine ui", __FUNCTION__);

		rm_vox_shutdown(vox);
		g_free(data);
		return;
	}

	/* Create internal playback structure */
	vox_playback = g_slice_new0(VoxPlaybackData);
	vox_playback->vox_data = vox;

	g_mutex_init(&vox_playback->seek_lock);

	/* Connect to builder objects */
	window = GTK_WIDGET(gtk_builder_get_object(builder, "voicebox"));
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(journal_get_window()));

	/* Play/Pause button */
	vox_playback->media_button = GTK_WIDGET(gtk_builder_get_object(builder, "play_button"));
	g_signal_connect(vox_playback->media_button, "clicked", G_CALLBACK(vox_media_button_clicked_cb), vox_playback);

	/* Playback time display */
	vox_playback->time = GTK_WIDGET(gtk_builder_get_object(builder, "time_label"));

	/* Progress bar. Connect signals for user interaction (sliding/pressing) */
	vox_playback->scale = GTK_WIDGET(gtk_builder_get_object(builder, "progress"));
	vox_playback->scale_id = g_signal_connect(vox_playback->scale, "change-value", G_CALLBACK(vox_scale_change_value_cb), vox_playback);

	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	gtk_widget_show_all(window);

	/* Start playback */
	vox_start_playback(vox_playback);

	/* Show dialog */
	gtk_dialog_run(GTK_DIALOG(window));

	/* Cleanup */
	vox_stop_playback(vox_playback);

	/* Shutdown vox system */
	rm_vox_shutdown(vox_playback->vox_data);

	gtk_widget_destroy(window);

	g_free(data);

	g_mutex_clear(&vox_playback->seek_lock);
	g_slice_free(VoxPlaybackData, vox_playback);
}

