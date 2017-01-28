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

GtkWidget *ui_label_new(gchar *text);
void gtk_widget_set_margin(GtkWidget *widget, gint x1, gint y1, gint x2, gint y2);

G_END_DECLS

#endif

