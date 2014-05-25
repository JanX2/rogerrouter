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

#include <string.h>

#include <gtk/gtk.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/router.h>
#include <libroutermanager/ftp.h>
#include <libroutermanager/password.h>
#include <libroutermanager/net_monitor.h>

#include <roger/main.h>
#include <roger/journal.h>
#include <roger/uitools.h>

#include <config.h>

static struct profile *profile = NULL;

static void profile_entry_changed(GtkEditable *entry, GtkAssistant *assistant)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
	gint page_num = gtk_assistant_get_current_page(assistant);
	GtkWidget *page = gtk_assistant_get_nth_page(assistant, page_num);
	GtkWidget *warning_label = g_object_get_data(G_OBJECT(assistant), "warning_label");
	GSList *profile_list = profile_get_list();
	struct profile *profile;

	while (profile_list != NULL) {
		profile = profile_list->data;
		g_assert(profile != NULL);

		if (!strcmp(profile->name, text)) {
			gtk_assistant_set_page_complete(assistant, page, FALSE);
			gtk_label_set_text(GTK_LABEL(warning_label), _("Profile name already in use!"));
			return;
		}

		profile_list = profile_list->next;
	}

	gtk_label_set_text(GTK_LABEL(warning_label), "");
	gtk_assistant_set_page_complete(assistant, page, TRUE);
}

void router_entry_changed(GtkEditable *entry, GtkAssistant *assistant)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
	gint page_num = gtk_assistant_get_current_page(assistant);
	GtkWidget *page = gtk_assistant_get_nth_page(assistant, page_num);

	gtk_assistant_set_page_complete(assistant, page, strlen(text) > 0);
}

static void assistant_apply(GtkAssistant *assistant, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(assistant));
}

static void assistant_cancel(GtkAssistant *assistant, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(assistant));
}

static void assistant_close(GtkAssistant *assistant, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(assistant));
}

gpointer get_settings_thread(gpointer data)
{
	GtkAssistant *assistant = data;
	GtkWidget *settings_spinner = g_object_get_data(G_OBJECT(assistant), "settings_spinner");
	GtkWidget *profile_entry = g_object_get_data(G_OBJECT(assistant), "profile_entry");
	GtkWidget *router_entry = g_object_get_data(G_OBJECT(assistant), "router_other_entry");
	GtkWidget *settings_label = g_object_get_data(G_OBJECT(assistant), "settings_label");
	GtkWidget *router_user_entry = g_object_get_data(G_OBJECT(assistant), "router_user_entry");
	GtkWidget *router_password_entry = g_object_get_data(G_OBJECT(assistant), "router_password_entry");
	GtkWidget *router_ftp_user_entry = g_object_get_data(G_OBJECT(assistant), "router_ftp_user_entry");
	GtkWidget *router_ftp_password_entry = g_object_get_data(G_OBJECT(assistant), "router_ftp_password_entry");
	GtkWidget *radio_button = g_object_get_data(G_OBJECT(assistant), "radio_button");
	const gchar *host = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_button)) ? gtk_entry_get_text(GTK_ENTRY(router_entry)) : "fritz.box";
	const gchar *user = gtk_entry_get_text(GTK_ENTRY(router_user_entry));
	const gchar *password = gtk_entry_get_text(GTK_ENTRY(router_password_entry));
	const gchar *ftp_user = gtk_entry_get_text(GTK_ENTRY(router_ftp_user_entry));
	const gchar *ftp_password = gtk_entry_get_text(GTK_ENTRY(router_ftp_password_entry));
	const gchar *name = gtk_entry_get_text(GTK_ENTRY(profile_entry));
	gchar *message = NULL;
	gboolean complete = FALSE;
	gint num = gtk_assistant_get_current_page(assistant);
	GtkWidget *page = gtk_assistant_get_nth_page(assistant, num);

	profile = profile_new(name, host, user, password);

	/* Check for router */
	if (router_present(profile->router_info) == FALSE) {
		/* Error: Could not detect box on this host name */
		message = g_strdup(_("Error: Could not detect box on this host name\nPlease check the host name"));
		goto end;
	}

	/* Get settings */
	if (router_get_settings(profile) == FALSE) {
		/* Error: Could not get settings from box */
		message = g_strdup(_("Error: Could not get settings from box\nPlease check your user/password."));
		goto end;
	}

	/* Test ftp login */
	struct ftp *ftp = ftp_init(host);
	if (ftp) {
		if (!ftp_login(ftp, ftp_user, ftp_password)) {
			/* Error: Could not login to ftp */
			message = g_strdup(_("Error: Could not login to box ftp server\nPlease check your ftp user/password."));
			ftp_shutdown(ftp);
			goto end;
		}
		ftp_shutdown(ftp);
	}

	/* Enable telnet & capi port */
	if (router_dial_number(profile, 1, "#96*5*")) {
		g_usleep(G_USEC_PER_SEC);
		router_hangup(profile, 1, "#96*5*");
	}
	if (router_dial_number(profile, 1, "#96*3*")) {
		g_usleep(G_USEC_PER_SEC);
		router_hangup(profile, 1, "#96*3*");
	}

	g_settings_set_string(profile->settings, "serial", profile->router_info->serial);

	g_settings_set_string(profile->settings, "ftp-user", ftp_user);
	password_manager_set_password(profile, "ftp-password", ftp_password);

	/* Set initial fax report dir */
	g_settings_set_string(profile->settings, "fax-report-dir", g_get_home_dir());

	/* Set initial softphone number */
	gchar **numbers = router_get_numbers(profile);
	if (numbers && numbers[0]) {
		g_settings_set_string(profile->settings, "phone-number", numbers[0]);
	}

	message = g_strdup_printf(_("New box integrated:\n%s\nCountry-Prefix: %s\nCity-Prefix: %s"),
	                          router_get_name(profile),
	                          router_get_country_code(profile),
	                          router_get_area_code(profile));
	complete = TRUE;

