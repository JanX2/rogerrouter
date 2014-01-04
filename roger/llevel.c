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

#include <libroutermanager/libfaxophone/faxophone.h>
#include <libroutermanager/libfaxophone/isdn-convert.h>

#include <roger/llevel.h>

struct history {
	double value;
	struct history *next;
};

static struct history *history_new(void)
{
	struct history *root = NULL;
	struct history *temp;
	int index;

	for (index = 0; index < 10; index++) {
		temp = root;
		root = g_slice_new(struct history);
		root->value = 0.0f;
		root->next = temp;
	}

	return root;
}

static double histroy_append(struct history **history, double x)
{
	double result = x;
	struct history **hist = &(*history)->next;

	/* get new maximum */
	while (*hist != NULL) {
		if ((*hist)->value > result) {
			result = (*hist)->value;
		}
		hist = &(*hist)->next;
	}

	/* append */
	*hist = *history;
	*history = (*history)->next;
	(*hist)->next = NULL;
	(*hist)->value = x;

	return result;
}

GtkWidget *line_level_bar_new(gint width, gint height)
{
	GtkWidget *bar;
	GtkWidget *progress_bar;

	/* create box which contains our progress bars */
	bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(bar, width, height);
	g_object_set_data(G_OBJECT(bar), "width", GINT_TO_POINTER(width));

	/* progress bar: fast/slow relation progressbar */
	progress_bar = gtk_progress_bar_new();

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
	g_object_set_data(G_OBJECT(bar), "pbar1", progress_bar);
	gtk_widget_set_size_request(progress_bar, 1, -1);
	gtk_box_pack_start(GTK_BOX(bar), progress_bar, TRUE, TRUE, 0);
	gtk_widget_show(progress_bar);

	/* Rest */
	g_object_set_data(G_OBJECT(bar), "history", history_new());

	return bar;
}

void line_level_set(GtkWidget *bar, double max)
{
	GtkWidget *bar1 = g_object_get_data(G_OBJECT(bar), "pbar1");
	GtkWidget *related = g_object_get_data(G_OBJECT(bar), "bar");
	struct history *history = g_object_get_data(G_OBJECT(bar), "history");
	double maxMax = history ? histroy_append(&history, max) : max;
	double percentage;

	g_object_set_data(G_OBJECT(bar), "history", history);

	percentage = maxMax > 0.0 ? max / maxMax : 0.0;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar1), percentage);

	if (related) {
		line_level_set(related, max);
	}
}
