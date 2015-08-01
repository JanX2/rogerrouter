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

#ifdef OLD_ASSISTANT

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
		router_hangup(profile, 1, "#96*5*");
	}
	if (router_dial_number(profile, 1, "#96*3*")) {
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

#else
#include <libroutermanager/routermanager.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/appobject-emit.h>

#include <roger/main.h>
#include <roger/journal.h>

struct assistant_priv {
	gint current_page;
	gint max_page;
	gint page;
	GtkWidget *assistant;
	GtkWidget *back;
	GtkWidget *next;
	GtkWidget *profile_name;
	GtkWidget *server;
	GtkWidget *user;
	GtkWidget *router_stack;
	GtkWidget *password;
	GtkWidget *password_spinner;

	GtkWidget *ftp_user;
	GtkWidget *ftp_password;

	struct profile *profile;
};

struct assistant_page {
	GtkWidget *(*create_child)(struct assistant_priv *priv);
	gboolean (*pre)(struct assistant_priv *priv);
	gboolean (*post)(struct assistant_priv *priv);
	gchar *name;
};

GtkWidget *welcome_page(struct assistant_priv *priv)
{
	GtkWidget *grid;
	GtkWidget *welcome;
	GtkWidget *logo;
	GtkWidget *description;
	gchar *path = g_strconcat(get_directory(APP_DATA), G_DIR_SEPARATOR_S, "app.png", NULL);

	/* Welcome */
	grid = gtk_grid_new();

	welcome = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(welcome), _("<span font_desc=\"30.0\"><b>Welcome!</b></span>"));
	gtk_widget_set_hexpand(welcome, TRUE);
	gtk_label_set_justify(GTK_LABEL(welcome), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), welcome, 0, 0, 1, 1);

	logo = gtk_image_new_from_file(path);
	gtk_grid_attach(GTK_GRID(grid), logo, 0, 1, 1, 1);

	description = gtk_label_new(_("In order to use Roger Router you have to connect a router with the application.\nThe following wizard will help you adding a router to the application.\n"));
	gtk_grid_attach(GTK_GRID(grid), description, 0, 2, 1, 1);

	gtk_widget_set_margin(description, 15, 10, 15, 10);
	g_free(path);

	return grid;
}

gboolean welcome_pre(struct assistant_priv *priv)
{
	g_debug("%s(): called", __FUNCTION__);
	gtk_widget_set_sensitive(priv->back, FALSE);
	gtk_widget_grab_focus(priv->next);

	return FALSE;
}

static void profile_entry_changed(GtkEditable *entry, struct assistant_priv *priv)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
	GSList *profile_list = profile_get_list();
	struct profile *profile;

	while (profile_list != NULL) {
		profile = profile_list->data;
		g_assert(profile != NULL);

		if (!strcmp(profile->name, text)) {
			gtk_widget_set_sensitive(priv->next, FALSE);
			//gtk_label_set_text(GTK_LABEL(warning_label), _("Profile name already in use!"));
			return;
		}

		profile_list = profile_list->next;
	}

	//gtk_label_set_text(GTK_LABEL(warning_label), "");
	gtk_widget_set_sensitive(priv->next, !EMPTY_STRING(text));
	//gtk_assistant_set_page_complete(assistant, page, TRUE);
}

gboolean profile_pre(struct assistant_priv *priv)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(priv->profile_name));
	g_debug("%s(): called", __FUNCTION__);
	gtk_widget_set_sensitive(priv->next, !EMPTY_STRING(text));
	gtk_widget_grab_focus(priv->profile_name);

	return FALSE;
}

