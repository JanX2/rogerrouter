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
#include <libroutermanager/router.h>
#include <libroutermanager/ftp.h>
#include <libroutermanager/password.h>

#include <roger/main.h>
#include <roger/phone.h>
#include <roger/pref.h>
#include <roger/pref_router.h>

static GtkWidget *ftp_password_entry;

/**
 * \brief Toggle password visiblity
 * \param togglebutton toggle button
 * \param user_data router password widget
 */
static void show_password_cb(GtkToggleButton *togglebutton, gpointer user_data)
{
	gtk_entry_set_visibility(GTK_ENTRY(user_data), gtk_toggle_button_get_active(togglebutton));
	gtk_entry_set_visibility(GTK_ENTRY(ftp_password_entry), gtk_toggle_button_get_active(togglebutton));
}

/**
 * \brief Verify that router password is valid
 * \param button verify button widget
 * \param user_data user data pointer (NULL)
 */
static void verify_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog;
	struct profile *profile = profile_get_active();

	router_logout(profile);

	if (router_login(profile) == TRUE) {
		dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, _("Login password is valid"));
	} else {
		dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Login password is invalid"));
	}

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/**
 * \brief Verify that ftp password is valid
 * \param button verify button widget
 * \param user_data user data pointer (NULL)
 */
static void verify_ftp_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog;
	struct ftp *client;
	struct profile *profile = profile_get_active();

	client = ftp_init(router_get_host(profile));
	if (!client) {
		dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not connect to FTP. Missing USB storage?"));
	} else {
		if (ftp_login(client, router_get_ftp_user(profile), router_get_ftp_password(profile)) == TRUE) {
			dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, _("FTP password is valid"));
		} else {
			dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("FTP password is invalid"));
		}
	}

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/**
 * \brief Update preferences button clicked callback
 * \param button update button widget
 * \param user_data preference window widget
 */