end:
	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(assistant)), NULL);

	gtk_label_set_text(GTK_LABEL(settings_label), message);

	gtk_assistant_set_page_complete(assistant, page, complete);
	gtk_spinner_stop(GTK_SPINNER(settings_spinner));
	gtk_widget_hide(settings_spinner);
	gtk_widget_show(settings_label);
	g_free(message);

	return NULL;
}

static void assistant_prepare(GtkAssistant *assistant, GtkWidget *page, gpointer data)
{
	gint page_num = gtk_assistant_get_current_page(assistant);

	switch (page_num) {
	case 0:
		/* Introduction */
		break;
	case 1:
		/* Profile name */
		break;
	case 2: {
		/* Router detection */
		gtk_assistant_set_page_complete(assistant, page, TRUE);
		break;
	}
	case 3: {
		/* Get settings & summary */
		GtkWidget *settings_spinner = g_object_get_data(G_OBJECT(assistant), "settings_spinner");
		GtkWidget *settings_label = g_object_get_data(G_OBJECT(assistant), "settings_label");

		gtk_label_set_text(GTK_LABEL(settings_label), "");
		gtk_widget_set_visible(settings_label, FALSE);

		gtk_widget_set_visible(settings_spinner, TRUE);
		gtk_spinner_start(GTK_SPINNER(settings_spinner));

		g_thread_new("Get settings", get_settings_thread, assistant);

		break;
	}
	case 4:
		g_debug("Adding profile");
		profile_add(profile);
		g_debug("Commit profile");
		profile_commit();
		g_debug("Reconnect monitor");
		net_monitor_reconnect();
		break;
	default:
		g_warning("Unknown page %d\n", page_num);
	}
}

GtkWidget *page_create_intro(void)
{
	GtkWidget *label;

	label = gtk_label_new(_("In the next few steps a new router box will be integrated into Roger Router\n"\
	                        "First performing a scan for all available devices and then\n"\
	                        "acquiring all needed information."));

	gtk_widget_set_margin(label, 15, 10, 15, 10);

	return label;
}