GtkWidget *profile_page(struct assistant_priv *priv)
{
	GtkWidget *grid;
	GtkWidget *label;
	GtkWidget *name;

	/* Box */
	grid = gtk_grid_new();
	gtk_widget_set_hexpand(grid, TRUE);
	gtk_widget_set_vexpand(grid, TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

	label = ui_label_new(_("Profile name:"));
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

	name = gtk_entry_new();
	priv->profile_name = name;
	gtk_widget_set_hexpand(name, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(name), TRUE);
	g_signal_connect(G_OBJECT(name), "changed", G_CALLBACK(profile_entry_changed), priv);
	gtk_grid_attach(GTK_GRID(grid), name, 1, 0, 1, 1);
	priv->user = name;

	return grid;
}

void router_listbox_destroy(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
}

gboolean scan(gpointer listbox)
{
	struct router_info router_info;

	/* For all children of list box, call router_listbox_destroy() */
	gtk_container_foreach(GTK_CONTAINER(listbox), router_listbox_destroy, NULL);

	memset(&router_info, 0, sizeof(struct router_info));
	router_info.host = g_strdup("fritz.box");

	/* Scan for router and add detected devices */
	if (router_present(&router_info) == TRUE) {
		GtkWidget *new_device;
		gchar *tmp;

		tmp = g_strdup_printf("<b>%s</b>\n<small>on %s</small>", router_info.name, router_info.host);

		new_device = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(new_device), tmp);
		gtk_widget_show(new_device);

		gtk_list_box_prepend(GTK_LIST_BOX(listbox), new_device);
	}
	g_free(router_info.host);

	return G_SOURCE_REMOVE;
}

static GtkWidget *router_listbox;

void router_row_selected_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	struct assistant_priv *priv = user_data;

	g_object_set_data(G_OBJECT(priv->router_stack), "server", "fritz.box");
	gtk_widget_set_sensitive(priv->next, TRUE);
}

void router_entry_changed(GtkEditable *entry, struct assistant_priv *priv)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	gtk_widget_set_sensitive(priv->next, strlen(text) > 0);
}

GtkWidget *router_page(struct assistant_priv *priv)
{
	GtkWidget *grid;
	GtkWidget *switcher;
	GtkWidget *stack;
	GtkWidget *frame;
	GtkWidget *spinner;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *manual_grid;

	/* Box */
	grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);

	stack = gtk_stack_new();
	priv->router_stack = stack;
	gtk_widget_set_vexpand(stack, TRUE);
	gtk_widget_set_hexpand(stack, TRUE);
	//gtk_widget_set_size_request(stack, 200, 100);

	frame = gtk_frame_new(NULL);
	router_listbox = gtk_list_box_new();
	gtk_container_add(GTK_CONTAINER(frame), router_listbox);

	spinner = gtk_spinner_new();
	gtk_spinner_start(GTK_SPINNER(spinner));
	gtk_widget_show(spinner);
	gtk_list_box_set_placeholder(GTK_LIST_BOX(router_listbox), spinner);

	g_signal_connect(router_listbox, "row-selected", G_CALLBACK(router_row_selected_cb), priv);

	gtk_stack_add_titled(GTK_STACK(stack), frame, _("Automatic"), _("Automatic"));

	manual_grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(manual_grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(manual_grid), 6);

	label = ui_label_new(_("Router IP:"));
	gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
	gtk_grid_attach(GTK_GRID(manual_grid), label, 0, 0, 1, 1);
	entry = gtk_entry_new();
	g_signal_connect(entry, "changed", G_CALLBACK(router_entry_changed), priv);
	gtk_widget_set_halign(entry, GTK_ALIGN_CENTER);
	gtk_grid_attach(GTK_GRID(manual_grid), entry, 1, 0, 1, 1);
	gtk_stack_add_titled(GTK_STACK(stack), manual_grid, _("Manual"), _("Manual"));
	priv->server = entry;

	switcher = gtk_stack_switcher_new();
	gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
	gtk_widget_set_hexpand(switcher, TRUE);
	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
	gtk_grid_attach(GTK_GRID(grid), switcher, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), stack, 0, 1, 1, 1);

	gtk_widget_show_all(grid);

	return grid;
}

gboolean router_pre(struct assistant_priv *priv)
{
	g_debug("%s(): called", __FUNCTION__);
	gtk_widget_set_sensitive(priv->next, FALSE);
	g_idle_add(scan, router_listbox);

	return FALSE;
}

