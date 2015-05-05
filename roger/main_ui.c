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
#include <string.h>

#include <gtk/gtk.h>

#include <libroutermanager/routermanager.h>

#include <roger/application.h>
#include <roger/main.h>

#include <config.h>

GtkApplication *application_new(void);

#ifdef G_OS_WIN32
#include <windows.h>
/**
 * \brief roger_main - windows library main function
 * \param hint hinstance
 * \param argc argument count
 * \param argv argument vector
 * \return error code
 */
int roger_main(HINSTANCE hint, int argc, char **argv);
int roger_main(HINSTANCE hint, int argc, char **argv)
{
#else
/**
 * \brief Main function
 * \param argc argument count
 * \param argv argument vector
 * \return error code
 */
int main(int argc, char **argv)
{
#endif
	int status;

	/* Set local bindings */
	bindtextdomain(GETTEXT_PACKAGE, get_directory(APP_LOCALE));
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	roger_app = application_new();

	gtk_window_set_default_icon_name("roger");
	gdk_notify_startup_complete();
	status = g_application_run(G_APPLICATION(roger_app), argc, argv);

	g_object_unref(roger_app);

	return status;
}