GtkWidget *page_create_profile(GtkWidget *window)
{
	GtkWidget *profile_grid;
	GtkWidget *profile_label;
	GtkWidget *profile_entry;
	GtkWidget *profile_warning_label;

	profile_grid = gtk_grid_new();

	/* Profile label */
	profile_label = gtk_label_new("");
	g_object_set_data(G_OBJECT(window), "profile_label", profile_label);
	gchar *tmp = ui_bold_text(_("Enter new profile name"));
	gtk_label_set_markup(GTK_LABEL(profile_label), tmp);
	g_free(tmp);
	gtk_widget_set_halign(profile_label, GTK_ALIGN_START);
	gtk_widget_set_margin(profile_label, 10, 10, 10, 10);
	gtk_grid_attach(GTK_GRID(profile_grid), profile_label, 0, 0, 1, 1);

	/* Profile entry */
	profile_entry = gtk_entry_new();
	gtk_widget_set_margin(profile_entry, 35, 5, 35, 5);
	g_object_set_data(G_OBJECT(window), "profile_entry", profile_entry);
	g_signal_connect(G_OBJECT(profile_entry), "changed", G_CALLBACK(profile_entry_changed), window);
	gtk_widget_set_tooltip_text(profile_entry, "Enter a unique name for your profile you can remember (e.g. \"Los Angeles\", \"At home\", \"Mum\" or whatever you like)");
	gtk_entry_set_activates_default(GTK_ENTRY(profile_entry), TRUE);

	gtk_grid_attach(GTK_GRID(profile_grid), profile_entry, 0, 1, 1, 1);

	/* Profile warning label */
	profile_warning_label = gtk_label_new("");
	g_object_set_data(G_OBJECT(window), "warning_label", profile_warning_label);
	gtk_widget_set_hexpand(profile_warning_label, TRUE);
	gtk_widget_set_vexpand(profile_warning_label, TRUE);
	gtk_grid_attach(GTK_GRID(profile_grid), profile_warning_label, 0, 2, 1, 1);

	return profile_grid;
}

