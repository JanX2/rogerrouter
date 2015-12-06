/**
 * Roger Router
 * Copyright (c) 2012-2014 Jan-Michael Brummer
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

#include <roger/uitools.h>

GtkWidget *ui_label_new(gchar *text)
{
	GtkWidget *label;

	label = gtk_label_new(text);
	gtk_widget_set_sensitive(label, FALSE);
	gtk_widget_set_halign(label, GTK_ALIGN_END);

	return label;
}

gchar *ui_bold_text(gchar *text)
{
	return g_strdup_printf("<b>%s</b>", text);
}

void gtk_widget_set_margin(GtkWidget *widget, gint x1, gint y1, gint x2, gint y2)
{
	gtk_widget_set_margin_top(widget, y1);
	gtk_widget_set_margin_bottom(widget, y2);

#if GTK_CHECK_VERSION(3,12,0)
	gtk_widget_set_margin_start(widget, x1);
	gtk_widget_set_margin_end(widget, x2);
#else
	gtk_widget_set_margin_left(widget, x1);
	gtk_widget_set_margin_right(widget, x2);
#endif
}

void ui_set_suggested_style(GtkWidget *widget)
{
#ifdef GTK_STYLE_CLASS_SUGGESTED_ACTION
	gtk_style_context_add_class(gtk_widget_get_style_context(widget), GTK_STYLE_CLASS_SUGGESTED_ACTION);
#endif
}

void ui_set_destructive_style(GtkWidget *widget)
{
#ifdef GTK_STYLE_CLASS_DESTRUCTIVE_ACTION
	gtk_style_context_add_class(gtk_widget_get_style_context(widget), GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);
#endif
}
void ui_set_button_style(GtkWidget *widget)
{
#ifdef GTK_STYLE_CLASS_BUTTON_ACTION
	gtk_style_context_add_class(gtk_widget_get_style_context(widget), GTK_STYLE_CLASS_BUTTON_ACTION);
#endif
}