gboolean router_post(struct assistant_priv *priv)
{
	struct router_info router_info;
	const gchar *server;

	g_debug("%s(): called", __FUNCTION__);

	if (!strcmp(gtk_stack_get_visible_child_name(GTK_STACK(priv->router_stack)), "Manual")) {
		server = gtk_entry_get_text(GTK_ENTRY(priv->server));
		g_object_set_data(G_OBJECT(priv->router_stack), "server", (gchar*)server);
	} else {
		server = g_object_get_data(G_OBJECT(priv->router_stack), "server");
	}

	memset(&router_info, 0, sizeof(struct router_info));
	router_info.host = g_strdup(server);

	/* Scan for router and add detected devices */
	if (router_present(&router_info) == FALSE) {
		emit_message(0, _("Could not load router configuration. Please change the router ip and try again"));
		return FALSE;
	}

	return TRUE;
}

GtkWidget *password_page(struct assistant_priv *priv)
{
	GtkWidget *grid;
	GtkWidget *label;
	GtkWidget *name;
	GtkWidget *password;

	/* Box */
	grid = gtk_grid_new();
	gtk_widget_set_hexpand(grid, TRUE);
	gtk_widget_set_vexpand(grid, TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

	label = ui_label_new(_("Router user:"));
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

	name = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(name), _("Optional"));
	gtk_grid_attach(GTK_GRID(grid), name, 1, 0, 1, 1);
	priv->user = name;

	label = ui_label_new(_("Router password:"));
	gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

	password = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(password), TRUE);
	gtk_entry_set_visibility(GTK_ENTRY(password), FALSE);
	gtk_grid_attach(GTK_GRID(grid), password, 1, 1, 1, 1);
	priv->password = password;

	priv->password_spinner = gtk_spinner_new();
	gtk_widget_set_hexpand(priv->password_spinner, TRUE);
	gtk_widget_set_vexpand(priv->password_spinner, TRUE);
	gtk_grid_attach(GTK_GRID(grid), priv->password_spinner, 0, 2, 1, 1);
	gtk_widget_set_no_show_all(priv->password_spinner, TRUE);

	return grid;
}

gboolean password_post(struct assistant_priv *priv)
{
	const gchar *name = gtk_entry_get_text(GTK_ENTRY(priv->profile_name));
	const gchar *host = g_object_get_data(G_OBJECT(priv->router_stack), "server");
	const gchar *user = gtk_entry_get_text(GTK_ENTRY(priv->user));
	const gchar *password = gtk_entry_get_text(GTK_ENTRY(priv->password));
	priv->profile = profile_new(name, host, user, password);
	gboolean result = FALSE;
	gchar *message;

	gtk_widget_show(priv->password_spinner);
	gtk_widget_set_visible(priv->password_spinner, TRUE);
	gtk_spinner_start(GTK_SPINNER(priv->password_spinner));

	/* Check for router */
	if (router_present(priv->profile->router_info) == FALSE) {
		message = g_strdup(_("Could not detect box on this host name\nPlease check the host name"));
		emit_message(0, message);
		goto out;
	}

	/* Get settings */
	if (router_get_settings(priv->profile) == FALSE) {
		message = g_strdup(_("Login failed. Login data are wrong or permissions are missing.\nPlease check your login data."));
		emit_message(0, message);
		goto out;
	}

	result = TRUE;
out:
	gtk_spinner_stop(GTK_SPINNER(priv->password_spinner));
	gtk_widget_set_visible(priv->password_spinner, FALSE);

	return result;
}

GtkWidget *ftp_password_page(struct assistant_priv *priv)
{
	GtkWidget *grid;
	GtkWidget *label;
	GtkWidget *ftp_user;
	GtkWidget *ftp_password;

	/* Box */
	grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

	label = ui_label_new(_("FTP User:"));
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

	ftp_user = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(grid), ftp_user, 1, 0, 1, 1);
	priv->ftp_user = ftp_user;

	label = ui_label_new(_("FTP Password:"));
	gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

	ftp_password = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(ftp_password), TRUE);
	gtk_entry_set_visibility(GTK_ENTRY(ftp_password), FALSE);
	gtk_grid_attach(GTK_GRID(grid), ftp_password, 1, 1, 1, 1);
	priv->ftp_password = ftp_password;

	return grid;
}