GtkWidget *page_create_router(GtkWidget *window)
{
	GtkWidget *router_grid;
	GtkWidget *router_label;
	GtkWidget *router_other_entry;
	GtkWidget *router_innergrid;
	GtkWidget *radio_button;
	GtkWidget *router_userdata_grid;
	GtkWidget *router_user_name;
	GtkWidget *router_user_label;
	GtkWidget *router_user_entry;
	GtkWidget *router_password_label;
	GtkWidget *router_password_entry;
	GtkWidget *router_ftp_userdata_grid;
	GtkWidget *router_ftp_user_name;
	GtkWidget *router_ftp_user_label;
	GtkWidget *router_ftp_user_entry;
	GtkWidget *router_ftp_password_label;
	GtkWidget *router_ftp_password_entry;
	GSList *radio_list = NULL;
	struct router_info router_info;

	router_grid = gtk_grid_new();

	/* Select router label */
	router_label = gtk_label_new("");
	gchar *tmp = ui_bold_text(_("Select your router:"));
	gtk_label_set_markup(GTK_LABEL(router_label), tmp);
	g_free(tmp);
	gtk_widget_set_halign(router_label, GTK_ALIGN_START);
	gtk_widget_set_margin(router_label, 10, 10, 10, 10);
	gtk_grid_attach(GTK_GRID(router_grid), router_label, 0, 0, 1, 1);

	/* Inner grid for radio buttons */
	router_innergrid = gtk_grid_new();
	gtk_widget_set_margin(router_innergrid, 35, 5, 35, 5);

	memset(&router_info, 0, sizeof(struct router_info));
	router_info.host = g_strdup("fritz.box");

	/* Scan for router and add detected devices */
	if (router_present(&router_info) == TRUE) {
		const gchar *router_name = router_info.name;
		gchar *name = g_strconcat(router_name, _(" on: "), router_info.host, NULL);
		radio_button = gtk_radio_button_new_with_label(NULL, name);
		g_free(name);
		gtk_grid_attach(GTK_GRID(router_innergrid), radio_button, 0, 0, 1, 1);
		radio_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button));
	}
	g_free(router_info.host);

	/* In every case add also an other button + entry box */
	radio_button = gtk_radio_button_new_with_label(radio_list, _("Other"));
	g_object_set_data(G_OBJECT(window), "radio_button", radio_button);
	gtk_grid_attach(GTK_GRID(router_innergrid), radio_button, 0, 1, 1, 1);

	/* Other entry box */
	router_other_entry = gtk_entry_new();
	gtk_widget_set_margin(router_other_entry, 10, 0, 10, 0);
	gtk_widget_set_hexpand(router_other_entry, TRUE);
	g_object_set_data(G_OBJECT(window), "router_other_entry", router_other_entry);
	gtk_grid_attach(GTK_GRID(router_innergrid), router_other_entry, 0, 2, 1, 1);

	gtk_grid_attach(GTK_GRID(router_grid), router_innergrid, 0, 2, 1, 1);

	radio_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button));

	/* Login data */

	/* Enter user label */
	router_user_label = gtk_label_new("");
	tmp = ui_bold_text(_("Enter login data:"));
	gtk_label_set_markup(GTK_LABEL(router_user_label), tmp);
	g_free(tmp);
	gtk_widget_set_halign(router_user_label, GTK_ALIGN_START);
	gtk_widget_set_margin(router_user_label, 10, 10, 10, 10);
	gtk_grid_attach(GTK_GRID(router_grid), router_user_label, 0, 4, 1, 1);

	/* User entry box */
	router_userdata_grid = gtk_grid_new();
	gtk_widget_set_margin(router_userdata_grid, 35, 5, 35, 5);

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(router_userdata_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(router_userdata_grid), 5);

	gtk_grid_attach(GTK_GRID(router_grid), router_userdata_grid, 0, 5, 1, 1);

	router_user_name = gtk_label_new("User:");
	gtk_grid_attach(GTK_GRID(router_userdata_grid), router_user_name, 0, 0, 1, 1);

	router_user_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(router_user_entry, _("Enter user if needed"));
	g_object_set_data(G_OBJECT(window), "router_user_entry", router_user_entry);
	gtk_widget_set_hexpand(router_user_entry, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(router_user_entry), TRUE);
	gtk_grid_attach(GTK_GRID(router_userdata_grid), router_user_entry, 1, 0, 1, 1);

	/* Enter password label */
	router_password_label = gtk_label_new("Password:");
	gtk_grid_attach(GTK_GRID(router_userdata_grid), router_password_label, 0, 1, 1, 1);

	/* Password entry box */
	router_password_entry = gtk_entry_new();
	g_object_set_data(G_OBJECT(window), "router_password_entry", router_password_entry);
	gtk_entry_set_visibility(GTK_ENTRY(router_password_entry), FALSE);
	gtk_widget_set_hexpand(router_password_entry, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(router_password_entry), TRUE);
	gtk_grid_attach(GTK_GRID(router_userdata_grid), router_password_entry, 1, 1, 1, 1);

	/* FTP Login data */

	/* Enter user label */
	router_ftp_user_label = gtk_label_new("");
	tmp = ui_bold_text(_("Enter FTP login data:"));
	gtk_label_set_markup(GTK_LABEL(router_ftp_user_label), tmp);
	g_free(tmp);
	gtk_widget_set_halign(router_ftp_user_label, GTK_ALIGN_START);
	gtk_widget_set_margin(router_ftp_user_label, 10, 10, 10, 10);
	gtk_grid_attach(GTK_GRID(router_grid), router_ftp_user_label, 0, 6, 1, 1);

	/* User entry box */
	router_ftp_userdata_grid = gtk_grid_new();
	gtk_widget_set_margin(router_ftp_userdata_grid, 35, 5, 35, 5);

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(router_ftp_userdata_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(router_ftp_userdata_grid), 5);

	gtk_grid_attach(GTK_GRID(router_grid), router_ftp_userdata_grid, 0, 7, 1, 1);

	router_ftp_user_name = gtk_label_new("User:");
	gtk_grid_attach(GTK_GRID(router_ftp_userdata_grid), router_ftp_user_name, 0, 0, 1, 1);

	router_ftp_user_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(router_ftp_user_entry), "ftpuser");
	g_object_set_data(G_OBJECT(window), "router_ftp_user_entry", router_ftp_user_entry);
	gtk_widget_set_hexpand(router_ftp_user_entry, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(router_ftp_user_entry), TRUE);
	gtk_grid_attach(GTK_GRID(router_ftp_userdata_grid), router_ftp_user_entry, 1, 0, 1, 1);

	/* Enter password label */
	router_ftp_password_label = gtk_label_new("Password:");
	gtk_grid_attach(GTK_GRID(router_ftp_userdata_grid), router_ftp_password_label, 0, 1, 1, 1);

	/* Password entry box */
	router_ftp_password_entry = gtk_entry_new();
	g_object_set_data(G_OBJECT(window), "router_ftp_password_entry", router_ftp_password_entry);
	gtk_entry_set_visibility(GTK_ENTRY(router_ftp_password_entry), FALSE);
	gtk_widget_set_hexpand(router_ftp_password_entry, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(router_ftp_password_entry), TRUE);
	gtk_grid_attach(GTK_GRID(router_ftp_userdata_grid), router_ftp_password_entry, 1, 1, 1, 1);

	gtk_widget_grab_focus(router_other_entry);

	gtk_widget_show_all(router_grid);

	return router_grid;
}