static void update_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *update_dialog = gtk_message_dialog_new(user_data, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, _("Do you want to reload the router settings and replace your preferences?"));
	gtk_window_set_title(GTK_WINDOW(update_dialog), _("Update settings"));
	gtk_window_set_position(GTK_WINDOW(update_dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gint result = gtk_dialog_run(GTK_DIALOG(update_dialog));
	gtk_widget_destroy(update_dialog);

	if (result != GTK_RESPONSE_OK) {
		return;
	}

	router_get_settings(profile_get_active());
}

/**
 * \brief Router login password changed callback - set password in password manager
 * \param entry password entry widget
 * \param user_data profile pointer
 */
static void router_login_password_changed_cb(GtkEntry *entry, gpointer user_data)
{
	struct profile *profile = user_data;

	password_manager_set_password(profile, "login-password", gtk_entry_get_text(GTK_ENTRY(entry)));
}

/**
 * \brief FTP login password changed callback - set password in password manager
 * \param entry password entry widget
 * \param user_data profile pointer
 */
static void router_ftp_password_changed_cb(GtkEntry *entry, gpointer user_data)
{
	struct profile *profile = user_data;

	password_manager_set_password(profile, "ftp-password", gtk_entry_get_text(GTK_ENTRY(entry)));
}

/**
 * \brief Create router preferences settings widget
 * \return router settings widget
 */
static GtkWidget *pref_page_router_settings(void)
{
	GtkWidget *group;
	GtkWidget *host_label;
	GtkWidget *host_entry;
	GtkWidget *login_user_label;
	GtkWidget *login_user_entry;
	GtkWidget *login_password_label;
	GtkWidget *login_password_entry;
	GtkWidget *ftp_user_label;
	GtkWidget *ftp_user_entry;
	GtkWidget *ftp_password_label;
	GtkWidget *verify_button;
	GtkWidget *show_password_toggle;
	struct profile *profile = profile_get_active();
	gint line = 0;

	/**
	 * Group settings:
	 * Host:     <INPUT>
	 * Login user: <INPUT>
	 * Login password: <INPUT> Verify-Button
	 * FTP user: <INPUT>
	 * FTP password: <INPUT> Verify-Button
	 * [ ] Show password
	 */
	group = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(group), 5);
	gtk_grid_set_column_spacing(GTK_GRID(group), 15);

	/* Host */
	host_label = ui_label_new(_("Host"));
	gtk_grid_attach(GTK_GRID(group), host_label, 0, line, 1, 1);

	host_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(host_entry, _("Host name, e.g. fritz.box or speedport.ip"));
	gtk_widget_set_hexpand(host_entry, TRUE);
	gtk_grid_attach(GTK_GRID(group), host_entry, 1, line, 1, 1);
	g_settings_bind(profile_get_active()->settings, "host", host_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/* Login user */
	line++;
	login_user_label = ui_label_new(_("Login user"));
	gtk_grid_attach(GTK_GRID(group), login_user_label, 0, line, 1, 1);

	login_user_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(login_user_entry, _("Router login user"));
	gtk_widget_set_hexpand(login_user_entry, TRUE);
	gtk_grid_attach(GTK_GRID(group), login_user_entry, 1, line, 1, 1);
	g_settings_bind(profile_get_active()->settings, "login-user", login_user_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/* Login password */
	line++;
	login_password_label = ui_label_new(_("Login password"));
	gtk_grid_attach(GTK_GRID(group), login_password_label, 0, line, 1, 1);

	login_password_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(login_password_entry, _("Router login password"));
	gtk_entry_set_visibility(GTK_ENTRY(login_password_entry), FALSE);
	gtk_widget_set_hexpand(login_password_entry, TRUE);
	gtk_grid_attach(GTK_GRID(group), login_password_entry, 1, line, 1, 1);
	gtk_entry_set_text(GTK_ENTRY(login_password_entry), router_get_login_password(profile));
	g_signal_connect(login_password_entry, "changed", G_CALLBACK(router_login_password_changed_cb), profile);
	//g_settings_bind(profile_get_active()->settings, "login-password", login_password_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/* Verify button */
	verify_button = gtk_button_new_with_label(_("Verify"));
	gtk_widget_set_tooltip_text(verify_button, _("Check if password is valid"));
	g_signal_connect(verify_button, "clicked", G_CALLBACK(verify_button_clicked_cb), pref_get_window());
	gtk_grid_attach(GTK_GRID(group), verify_button, 2, line, 1, 1);

	/* FTP user */
	line++;
	ftp_user_label = ui_label_new(_("FTP user"));
	gtk_grid_attach(GTK_GRID(group), ftp_user_label, 0, line, 1, 1);

	ftp_user_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(ftp_user_entry, _("FTP user"));
	gtk_widget_set_hexpand(ftp_user_entry, TRUE);
	gtk_grid_attach(GTK_GRID(group), ftp_user_entry, 1, line, 1, 1);
	g_settings_bind(profile_get_active()->settings, "ftp-user", ftp_user_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/* FTP-Password */
	line++;
	ftp_password_label = ui_label_new(_("FTP password"));
	gtk_grid_attach(GTK_GRID(group), ftp_password_label, 0, line, 1, 1);

	ftp_password_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(ftp_password_entry, _("Router FTP password"));
	gtk_entry_set_visibility(GTK_ENTRY(ftp_password_entry), FALSE);
	gtk_widget_set_hexpand(ftp_password_entry, TRUE);
	gtk_grid_attach(GTK_GRID(group), ftp_password_entry, 1, line, 1, 1);
	gtk_entry_set_text(GTK_ENTRY(ftp_password_entry), router_get_ftp_password(profile));
	g_signal_connect(ftp_password_entry, "changed", G_CALLBACK(router_ftp_password_changed_cb), profile);
	//g_settings_bind(profile_get_active()->settings, "ftp-password", ftp_password_entry, "text", G_SETTINGS_BIND_DEFAULT);

	/* Verify button */
	verify_button = gtk_button_new_with_label(_("Verify"));
	gtk_widget_set_tooltip_text(verify_button, _("Check if password is valid"));
	g_signal_connect(verify_button, "clicked", G_CALLBACK(verify_ftp_button_clicked_cb), pref_get_window());
	gtk_grid_attach(GTK_GRID(group), verify_button, 2, line, 1, 1);

	/* Show passwords */
	line++;
	show_password_toggle = gtk_check_button_new_with_label(_("Show passwords"));
	GdkRGBA col;
	gdk_rgba_parse(&col, "#808080");
	gtk_widget_override_color(show_password_toggle, GTK_STATE_FLAG_NORMAL, &col);

	gtk_widget_set_tooltip_text(show_password_toggle, _("Toggle passwords visibility"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_password_toggle), FALSE);
	g_signal_connect(show_password_toggle, "toggled", G_CALLBACK(show_password_cb), login_password_entry);
	gtk_grid_attach(GTK_GRID(group), show_password_toggle, 1, line, 1, 1);

	gtk_widget_set_hexpand(group, TRUE);

	return pref_group_create(group, _("Settings"), TRUE, FALSE);
}

/**
 * \brief Create router information widget
 * \return router information widget
 */
static GtkWidget *pref_page_router_information(void)
{
	GtkWidget *group;
	GtkWidget *space;
	GtkWidget *box_label;
	GtkWidget *update_button;
	const gchar *box_name;
	const gchar *firmware_name;
	gchar *box_string;

	/**
	 * Group settings:
	 * <REFRESH-BUTTON> ROUTER_NAME (FIRMWARE)
	 */
	group = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(group), 5);
	gtk_grid_set_column_spacing(GTK_GRID(group), 5);

	space = gtk_label_new("");
	gtk_widget_set_vexpand(space, TRUE);
	gtk_grid_attach(GTK_GRID(group), space, 0, 0, 1, 1);

	/* Update button */
	update_button = gtk_button_new_with_mnemonic(_("_Refresh"));
	gtk_widget_set_tooltip_text(update_button, _("Refresh settings"));
	g_signal_connect(update_button, "clicked", G_CALLBACK(update_button_clicked_cb), pref_get_window());
	gtk_grid_attach(GTK_GRID(group), update_button, 0, 1, 1, 1);

	/* Box Information */
	box_name = router_get_name(profile_get_active());
	firmware_name = router_get_version(profile_get_active());
	box_string = g_strdup_printf(_("%s (Firmware: %s)"), box_name, firmware_name);
	box_label = ui_label_new(box_string);
	gtk_misc_set_alignment(GTK_MISC(box_label), 0.5, 0.5);
	gtk_widget_set_hexpand(box_label, TRUE);
	g_free(box_string);
	gtk_grid_attach(GTK_GRID(group), box_label, 1, 1, 1, 1);

	return pref_group_create(group, "", TRUE, FALSE);
}

/**
 * \brief Create router preferences widget
 * \return router widget
 */
GtkWidget *pref_page_router(void)
{
	GtkWidget *grid;
	GtkWidget *group;

	grid = gtk_grid_new();

	group = pref_page_router_settings();
	gtk_grid_attach(GTK_GRID(grid), group, 0, 0, 1, 1);

	group = pref_page_router_information();
	gtk_grid_attach(GTK_GRID(grid), group, 0, 2, 1, 1);

	return grid;
}
