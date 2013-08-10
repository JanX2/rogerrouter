/**
 * Roger Router
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

#include <libroutermanager/file.h>
#include <debugwindow.h>

#define _(x) x

static guint debug_window_log_id = 0;
static gboolean debug_window_pause = FALSE;
static GLogLevelFlags debug_window_log_level = 0xFF;
static GtkWidget *textview = NULL;

void debug_window_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	GString *output;
	GtkTextBuffer *buffer;
	GtkTextIter end;

	if (!textview) {
		return;
	}

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

	if (debug_window_pause) {
		return;
	}

	if (!(log_level & debug_window_log_level)) {
		return;
	}

	output = g_string_new("");

	if (log_domain) {
		g_string_append_printf(output, "%s:", log_domain);
	}

	switch (log_level) {
	case G_LOG_LEVEL_ERROR:
			output = g_string_append(output, "Error:");
			break;
	case G_LOG_LEVEL_CRITICAL:
			output = g_string_append(output, "Critical:");
			break;
	case G_LOG_LEVEL_WARNING:
			output = g_string_append(output, "Warning:");
			break;
	case G_LOG_LEVEL_DEBUG:
			output = g_string_append(output, "Debug:");
			break;
	default:
			output = g_string_append(output, "*");
			break;
	}

	g_string_append_printf(output, " %s\n", message);

	gtk_text_buffer_get_end_iter(buffer, &end);
	gtk_text_buffer_insert(buffer, &end, output->str, output->len);

	g_string_free(output, TRUE);
}

static void debug_window_destroy(GtkWidget *widget, gpointer user_data)
{
	if (debug_window_log_id) {
		g_debug("Destroy debug window log handler");
		g_log_remove_handler(NULL, debug_window_log_id);
		debug_window_log_id = 0;
	}
	textview = NULL;
}

void debug_window_save_clicked(GtkWidget *button, gpointer *user_data)
{
	GtkTextView *textview = GTK_TEXT_VIEW(user_data);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Save debug log", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GtkTextIter start;
		GtkTextIter end;
		gchar *filename;
		gchar *text;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);
		text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		file_save(filename, text, -1);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);
}

void debug_window_clear_clicked(GtkWidget *button, gpointer *user_data)
{
	GtkTextView *textview = GTK_TEXT_VIEW(user_data);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
	GtkTextIter start;
	GtkTextIter end;

	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);

	gtk_text_buffer_delete(buffer, &start, &end);
}

void debug_window_pause_clicked(GtkWidget *button, gpointer *user_data)
{
	debug_window_pause = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button));
}

void level_box_changed(GtkComboBox *box, gpointer user_data)
{
	gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(box));

	debug_window_log_level = 0;

	switch (index) {
		case 0:
			/* Debug */
			debug_window_log_level |= G_LOG_LEVEL_DEBUG;
		case 1:
			/* Warning */
			debug_window_log_level |= G_LOG_LEVEL_WARNING;
		case 2:
			/* Error */
			debug_window_log_level |= G_LOG_LEVEL_ERROR;
		default:
			break;
	}
}

void debug_window(void)
{
	GtkWidget *window;
	GtkWidget *grid;
	GtkWidget *toolbar;
	GtkToolItem *save_button;
	GtkToolItem *clear_button;
	GtkToolItem *separator;
	GtkToolItem *pause_button;
	GtkWidget *level_label;
	GtkWidget *level_box;
	GtkWidget *scroll_window;

	if (debug_window_log_id) {
		return;
	}

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "destroy", G_CALLBACK(debug_window_destroy), NULL);

	gtk_window_set_title(GTK_WINDOW(window), _("Debug window"));
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	
	grid = gtk_grid_new();

	/* Toolbar + butttons */
	toolbar = gtk_toolbar_new();
	gtk_widget_set_hexpand(toolbar, FALSE);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), FALSE);
	gtk_grid_attach(GTK_GRID(grid), toolbar, 1, 1, 1, 1);

	save_button = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(save_button), 0);

	clear_button = gtk_tool_button_new_from_stock(GTK_STOCK_CLEAR);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(clear_button), 1);

	separator = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(separator), 2);

	pause_button = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_MEDIA_PAUSE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(pause_button), 1);

	separator = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(separator), 2);
	
	/* Log level */
	level_label = gtk_label_new(_("Level:"));
	gtk_misc_set_alignment(GTK_MISC(level_label), 1.0f, 0.5f);
	gtk_grid_attach(GTK_GRID(grid), level_label, 2, 1, 1, 1);

	level_box = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(level_box), NULL, _("Debug"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(level_box), NULL, _("Warning"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(level_box), NULL, _("Error"));
	g_signal_connect(G_OBJECT(level_box), "changed", G_CALLBACK(level_box_changed), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(level_box), 0);
	gtk_grid_attach(GTK_GRID(grid), level_box, 3, 1, 1, 1);

	/* Scoll window + textview */
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_hexpand(scroll_window, TRUE);
	gtk_widget_set_vexpand(scroll_window, TRUE);

	textview = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
	gtk_container_add(GTK_CONTAINER(scroll_window), textview);

	gtk_grid_attach(GTK_GRID(grid), scroll_window, 1, 2, 3, 1);

	gtk_container_add(GTK_CONTAINER(window), grid);

	g_signal_connect(G_OBJECT(save_button), "clicked", G_CALLBACK(debug_window_save_clicked), textview);
	g_signal_connect(G_OBJECT(clear_button), "clicked", G_CALLBACK(debug_window_clear_clicked), textview);
	g_signal_connect(G_OBJECT(pause_button), "clicked", G_CALLBACK(debug_window_pause_clicked), textview);

	gtk_widget_show_all(window);

	debug_window_log_id = g_log_set_handler(NULL,  G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, debug_window_log_handler, NULL);
}
