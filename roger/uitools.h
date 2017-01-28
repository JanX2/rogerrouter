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

#ifndef UITOOLS_H
#define UITOOLS_H

G_BEGIN_DECLS

#ifndef GTK_DIALOG_USE_HEADER_BAR
#define GTK_DIALOG_USE_HEADER_BAR 0
#endif

#define ui_bold_text(text) g_strdup_printf("<b>%s</b>", text)
#define ui_set_suggested_style(widget) gtk_style_context_add_class(gtk_widget_get_style_context(widget), GTK_STYLE_CLASS_SUGGESTED_ACTION)
#define ui_set_destructive_style(widget) gtk_style_context_add_class(gtk_widget_get_style_context(widget), GTK_STYLE_CLASS_DESTRUCTIVE_ACTION)
#define ui_set_button_style(widget) gtk_style_context_add_class(gtk_widget_get_style_context(widget), GTK_STYLE_CLASS_BUTTON_ACTION)

#define gtk_widget_set_margin(widget, x1, y1, x2, y2) {\
	gtk_widget_set_margin_top(widget, y1);\
	gtk_widget_set_margin_bottom(widget, y2);\
	gtk_widget_set_margin_start(widget, x1);\
	gtk_widget_set_margin_end(widget, x2);\
}

static inline GtkWidget *ui_group_create(GtkWidget *box, gchar *title_str, gboolean hexpand, gboolean vexpand) {
	GtkWidget *group;
	GtkWidget *title;
	gchar *title_markup = ui_bold_text(title_str);

	group = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(group), 5);

	/* Configure plugins label */
	title = gtk_label_new("");
	gtk_widget_set_halign(title, GTK_ALIGN_START);
	gtk_widget_set_margin(title, 10, 5, 10, 5);
	gtk_label_set_markup(GTK_LABEL(title), title_markup);
	gtk_grid_attach(GTK_GRID(group), title, 0, 0, 1, 1);

	gtk_widget_set_margin(box, 20, 0, 20, 10);
	gtk_grid_attach(GTK_GRID(group), box, 0, 1, 1, 1);

	g_free(title_markup);

	return group;
}

/**
 * ui_label_new:
 * @text: new ui text label
 *
 * Creates a new ui description label (not sensitive and align at end)
 *
 * Returns: new ui label
 */
static inline GtkWidget *ui_label_new(gchar *text)
{
	GtkWidget *label;

	label = gtk_label_new(text);
	gtk_widget_set_sensitive(label, FALSE);
	gtk_widget_set_halign(label, GTK_ALIGN_END);

	return label;
}

G_END_DECLS

#endif

