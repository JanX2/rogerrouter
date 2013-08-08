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

#include <libcallibre/profile.h>
#include <libcallibre/plugins.h>

#include <main.h>

#include <pref.h>
#include <pref_router.h>
#include <pref_plugins.h>
#include <pref_filters.h>
#include <pref_profiles.h>
#include <pref_softphone.h>
#include <pref_audio.h>
#include <pref_fax.h>
#include <pref_action.h>
#include <pref_prefix.h>
#include <pref_misc.h>

static GtkWidget *dialog = NULL;

inline GtkWidget *ui_label_new(gchar *text)
{
	GtkWidget *label;
	GdkRGBA col;

	label = gtk_label_new(text);
	//gtk_widget_set_hexpand(label, TRUE);
	gdk_rgba_parse(&col, "#808080");
	gtk_widget_override_color(label, GTK_STATE_NORMAL, &col);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	return label;
}

void pref_notebook_add_page(GtkWidget *notebook, GtkWidget *page, gchar *title)
{
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), page, title);
}

GtkWidget *pref_group_create(GtkWidget *box, gchar *title_str, gboolean hexpand, gboolean vexpand)
{
	GtkWidget *grid;
	GtkWidget *align1;
	GtkWidget *title;
	gchar *title_markup = g_strdup_printf("<b>%s</b>", title_str);

	grid = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);

	/* Configure plugins label */
	title = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(title), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(title), 10, 5);
	gtk_label_set_markup(GTK_LABEL(title), title_markup);
	gtk_grid_attach(GTK_GRID(grid), title, 0, 0, 1, 1);

	/* Create alignment */
	align1 = gtk_alignment_new(0, 0.5, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align1), 0, 10, 20, 20);
	gtk_container_add(GTK_CONTAINER(align1), box);
	gtk_grid_attach(GTK_GRID(grid), align1, 0, 1, 1, 1);

	g_free(title_markup);

	return grid;
}

static void pref_response_cb(GtkDialog *dialog_win, gint response_id, gpointer user_data)
{
	gtk_widget_destroy(GTK_WIDGET(dialog_win));
	dialog = NULL;
}

GtkWindow *pref_get_window(void)
{
	return dialog ? GTK_WINDOW(dialog) : NULL;
}

void preferences(void)
{
	GtkWidget *content;
	GtkWidget *notebook = gtk_notebook_new();
	GtkWidget *page;

	if (!profile_get_active()) {
		return;
	}

	if (dialog) {
		gtk_window_present(GTK_WINDOW(dialog));
		return;
	}

	dialog = gtk_dialog_new_with_buttons(_("Preferences"), NULL, 0, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	page = pref_page_router();
	pref_notebook_add_page(notebook, page, _("Router"));

	page = pref_page_prefix();
	pref_notebook_add_page(notebook, page, _("Prefix"));

	page = pref_page_fax();
	pref_notebook_add_page(notebook, page, _("Fax"));

	page = pref_page_softphone();
	pref_notebook_add_page(notebook, page, _("Softphone"));

	page = pref_page_filters();
	pref_notebook_add_page(notebook, page, _("Filters"));

	page = pref_page_plugins();
	pref_notebook_add_page(notebook, page, _("Plugins"));

	//page = pref_page_profiles();
	//pref_notebook_add_page(notebook, page, _("Profiles"));

	page = pref_page_audio();
	pref_notebook_add_page(notebook, page, _("Audio"));

	page = pref_page_action();
	pref_notebook_add_page(notebook, page, _("Actions"));

#ifdef G_OS_WIN32
	page = pref_page_misc();
	pref_notebook_add_page(notebook, page, _("Misc"));
#endif

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);
	gtk_container_add(GTK_CONTAINER(content), notebook);

	gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 430);

	g_signal_connect(dialog, "response", G_CALLBACK(pref_response_cb), dialog);

#ifdef G_OS_WIN32
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
#endif

	gtk_widget_show_all(dialog);
}
