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

#include <libroutermanager/router.h>
#include <libroutermanager/password.h>
#include <libroutermanager/ftp.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/net_monitor.h>
#include <libroutermanager/ssdp.h>

#include <roger/main.h>
#include <roger/journal.h>
#include <roger/application.h>
#include <roger/uitools.h>

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
	GList *routers = ssdp_get_routers();
	GList *list;

	/* For all children of list box, call router_listbox_destroy() */
	gtk_container_foreach(GTK_CONTAINER(listbox), router_listbox_destroy, NULL);

	for (list = routers; list != NULL; list = list->next) {
		GtkWidget *new_device;
		struct router_info *router_info = list->data;
		gchar *tmp;

		tmp = g_strdup_printf("<b>%s</b>\n<small>on %s</small>", router_info->name, router_info->host);

		new_device = gtk_label_new("");
		g_object_set_data(G_OBJECT(new_device), "host", router_info->host);
		gtk_label_set_markup(GTK_LABEL(new_device), tmp);
		gtk_widget_show(new_device);

		gtk_list_box_prepend(GTK_LIST_BOX(listbox), new_device);
	}

	return G_SOURCE_REMOVE;
}

static GtkWidget *router_listbox;

void router_row_selected_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	struct assistant_priv *priv = user_data;
	GtkWidget *child;

	if (row != NULL) {
		child = gtk_container_get_children(GTK_CONTAINER(row))->data;
		gchar *host = g_object_get_data(G_OBJECT(child), "host");
		g_object_set_data(G_OBJECT(priv->router_stack), "server", host);
	}

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
	gtk_widget_set_sensitive(priv->next, FALSE);
	g_idle_add(scan, router_listbox);

	return FALSE;
}

gboolean router_post(struct assistant_priv *priv)
{
	struct router_info router_info;
	const gchar *server;

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

	profile_add(priv->profile);
	profile_commit();
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
		gtk_widget_destroy(priv->assistant);
		return;
	}

	priv->current_page--;
	if (priv->current_page == priv->max_page - 2) {
		gtk_button_set_label(GTK_BUTTON(priv->next), _("Next"));
	}

	if (priv->current_page == 0) {
		gtk_button_set_label(GTK_BUTTON(priv->back), _("Quit"));
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

	gtk_button_set_label(GTK_BUTTON(priv->back), _("Back"));

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
	gtk_application_add_window(roger_app, GTK_WINDOW(window));
	priv->assistant = window;

	header = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), _("Setup Wizard"));
	gtk_window_set_titlebar(GTK_WINDOW(window), header);

	back_button = gtk_button_new_with_label(_("Quit"));
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), back_button);
	priv->back = back_button;

	next_button = gtk_button_new_with_label(_("Next"));
	ui_set_suggested_style(next_button);
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

		gtk_stack_add_named(GTK_STACK(stack), child, assistant_pages[i].name);
	}

	priv->current_page = 0;
	priv->max_page = i;

	g_signal_connect(back_button, "clicked", G_CALLBACK(back_button_clicked_cb), stack);
	g_signal_connect(next_button, "clicked", G_CALLBACK(next_button_clicked_cb), stack);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_widget_show_all(window);
	gtk_stack_set_visible_child_name(GTK_STACK(stack), assistant_pages[0].name);
}
