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

/**
 * pdf_draw_cb:
 * @widget: a #GtkDrawingArea
 * @cr: a #cairo_t
 * @user_data: a #PopplerPage
 *
 * Draw pdf on gtk drawing area
 *
 * Returns: %FALSE
 */
static gboolean pdf_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	PopplerPage *page = user_data;
	guint width;
	gdouble popwidth, popheight, scale_factor;

	width = gtk_widget_get_allocated_width(widget);
	poppler_page_get_size(page, &popwidth, &popheight);

	scale_factor = width / popwidth;

	cairo_scale(cr, scale_factor, scale_factor);
	poppler_page_render(page, cr);

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
 * pdf_next_page:
 *
 * Skip to next page of pdf
 */
static void pdf_next_page(void)
{
}

/**
 * app_pdf:
 * @uri: pdf uri
 *
 * Opens pdf viewer for file delivered in @uri.
 *
 * Returns: 0 on success, otherwise error
 */
int app_pdf(char *uri) {
	GtkWidget* win;
	GtkWidget *da;
	GtkWidget *headerbar;
	GtkWidget *sw;
	GtkWidget *grid;
	GError *err = NULL;
	PopplerDocument *doc;
	PopplerPage *page;
	gdouble popwidth, popheight;
	gint pages;

	/* Open document */
	doc = poppler_document_new_from_file(uri, NULL, &err);
	if (!doc) {
			g_warning("%s(): %s", __FUNCTION__, err->message);

			g_object_unref(err);

			return -1;
	}

	/* Get first page */
	page = poppler_document_get_page(doc, 0);
	if(!page) {
			g_warning("%s(): Could not open first page of document", __FUNCTION__);

			g_object_unref(doc);

			return -2;
	}

	/* Get number of pages */
	pages = poppler_document_get_n_pages(doc);
	g_debug("%s(): There are %d pages in this pdf", __FUNCTION__, pages);

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
	da = gtk_drawing_area_new();
	g_signal_connect(G_OBJECT(da), "draw", G_CALLBACK(pdf_draw_cb), page);
	gtk_widget_set_hexpand(da, TRUE);
	gtk_widget_set_vexpand(da, TRUE);
	gtk_grid_attach(GTK_GRID(grid), da, 0, 0, 1, 1);

	/* If we have more than one page, add navigation buttons */
	if (pages > 1) {
		GtkWidget *button;
		GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

		/* Set linked/raised style */
		gtk_container_add(GTK_CONTAINER(headerbar), box);
		gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_RAISED);
		gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_LINKED);

		/* Add prev button */
		button = gtk_button_new_from_icon_name("go-previous-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pdf_next_page), page);
		gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

		/* Add next button */
		button = gtk_button_new_from_icon_name("go-next-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pdf_next_page), page);
		gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	}

	/* Get page size and set default window size */
	poppler_page_get_size(page, &popwidth, &popheight);
	gtk_window_set_default_size(GTK_WINDOW(win), popwidth, popheight);

	g_signal_connect(G_OBJECT(win), "configure-event", G_CALLBACK(pdf_configure_event_cb), da);

	/* Show window */
	gtk_widget_show_all(win);

	return 0;
}

