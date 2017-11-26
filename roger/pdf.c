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

#include <glib/gi18n.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <poppler.h>

#include <rm/rm.h>

typedef struct pdf_viewer {
	PopplerDocument *doc;
	PopplerPage *page;
	GtkWidget *da;
	GtkWidget *prev;
	GtkWidget *next;
	gchar *data;
	gint length;
	gint num_pages;
	gint current_page;
} PdfViewer;

/**
 * pdf_draw_cb:
 * @widget: a #GtkDrawingArea
 * @cr: a #cairo_t
 * @user_data: a #PdfViewer
 *
 * Draw pdf on gtk drawing area
 *
 * Returns: %FALSE
 */
static gboolean pdf_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	PdfViewer *pdf_viewer = user_data;
	guint width;
	gdouble popwidth, popheight, scale_factor;

	width = gtk_widget_get_allocated_width(widget);
	poppler_page_get_size(pdf_viewer->page, &popwidth, &popheight);

	scale_factor = width / popwidth;

	cairo_scale(cr, scale_factor, scale_factor);
	poppler_page_render(pdf_viewer->page, cr);

	gtk_widget_set_size_request(widget, (int)popwidth * scale_factor, (int)popheight * scale_factor);

	return FALSE;
}

/**
 * pdf_configure_event_cb:
 * @widget: a window widget
 * @event: a #GdkEvent
 * @user_data: a #GtkDrawingArea
 *
 * Callback for configure-event - ensure that drawing area has correct size
 *
 * Returns: %FALSE
 */
static gboolean pdf_configure_event_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	GtkWidget *da = user_data;
	gint width;
	gint height;

	gtk_window_get_size(GTK_WINDOW(widget), &width, &height);
	gtk_widget_set_size_request(da, width, height);

	return FALSE;
}

/**
 * pdf_prev_page:
 *
 * Skip to prev page of pdf
 */
static void pdf_prev_page(GtkWidget *widget, gpointer user_data)
{
	PdfViewer *pdf_viewer = user_data;

	if (pdf_viewer->current_page - 1 >= 0) {
		pdf_viewer->current_page--;

		pdf_viewer->page = poppler_document_get_page(pdf_viewer->doc, pdf_viewer->current_page);
		gtk_widget_queue_draw(pdf_viewer->da);

		if (pdf_viewer->current_page == 0) {
			gtk_widget_set_sensitive(pdf_viewer->prev, FALSE);
		}
		gtk_widget_set_sensitive(pdf_viewer->next, TRUE);
	}
}

/**
 * pdf_next_page:
 *
 * Skip to next page of pdf
 */
static void pdf_next_page(GtkWidget *widget, gpointer user_data)
{
	PdfViewer *pdf_viewer = user_data;

	if (pdf_viewer->current_page + 1 < pdf_viewer->num_pages) {
		pdf_viewer->current_page++;

		pdf_viewer->page = poppler_document_get_page(pdf_viewer->doc, pdf_viewer->current_page);
		gtk_widget_queue_draw(pdf_viewer->da);

		if (pdf_viewer->current_page == pdf_viewer->num_pages - 1) {
			gtk_widget_set_sensitive(pdf_viewer->next, FALSE);
		}
		gtk_widget_set_sensitive(pdf_viewer->prev, TRUE);
	}
}

static void pdf_save(GtkWidget *widget, gpointer user_data)
{
	PdfViewer *pdf_viewer = user_data;
	GtkWidget *filechooser;
	gint result;
	gchar *file = NULL;

	filechooser = gtk_file_chooser_dialog_new(_("Save Fax"), NULL, GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"),
                                      GTK_RESPONSE_CANCEL,
                                      _("_Save"),
                                      GTK_RESPONSE_ACCEPT,
                                      NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(filechooser), "fax.pdf");
	result = gtk_dialog_run(GTK_DIALOG(filechooser));

	if (result == GTK_RESPONSE_ACCEPT) {
		file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filechooser));
		if (file) {
			rm_file_save(file, pdf_viewer->data, pdf_viewer->length);
		}
	}
	gtk_widget_destroy(filechooser);
}

