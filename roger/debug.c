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

#include <gtk/gtk.h>

#include <rm/rm.h>

#include <roger/application.h>
#include <roger/main.h>

/** Debug window */
static GtkWidget *debug_window = NULL;

typedef struct debug_data {
	GLogLevelFlags flags;
	gchar *message;
} DebugData;

/**
 * debug_log_idle:
 * @user_data: a #DebugData package
 *
 * Updates textview with new log data
 *
 * Returns: %G_SOURCE_REMOVE
 */
static gboolean debug_log_idle(gpointer user_data)
{
	GtkWidget *textview = g_object_get_data(G_OBJECT(debug_window), "textview");
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	DebugData *data = user_data;
	GtkTextIter iter;
	gchar *type;
	gchar *tag;
	GDateTime *datetime;
	GString *output;
	gchar *time;

	output = g_string_new("");
	datetime = g_date_time_new_now_local();
	time = g_date_time_format(datetime, "%H:%M:%S");
	g_string_append_printf(output, "(%s)", time);
	g_free(time);
	g_date_time_unref(datetime);

	switch (data->flags) {
	case G_LOG_LEVEL_CRITICAL:
		type = g_strdup(" CRITICAL: ");
		tag = "red_fg";
		break;
	case G_LOG_LEVEL_ERROR:
		type = g_strdup(" ERROR: ");
		tag = "red_fg";
		break;
	case G_LOG_LEVEL_WARNING:
		type = g_strdup(" WARNING: ");
		tag = "orange_fg";
		break;
	case G_LOG_LEVEL_DEBUG:
		type = g_strdup(" DEBUG: ");
		tag = NULL;
		break;
	case G_LOG_LEVEL_INFO:
		type = g_strdup(" INFO: ");
		tag = "blue_fg";
		break;
	default:
		type = g_strdup(" SYSTEM: ");
		tag = "blue_fg";
		break;
	}

	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(text_buffer), &iter);
	gtk_text_buffer_insert_with_tags_by_name(text_buffer, &iter, output->str, output->len, tag, NULL);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(text_buffer), &iter);
	gtk_text_buffer_insert_with_tags_by_name(text_buffer, &iter, type, -1, "bold", tag, NULL);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(text_buffer), &iter);
	gtk_text_buffer_insert_with_tags_by_name(text_buffer, &iter, data->message, -1, "lmarg", tag, NULL);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(text_buffer), &iter);
	gtk_text_buffer_insert_with_tags_by_name(text_buffer, &iter, "\n", -1, tag, NULL);

	/* Scroll to end */
	GtkTextMark *mark = gtk_text_buffer_get_insert(text_buffer);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview), mark, 0.0, TRUE, 0.5, 1);

	g_free(type);
	g_string_free(output, TRUE);
	g_slice_free(DebugData, data);

	return G_SOURCE_REMOVE;
}

/**
 * debug_log_handler:
 * @flags: #GLogLevelFlags
 * @message: debug message
 *
 * Assembles a debug data package and transfer it to log idle writer
 */
static void debug_log_handler(GLogLevelFlags flags, const gchar *message)
{
	DebugData *data = g_slice_new0(struct debug_data);

	data->flags = flags;
	data->message = g_strdup(message);

	g_idle_add(debug_log_idle, data);
}

/**
 * debug_delete_event_cb:
 * @widget: window widget
 * @event: a #GdkEvent
 * @user_data: unused
 *
 * Delete event of window. Clears debug flag and removes app log handler
 *
 * Returns: %FALSE
 */
static gboolean debug_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	g_settings_set_boolean(app_settings, "debug", FALSE);

	rm_log_set_app_handler(NULL);

	debug_window = NULL;

	return FALSE;
}

/**
 * debug_clear_clicked_cb:
 * @widget: button widget
 * @user_data: a textview widget
 *
 * Clears textview text
 */
static void debug_clear_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	GtkWidget *textview = user_data;
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

	gtk_text_buffer_set_text(text_buffer, "", -1);
}

/**
 * debug_save_clicked_cb:
 * @widget: button widget
 * @user_data: a textview widget
 *
 * Save debug file callback
 */
static void debug_save_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	GtkWidget *textview = user_data;
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	GtkFileChooserNative *native;
	GtkFileChooser *chooser;
	gchar *time;
	gchar *buf;
	GDateTime *datetime = g_date_time_new_now_local();
	gint res;

	time = g_date_time_format(datetime, "%Y-%m-%d-%H-%M-%S");
	buf = g_strdup_printf("roger-log-%s.txt", time);
	g_free(time);
	g_date_time_unref(datetime);

	native = gtk_file_chooser_native_new("Save Log", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, NULL, NULL);
	chooser = GTK_FILE_CHOOSER(native);

	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

	gtk_file_chooser_set_current_name(chooser, buf);

	res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
	if (res == GTK_RESPONSE_ACCEPT) {
		GtkTextIter start, end;
		char *filename;

		filename = gtk_file_chooser_get_filename(chooser);
		gtk_text_buffer_get_start_iter(text_buffer, &start);
		gtk_text_buffer_get_end_iter(text_buffer, &end);
		rm_file_save(filename, gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE), -1);
		g_free(filename);
	}

	g_free(buf);

	g_object_unref(native);
}

/**
 * app_debug_window:
 *
 * Show debug window
 */
void app_debug_window(void)
{
	GtkWidget *header_bar;
	GtkWidget *button;
	GtkWidget *scroll_win;
	GtkWidget *textview = gtk_text_view_new();
	GtkTextBuffer *text_buffer;

	if (debug_window) {
		gtk_window_present(GTK_WINDOW(debug_window));

		return;
	}

	/* Create debug window */
	debug_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_object_set_data(G_OBJECT(debug_window), "textview", textview);
	gtk_window_set_default_size(GTK_WINDOW(debug_window), 600, 300);
	g_signal_connect(debug_window, "delete-event", G_CALLBACK(debug_delete_event_cb), NULL);

	/* Add header bar */
	header_bar = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), _("Debug"));
	gtk_window_set_titlebar(GTK_WINDOW(debug_window), header_bar);

	/* Add save button */
	button = gtk_button_new_from_icon_name("document-save-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	g_signal_connect(button, "clicked", G_CALLBACK(debug_save_clicked_cb), textview);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), button);

	/* Add clear button */
	button = gtk_button_new_from_icon_name("edit-clear-all-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	g_signal_connect(button, "clicked", G_CALLBACK(debug_clear_clicked_cb), textview);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), button);

	/* Add scrolled window */
	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_hexpand(scroll_win, TRUE);
	gtk_widget_set_vexpand(scroll_win, TRUE);
	gtk_container_add(GTK_CONTAINER(debug_window), scroll_win);

	/* Add textview and set text buffer tags */
	gtk_container_add(GTK_CONTAINER(scroll_win), textview);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_CHAR);

	text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_create_tag(text_buffer, "blue_fg", "foreground", "blue", NULL);
	gtk_text_buffer_create_tag(text_buffer, "red_fg", "foreground", "red", NULL);
	gtk_text_buffer_create_tag(text_buffer, "orange_fg", "foreground", "orange", NULL);
	gtk_text_buffer_create_tag(text_buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag(text_buffer, "lmarg", "left_margin", 12, NULL);

	/* Show window */
	gtk_widget_show_all(debug_window);

	/* Persist debug state and set app handler for logging */
	g_settings_set_boolean(app_settings, "debug", TRUE);
	rm_log_set_app_handler(debug_log_handler);
}

