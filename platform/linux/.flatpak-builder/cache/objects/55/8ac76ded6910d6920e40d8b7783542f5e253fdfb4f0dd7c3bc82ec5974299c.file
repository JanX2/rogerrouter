/**
 * Roger Router
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#include <libroutermanager/voxplay.h>
#include <libroutermanager/router.h>

#include <roger/journal.h>
#include <roger/main.h>

struct journal_playback {
	gchar *data;
	gsize len;
	gpointer vox_data;
	gint fraction;

	GtkWidget *media_button;
	GtkWidget *time;
	GtkWidget *scale;
};

static struct journal_playback *playback_data = NULL;

gboolean vox_scale_change_value_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data);

static gboolean vox_update_ui(gpointer data)
{
	gint fraction;

	fraction = vox_get_fraction(playback_data->vox_data);

	if (playback_data->fraction != fraction) {
		gchar *tmp;
		gfloat seconds = vox_get_seconds(playback_data->vox_data);

		playback_data->fraction = fraction;

		gtk_range_set_value(GTK_RANGE(playback_data->scale), (float)playback_data->fraction / (float)100);
		tmp = g_strdup_printf(_("Time: %2.2d:%2.2d:%2.2d"), (gint)seconds / 3600, (gint)seconds / 60, (gint)seconds % 60);
		gtk_label_set_text(GTK_LABEL(playback_data->time), tmp);
		g_free(tmp);

		if (playback_data->fraction == 100) {
			GtkWidget *media_image = gtk_image_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image(GTK_BUTTON(playback_data->media_button), media_image);
		}
	}

	return TRUE;
}

void vox_media_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	if (playback_data->vox_data && playback_data->fraction != 100) {
		GtkWidget *media_image;

		if (vox_playpause(playback_data->vox_data)) {
			media_image = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
		} else {
			media_image = gtk_image_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
		}

		gtk_button_set_image(GTK_BUTTON(playback_data->media_button), media_image);
	} else {
		gtk_range_set_value(GTK_RANGE(playback_data->scale), 0.0f);
		vox_play(playback_data->vox_data);

		GtkWidget *media_image = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON(playback_data->media_button), media_image);
	}
}

gboolean vox_scale_change_value_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	value = ABS(value);

	if (playback_data->vox_data && playback_data->fraction != 100) {
		vox_seek(playback_data->vox_data, value);
	}

	return FALSE;
}

void answeringmachine_play(const gchar *name)
{
	GtkWidget *window;
	gsize len = 0;
	gchar *data = router_load_voice(profile_get_active(), name, &len);
	guint update_id;
	GError *error = NULL;
	gpointer vox;
	GtkBuilder *builder;

	if (!data || !len) {
		g_debug("could not load file!");
		g_free(data);
		return;
	}

	vox = vox_init(data, len, &error);
	if (!vox) {
		g_debug("Could not init vox!");
		g_free(data);
		return;
	}

	/* Only open once at a time */
	if (playback_data) {
		return;
	}

	builder = gtk_builder_new_from_resource("/org/tabos/roger/answeringmachine.glade");
	if (!builder) {
		g_warning("Could not load answeringmachine ui");
		return;
	}

	playback_data = g_slice_new(struct journal_playback);
	playback_data->data = data;
	playback_data->len = len;
	playback_data->vox_data = vox;

	/* Connect to builder objects */
	window = GTK_WIDGET(gtk_builder_get_object(builder, "voicebox"));
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(journal_get_window()));
	playback_data->media_button = GTK_WIDGET(gtk_builder_get_object(builder, "play_button"));

#if GTK_CHECK_VERSION(3,20,0)
	gchar *css_data = g_strdup_printf(".circular-button { border-radius: 20px; -gtk-outline-radius: 20px;}");
#else
	gchar *css_data = g_strdup_printf(".circular-button { border-radius: 20px; outline-radius: 20px; }");
#endif
	GtkCssProvider *css_provider = gtk_css_provider_get_default();
	gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
	g_free(css_data);

	GtkStyleContext *style_context =  gtk_widget_get_style_context(playback_data->media_button);
	gtk_style_context_add_provider(style_context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_class(style_context, "circular-button");

	playback_data->time = GTK_WIDGET(gtk_builder_get_object(builder, "time_label"));

	playback_data->scale = GTK_WIDGET(gtk_builder_get_object(builder, "progress"));
	update_id = g_timeout_add(250, vox_update_ui, playback_data);

	vox_play(playback_data->vox_data);

	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	gtk_widget_show_all(window);
	gtk_dialog_run(GTK_DIALOG(window));

	g_source_remove(update_id);

	if (playback_data->vox_data) {
		vox_stop(playback_data->vox_data);
	}

	gtk_widget_destroy(window);
	playback_data->scale = NULL;

	g_free(data);
	g_slice_free(struct journal_playback, playback_data);

	playback_data = NULL;
}
