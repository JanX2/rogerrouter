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

#include <config.h>

#include <gtk/gtk.h>

#include <rm/rmvox.h>
#include <rm/rmrouter.h>

#include <roger/journal.h>
#include <roger/main.h>

struct _VoxPlayback {
	gpointer vox_data;
	gint fraction;
	gboolean seek_lock;

	GtkWidget *media_button;
	GtkWidget *time;
	GtkWidget *scale;
};

typedef struct _VoxPlayback VoxPlayback;

/**
 * vox_update_ui:
 * @data: a pointer to internal vox playback data
 */
static gboolean vox_update_ui(gpointer data)
{
	VoxPlayback *vox_playback = data;
	gint fraction;

	if (vox_playback->seek_lock) {
		return TRUE;
	}

	fraction = rm_vox_get_fraction(vox_playback->vox_data);

	if (vox_playback->fraction != fraction) {
		gchar *tmp;
		gfloat seconds = rm_vox_get_seconds(vox_playback->vox_data);

		vox_playback->fraction = fraction;

		gtk_range_set_value(GTK_RANGE(vox_playback->scale), (float)vox_playback->fraction / (float)100);
		tmp = g_strdup_printf(_("Time: %2.2d:%2.2d:%2.2d"), (gint)seconds / 3600, (gint)seconds / 60, (gint)seconds % 60);
		gtk_label_set_text(GTK_LABEL(vox_playback->time), tmp);
		g_free(tmp);

		if (vox_playback->fraction == 100) {
			GtkWidget *media_image = gtk_image_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image(GTK_BUTTON(vox_playback->media_button), media_image);
			gtk_widget_set_sensitive(vox_playback->scale, FALSE);
		}
	}

	return TRUE;
}

/**
 * vox_media_button_clicked_cb:
 * @button: a #GtkWidget
 * @user_data: a pointer to internal voice playback data
 */
static void vox_media_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	VoxPlayback *vox_playback = user_data;

	if (vox_playback->fraction != 100) {
		GtkWidget *media_image;

		if (rm_vox_playpause(vox_playback->vox_data)) {
			media_image = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
		} else {
			media_image = gtk_image_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
		}

		gtk_button_set_image(GTK_BUTTON(vox_playback->media_button), media_image);
	} else {
		/* Reset scale range */
		gtk_range_set_value(GTK_RANGE(vox_playback->scale), 0.0f);

		/* Start playback */
		vox_playback->fraction = 0;
		rm_vox_play(vox_playback->vox_data);

		/* Change button image */
		GtkWidget *media_image = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON(vox_playback->media_button), media_image);
		gtk_widget_set_sensitive(vox_playback->scale, TRUE);
	}
}

/**
 * vox_scale_change_value_cb:
 * @range: a #GtkRange
 * @scroll: a #GtkScrollType
 * @value: current sclae value
 * @user_data: a pointer to internal vox playback data
 *
 * Returns: %FALSE
 */
static gboolean vox_scale_change_value_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	VoxPlayback *vox_playback = user_data;

	if (!vox_playback->seek_lock) {
		return FALSE;
	}

	if (vox_playback->fraction != 100) {
		rm_vox_seek(vox_playback->vox_data, gtk_range_get_value(range));
	}

	return FALSE;
}

static gboolean vox_scale_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	VoxPlayback *vox_playback = user_data;

	vox_playback->seek_lock = TRUE;
	rm_vox_playpause(vox_playback->vox_data);

	return FALSE;
}

static gboolean vox_scale_button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	VoxPlayback *vox_playback = user_data;

	vox_playback->seek_lock = FALSE;
	rm_vox_playpause(vox_playback->vox_data);

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
	guint update_id;
	gpointer vox;
	VoxPlayback *vox_playback;

	/* Load voice data */
	data = rm_router_load_voice(rm_profile_get_active(), name, &len);
	if (!data || !len) {
		g_debug("%s(): could not load file!", __FUNCTION__);
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
	vox_playback = g_slice_new0(VoxPlayback);
	vox_playback->vox_data = vox;

	/* Connect to builder objects */
	window = GTK_WIDGET(gtk_builder_get_object(builder, "voicebox"));
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(journal_get_window()));
	vox_playback->media_button = GTK_WIDGET(gtk_builder_get_object(builder, "play_button"));
	g_signal_connect(vox_playback->media_button, "clicked", G_CALLBACK(vox_media_button_clicked_cb), vox_playback);

#if GTK_CHECK_VERSION(3,20,0)
	gchar *css_data = g_strdup_printf(".circular-button { border-radius: 20px; -gtk-outline-radius: 20px;}");
#else
	gchar *css_data = g_strdup_printf(".circular-button { border-radius: 20px; outline-radius: 20px; }");
#endif
	GtkCssProvider *css_provider = gtk_css_provider_get_default();
	gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
	g_free(css_data);

	GtkStyleContext *style_context =  gtk_widget_get_style_context(vox_playback->media_button);
	gtk_style_context_add_provider(style_context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_class(style_context, "circular-button");

	vox_playback->time = GTK_WIDGET(gtk_builder_get_object(builder, "time_label"));

	vox_playback->scale = GTK_WIDGET(gtk_builder_get_object(builder, "progress"));
	g_signal_connect(vox_playback->scale, "change-value", G_CALLBACK(vox_scale_change_value_cb), vox_playback);
	g_signal_connect(vox_playback->scale, "button-press-event", G_CALLBACK(vox_scale_button_press_event_cb), vox_playback);
	g_signal_connect(vox_playback->scale, "button-release-event", G_CALLBACK(vox_scale_button_release_event_cb), vox_playback);
	update_id = g_timeout_add(250, vox_update_ui, vox_playback);

	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	gtk_widget_show_all(window);

	/* Start playback */
	rm_vox_play(vox_playback->vox_data);

	/* Show dialog */
	gtk_dialog_run(GTK_DIALOG(window));

	/* Cleanup */
	g_source_remove(update_id);

	/* Shutdown vox system */
	rm_vox_shutdown(vox_playback->vox_data);

	gtk_widget_destroy(window);

	g_free(data);
	g_slice_free(VoxPlayback, vox_playback);
}
