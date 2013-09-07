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

#include <roger/uitools.h>

inline GtkWidget *ui_label_new(gchar *text)
{
	GtkWidget *label;
	GdkRGBA col;

	label = gtk_label_new(text);
	gdk_rgba_parse(&col, "#808080");
	gtk_widget_override_color(label, GTK_STATE_NORMAL, &col);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	return label;
}

gchar *ui_bold_text(gchar *text)
{
	return g_strdup_printf("<b>%s</b>", text);
}
