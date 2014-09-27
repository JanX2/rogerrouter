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

#include <stdlib.h>

#include <gtk/gtk.h>

#include <libroutermanager/routermanager.h>

#include <roger/main.h>
#include <roger/journal.h>

#include <config.h>

/**
 * \brief Dialog response callback - destroy dialog
 * \param widget dialog widget we will destroy
 * \param user_data UNUSED
 */
static void about_response(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
}

/**
 * \brief About dialog
 */
void app_show_about(void)
{
	GtkWidget *dialog = NULL;
	const gchar *authors[] = {
		"Jan-Michael Brummer <jan.brummer@tabos.org>",
		"Louis Lagendijk <louis@fazant.net>",
		NULL
	};
	const gchar *documenters[] = {
		"Jan-Michael Brummer <jan.brummer@tabos.org>",
		NULL
	};
	char *translators =
	    "Jan-Michael Brummer <jan.brummer@tabos.org>";
	gchar *path = g_strconcat(get_directory(APP_DATA), G_DIR_SEPARATOR_S, "app.png", NULL);

	/* create about dialog */
	dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), PACKAGE_NAME);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), PACKAGE_VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "(C) 2012-2014, Jan-Michael Brummer <jan.brummer@tabos.org>");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("FRITZ!Box Journal, Soft/phone, and Fax\nDedicated to my father"));
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_2_0);
	gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(dialog), TRUE);
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(dialog), documenters);
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), g_locale_to_utf8(translators, -1, 0, 0, 0));
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), PACKAGE_BUGREPORT);
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), gdk_pixbuf_new_from_file(path, NULL));
	g_free(path);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(about_response), dialog);

	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(journal_get_window()));

	gtk_dialog_run(GTK_DIALOG(dialog));
}
