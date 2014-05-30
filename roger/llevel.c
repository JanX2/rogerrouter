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

#define PROGRESS_BAR 1

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

GtkWidget *line_level_bar_new(void)
{
	GtkWidget *bar;

#ifdef PROGRESS_BAR
	/* progress bar: fast/slow relation progressbar */
	bar = gtk_progress_bar_new();

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar), 0.0);
	gtk_progress_bar_set_inverted(GTK_PROGRESS_BAR(bar), TRUE);
#else
	bar = gtk_level_bar_new();
	gtk_level_bar_set_value(GTK_LEVEL_BAR(bar), 0.0);
	gtk_level_bar_set_inverted(GTK_LEVEL_BAR(bar), TRUE);
#endif
	gtk_orientable_set_orientation(GTK_ORIENTABLE(bar), GTK_ORIENTATION_VERTICAL);
	g_object_set_data(G_OBJECT(bar), "history", history_new());

	return bar;
}

void line_level_set(GtkWidget *bar, double max)
{
	struct history *history = g_object_get_data(G_OBJECT(bar), "history");
	double max_max = history ? histroy_append(&history, max) : max;
	double percentage;

	g_object_set_data(G_OBJECT(bar), "history", history);

	percentage = max_max > 0.0 ? max / max_max : 0.0;

#ifdef PROGRESS_BAR
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar), percentage);
#else
	gtk_level_bar_set_value(GTK_LEVEL_BAR(bar), percentage);
#endif
}