gboolean ftp_password_pre(struct assistant_priv *priv)
{
	const gchar *user = gtk_entry_get_text(GTK_ENTRY(priv->user));
	const gchar *password = gtk_entry_get_text(GTK_ENTRY(priv->password));

	gtk_entry_set_text(GTK_ENTRY(priv->ftp_user), EMPTY_STRING(user) ? "ftpuser" : user);
	gtk_entry_set_text(GTK_ENTRY(priv->ftp_password), password);

	return FALSE;
}

gboolean ftp_password_post(struct assistant_priv *priv)
{
	const gchar *host = g_object_get_data(G_OBJECT(priv->router_stack), "server");
	const gchar *ftp_user = gtk_entry_get_text(GTK_ENTRY(priv->ftp_user));
	const gchar *ftp_password = gtk_entry_get_text(GTK_ENTRY(priv->ftp_password));
	gchar *message;

	/* Test ftp login */
	struct ftp *ftp = ftp_init(host);
	if (ftp) {
		if (!ftp_login(ftp, ftp_user, ftp_password)) {
			/* Error: Could not login to ftp */
			message = g_strdup(_("Error: Could not login to box ftp server\nPlease check your ftp user/password."));
			emit_message(0, message);
			ftp_shutdown(ftp);
			return FALSE;
		}
		ftp_shutdown(ftp);
	}

	return TRUE;
}

GtkWidget *finish_page(struct assistant_priv *priv)
{
	GtkWidget *grid;
	GtkWidget *welcome;
	GtkWidget *logo;
	GtkWidget *description;
	gchar *path = g_strconcat(get_directory(APP_DATA), G_DIR_SEPARATOR_S, "app.png", NULL);

	/* Welcome */
	grid = gtk_grid_new();

	welcome = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(welcome), _("<span font_desc=\"30.0\"><b>Finish!</b></span>"));
	gtk_widget_set_hexpand(welcome, TRUE);
	gtk_label_set_justify(GTK_LABEL(welcome), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), welcome, 0, 0, 1, 1);

	logo = gtk_image_new_from_icon_name("face-smile-big-symbolic", GTK_ICON_SIZE_DIALOG);

	gtk_image_set_pixel_size(GTK_IMAGE(logo), 128);
	gtk_grid_attach(GTK_GRID(grid), logo, 0, 1, 1, 1);

	description = gtk_label_new(_("If you like Roger Router please consider a donation.\n"));
	gtk_grid_attach(GTK_GRID(grid), description, 0, 2, 1, 1);

	gtk_widget_set_margin(description, 15, 10, 15, 10);
	g_free(path);

	return grid;
}

gboolean finish_pre(struct assistant_priv *priv)
{
	const gchar *ftp_user = gtk_entry_get_text(GTK_ENTRY(priv->ftp_user));
	const gchar *ftp_password = gtk_entry_get_text(GTK_ENTRY(priv->ftp_password));

	/* Enable telnet & capi port */
	if (router_dial_number(priv->profile, 1, "#96*5*")) {
		router_hangup(priv->profile, 1, "#96*5*");
	}
	if (router_dial_number(priv->profile, 1, "#96*3*")) {
		router_hangup(priv->profile, 1, "#96*3*");
	}

	g_settings_set_string(priv->profile->settings, "serial", priv->profile->router_info->serial);

	g_settings_set_string(priv->profile->settings, "ftp-user", ftp_user);
	password_manager_set_password(priv->profile, "ftp-password", ftp_password);

	/* Set initial fax report dir */
	g_settings_set_string(priv->profile->settings, "fax-report-dir", g_get_home_dir());

	/* Set initial softphone number */
	gchar **numbers = router_get_numbers(priv->profile);
	if (numbers && numbers[0]) {
		g_settings_set_string(priv->profile->settings, "phone-number", numbers[0]);
	}

	g_debug("Adding profile");
	profile_add(priv->profile);
	g_debug("Commit profile");
	profile_commit();
	g_debug("Reconnect monitor");
	net_monitor_reconnect();

	return FALSE;
}