GtkWidget *page_create_settings(GtkWidget *window)
{
	GtkWidget *settings_grid;
	GtkWidget *settings_label;
	GtkWidget *settings_spinner;

	settings_grid = gtk_grid_new();

	settings_spinner = gtk_spinner_new();
	gtk_widget_set_hexpand(settings_spinner, TRUE);
	gtk_widget_set_vexpand(settings_spinner, TRUE);
	gtk_grid_attach(GTK_GRID(settings_grid), settings_spinner, 0, 0, 1, 1);

	settings_label = gtk_label_new("");
	gtk_widget_set_hexpand(settings_label, TRUE);
	gtk_widget_set_vexpand(settings_label, TRUE);
	gtk_grid_attach(GTK_GRID(settings_grid), settings_label, 0, 1, 1, 1);

	g_object_set_data(G_OBJECT(window), "settings_label", settings_label);
	g_object_set_data(G_OBJECT(window), "settings_spinner", settings_spinner);

	return settings_grid;
}

void assistant()
{
	GtkWidget *window;
	GtkWidget *intro;
	GtkWidget *profile;
	GtkWidget *router;
	GtkWidget *settings;
	GtkWidget *summary_label;

	window = gtk_assistant_new();

	/* Page 1 - Introduction */
	intro = page_create_intro();
	gtk_assistant_append_page(GTK_ASSISTANT(window), intro);
	gtk_assistant_set_page_type(GTK_ASSISTANT(window), intro, GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_title(GTK_ASSISTANT(window), intro, _("Introduction"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(window), intro, TRUE);

	/* Page 2 - Enter profile name */
	profile = page_create_profile(window);
	gtk_assistant_append_page(GTK_ASSISTANT(window), profile);
	gtk_assistant_set_page_type(GTK_ASSISTANT(window), profile, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(window), profile, _("Enter new profile name"));

	/* Page 3 - Select router box - filled by prepare function */
	router = page_create_router(window);
	gtk_assistant_append_page(GTK_ASSISTANT(window), router);
	gtk_assistant_set_page_title(GTK_ASSISTANT(window), router, _("Select router box"));
	gtk_assistant_set_page_type(GTK_ASSISTANT(window), router, GTK_ASSISTANT_PAGE_CONTENT);

	/* Page 4 - Get settings */
	settings = page_create_settings(window);
	gtk_assistant_append_page(GTK_ASSISTANT(window), settings);
	gtk_assistant_set_page_title(GTK_ASSISTANT(window), settings, _("Get settings"));
	gtk_assistant_set_page_type(GTK_ASSISTANT(window), settings, GTK_ASSISTANT_PAGE_CONTENT);

	/* Page 5 - Summary */
	summary_label = gtk_label_new(_("The profile setup is now complete. Please go to the preferences and customize Roger Router.\n\nIf you like Roger Router please consider a donation.\n\nWe wish you much fun!\n"));

	gtk_assistant_append_page(GTK_ASSISTANT(window), summary_label);
	gtk_assistant_set_page_title(GTK_ASSISTANT(window), summary_label, _("Summary"));
	gtk_assistant_set_page_type(GTK_ASSISTANT(window), summary_label, GTK_ASSISTANT_PAGE_SUMMARY);

	g_signal_connect(G_OBJECT(window), "apply", G_CALLBACK(assistant_apply), NULL);
	g_signal_connect(G_OBJECT(window), "cancel", G_CALLBACK(assistant_cancel), NULL);
	g_signal_connect(G_OBJECT(window), "close", G_CALLBACK(assistant_close), NULL);
	g_signal_connect(G_OBJECT(window), "prepare", G_CALLBACK(assistant_prepare), NULL);

	gtk_window_set_title(GTK_WINDOW(window), _(PACKAGE_NAME " Profile Assistant"));
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 300, 300);

	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
	gtk_widget_show_all(window);
}
