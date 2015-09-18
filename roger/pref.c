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

#include <libroutermanager/profile.h>
#include <libroutermanager/plugins.h>

#include <roger/main.h>
#include <roger/journal.h>

#include <roger/pref.h>
#include <roger/pref_router.h>
#include <roger/pref_plugins.h>
#include <roger/pref_filters.h>
#include <roger/pref_softphone.h>
#include <roger/pref_security.h>
#include <roger/pref_audio.h>
#include <roger/pref_fax.h>
#include <roger/pref_action.h>
#include <roger/pref_prefix.h>
#include <roger/pref_misc.h>
#include <roger/uitools.h>
#include <roger/application.h>

static GtkWidget *dialog = NULL;

void pref_notebook_add_page(GtkWidget *notebook, GtkWidget *page, gchar *title)
{
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), page, title);
}

GtkWidget *pref_group_create(GtkWidget *box, gchar *title_str, gboolean hexpand, gboolean vexpand)
{
	GtkWidget *grid;
	GtkWidget *title;
	gchar *title_markup = ui_bold_text(title_str);

	grid = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);

	/* Configure plugins label */
	title = gtk_label_new("");
	gtk_widget_set_halign(title, GTK_ALIGN_START);
	gtk_widget_set_margin(title, 10, 5, 10, 5);
	gtk_label_set_markup(GTK_LABEL(title), title_markup);
	gtk_grid_attach(GTK_GRID(grid), title, 0, 0, 1, 1);

	gtk_widget_set_margin(box, 20, 0, 20, 10);
	gtk_grid_attach(GTK_GRID(grid), box, 0, 1, 1, 1);

	g_free(title_markup);

	return grid;
}

GtkWindow *pref_get_window(void)
{
	return dialog ? GTK_WINDOW(dialog) : NULL;
}

void dialog_destroy_cb(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
	dialog = NULL;
}

void preferences(void)
{
	GtkWidget *notebook = gtk_notebook_new();
	GtkWidget *page;
	GtkWidget *parent = NULL;

	if (!profile_get_active()) {
		return;
	}

	if (dialog) {
		gtk_window_present(GTK_WINDOW(dialog));
		return;
	}

	parent = journal_get_window();

	dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Preferences"));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_application_add_window(roger_app, GTK_WINDOW(dialog));
	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(dialog_destroy_cb), NULL);

	if (roger_uses_headerbar()) {
		/* Create header bar and add it to the window */
		GtkWidget *header_bar = gtk_header_bar_new();
		gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
		gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), _("Preferences"));
		gtk_window_set_titlebar(GTK_WINDOW(dialog), header_bar);
	}

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

	page = pref_page_security();
	pref_notebook_add_page(notebook, page, _("Security"));

	page = pref_page_audio();
	pref_notebook_add_page(notebook, page, _("Audio"));

	page = pref_page_action();
	pref_notebook_add_page(notebook, page, _("Actions"));

#ifdef G_OS_WIN32
	page = pref_page_misc();
	pref_notebook_add_page(notebook, page, _("Misc"));
#endif

	//gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);
	gtk_container_add(GTK_CONTAINER(dialog), notebook);

	//gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 430);

	gtk_widget_show_all(dialog);
}