struct assistant_page assistant_pages[] = {
	{welcome_page, welcome_pre, NULL, "welcome"},
	{profile_page, profile_pre, NULL, "profile"},
	{router_page, router_pre, router_post, "router"},
	{password_page, NULL, password_post, "password"},
	{ftp_password_page, ftp_password_pre, ftp_password_post, "ftp_password"},
	{finish_page, finish_pre, NULL, "finish"},
	{NULL, NULL, NULL, NULL}
};

void back_button_clicked_cb(GtkWidget *next, gpointer user_data)
{
	GtkWidget *stack = user_data;
	struct assistant_priv *priv = g_object_get_data(G_OBJECT(stack), "priv");

	if (priv->current_page <= 0) {
		return;
	}

	priv->current_page--;
	if (priv->current_page == priv->max_page - 2) {
		gtk_button_set_label(GTK_BUTTON(priv->next), _("Next"));
	}

	gtk_widget_set_sensitive(priv->next, TRUE);
	gtk_widget_set_sensitive(priv->back, TRUE);

	if (assistant_pages[priv->current_page].pre) {
		assistant_pages[priv->current_page].pre(priv);
	}

	gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT);
	gtk_stack_set_visible_child_name(GTK_STACK(stack), assistant_pages[priv->current_page].name);
}

void next_button_clicked_cb(GtkWidget *next, gpointer user_data)
{
	GtkWidget *stack = user_data;
	struct assistant_priv *priv = g_object_get_data(G_OBJECT(stack), "priv");

	if (assistant_pages[priv->current_page].post) {
		if (!assistant_pages[priv->current_page].post(priv)) {
			return;
		}
	}

	if (priv->current_page >= priv->max_page - 1) {
		gtk_widget_destroy(priv->assistant);
		return;
	}

	priv->current_page++;

	gtk_widget_set_sensitive(priv->next, TRUE);
	gtk_widget_set_sensitive(priv->back, TRUE);

	if (assistant_pages[priv->current_page].pre) {
		assistant_pages[priv->current_page].pre(priv);
	}

	gtk_widget_set_can_default(next, TRUE);
	gtk_widget_grab_default(next);
	gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT);
	gtk_stack_set_visible_child_name(GTK_STACK(stack), assistant_pages[priv->current_page].name);

	if (priv->current_page >= priv->max_page - 1) {
		gtk_button_set_label(GTK_BUTTON(priv->next), _("Done"));
	}
}

void assistant()
{
	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *back_button;
	GtkWidget *next_button;
	GtkWidget *stack;
	struct assistant_priv *priv = g_slice_alloc(sizeof(struct assistant_priv));
	gint i;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	priv->assistant = window;

	header = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), _("Setup Wizard"));
	gtk_window_set_titlebar(GTK_WINDOW(window), header);

	back_button = gtk_button_new_with_label(_("Back"));
	gtk_widget_set_sensitive(back_button, FALSE);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), back_button);
	priv->back = back_button;

	next_button = gtk_button_new_with_label(_("Next"));
	gtk_style_context_add_class(gtk_widget_get_style_context(next_button), GTK_STYLE_CLASS_SUGGESTED_ACTION);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), next_button);

	stack = gtk_stack_new();
	gtk_stack_set_homogeneous(GTK_STACK(stack), TRUE);
	g_object_set_data(G_OBJECT(stack), "priv", priv);
	gtk_widget_set_margin(stack, 18, 18, 18, 18);
	gtk_container_add(GTK_CONTAINER(window), stack);

	priv->next = next_button;
	gtk_widget_grab_focus(priv->next);

	for (i = 0; assistant_pages[i].create_child != NULL; i++) {
		GtkWidget *child = assistant_pages[i].create_child(priv);

		g_debug("Adding %s", assistant_pages[i].name);
		gtk_stack_add_named(GTK_STACK(stack), child, assistant_pages[i].name);
	}

	priv->current_page = 0;
	priv->max_page = i;

	g_signal_connect(back_button, "clicked", G_CALLBACK(back_button_clicked_cb), stack);
	g_signal_connect(next_button, "clicked", G_CALLBACK(next_button_clicked_cb), stack);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	//gtk_window_set_default_size(GTK_WINDOW(window), 300, 300);

	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_widget_show_all(window);
	gtk_stack_set_visible_child_name(GTK_STACK(stack), assistant_pages[0].name);
}

#endif