/**
 * app_pdf:
 * @data: data in memory
 * @length: length of data
 * @uri: pdf uri
 *
 * Opens pdf viewer for in memory (data/length) or file delivered in @uri.
 *
 * Returns: 0 on success, otherwise error
 */
int app_pdf(char *data, int length, gchar *uri) {
	GtkWidget* win;
	GtkWidget *headerbar;
	GtkWidget *sw;
	GtkWidget *grid;
	GError *err = NULL;
	PopplerDocument *doc;
	gdouble popwidth, popheight;
	PdfViewer *pdf_viewer;

	/* Open document */
	if (data && length) {
		doc = poppler_document_new_from_data(data, length, "", &err);
	} else {
		doc = poppler_document_new_from_file(uri, "", &err);
	}

	if (!doc) {
			g_warning("%s(): %s", __FUNCTION__, err->message);

			g_error_free(err);

			return -1;
	}

	pdf_viewer = g_slice_alloc0(sizeof(PdfViewer));
	pdf_viewer->doc = doc;
	pdf_viewer->length = length;
	pdf_viewer->data = data;

	/* Get first page */
	pdf_viewer->page = poppler_document_get_page(doc, 0);
	if(!pdf_viewer->page) {
			g_warning("%s(): Could not open first page of document", __FUNCTION__);

			g_object_unref(pdf_viewer->doc);

			return -2;
	}

	/* Get number of pages */
	pdf_viewer->num_pages = poppler_document_get_n_pages(doc);
	g_debug("%s(): There are %d pages in this pdf", __FUNCTION__, pdf_viewer->num_pages);

	/* Create window */
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	/* Create headerbar and add it to the window */
	headerbar = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(headerbar), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(headerbar), _("Fax Viewer"));
	gtk_window_set_titlebar(GTK_WINDOW(win), headerbar);

	/* Create and add scrolled window */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(win), sw);

	/* Create grid */
	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(sw), grid);

	/* Create drawing area */
	pdf_viewer->da = gtk_drawing_area_new();
	g_signal_connect(G_OBJECT(pdf_viewer->da), "draw", G_CALLBACK(pdf_draw_cb), pdf_viewer);
	gtk_widget_set_hexpand(pdf_viewer->da, TRUE);
	gtk_widget_set_vexpand(pdf_viewer->da, TRUE);
	gtk_grid_attach(GTK_GRID(grid), pdf_viewer->da, 0, 0, 1, 1);

		GtkWidget *button;
		GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

		/* Set linked/raised style */
		gtk_header_bar_pack_start (GTK_HEADER_BAR(headerbar), box);
		gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_RAISED);
		gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_LINKED);

		/* Add prev button */
		button = gtk_button_new_from_icon_name("go-previous-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pdf_prev_page), pdf_viewer);
		gtk_widget_set_sensitive(button, FALSE);
		gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
		pdf_viewer->prev = button;

		/* Add next button */
		button = gtk_button_new_from_icon_name("go-next-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_widget_set_sensitive(button, FALSE);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pdf_next_page), pdf_viewer);
		gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
		pdf_viewer->next = button;

		button = gtk_button_new_from_icon_name("document-save-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pdf_save), pdf_viewer);
		gtk_header_bar_pack_end(GTK_HEADER_BAR(headerbar), button);

	if (pdf_viewer->num_pages > 1) {
		gtk_widget_set_sensitive(pdf_viewer->next, TRUE);
	}

	/* Get page size and set default window size */
	poppler_page_get_size(pdf_viewer->page, &popwidth, &popheight);
	gtk_window_set_default_size(GTK_WINDOW(win), popwidth, popheight);

	g_signal_connect(G_OBJECT(win), "configure-event", G_CALLBACK(pdf_configure_event_cb), pdf_viewer->da);

	/* Show window */
	gtk_widget_show_all(win);

	return 0;
}

