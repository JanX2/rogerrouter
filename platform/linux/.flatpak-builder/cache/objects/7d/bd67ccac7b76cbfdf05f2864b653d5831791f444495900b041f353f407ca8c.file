/**
 * Roger Router
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#include <config.h>

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

/** Assistant structure, keeping track of internal widgets and states */
struct assistant {
	/* Internal states for child page handling */
	gint current_page;
	gint max_page;
	gint page;

	/* Assistant internal widgets */
	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *stack;
	GtkWidget *back_button;
	GtkWidget *next_button;

	/* Profile name entry */
	GtkWidget *profile_name;

	/* Internal router selection widgets */
	GtkWidget *server;
	GtkWidget *router_stack;

	/* Router credentials */
	GtkWidget *user;
	GtkWidget *password;

	/* FTP credentials */
	GtkWidget *ftp_user;
	GtkWidget *ftp_password;

	/** Router listbox for automatic discovered devices */
	GtkWidget *router_listbox;

	/** New assistant created profile */
	struct profile *profile;
};

/** Assistant page structure, holding name and pre/post functions */
struct assistant_page {
	gchar *name;
	void (*pre)(struct assistant *assistant);
	gboolean (*post)(struct assistant *assistant);
};

/** Assistant */
static struct assistant *assistant = NULL;

/**
 * \brief Pre welcome page
 * \param assistant assistant structure
 */
static void welcome_pre(struct assistant *assistant)
{
	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Welcome"));
}

/**
 * \brief Profile entry changed callback
 * \param entry entry widget
 * \param user_data UNUSED
 */
void profile_entry_changed(GtkEditable *entry, gpointer user_data)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
	GSList *profile_list = profile_get_list();
	struct profile *profile;

	/* Loop through all known profiles and check for duplicate profile name */
	while (profile_list != NULL) {
		profile = profile_list->data;
		g_assert(profile != NULL);

		if (!strcmp(profile->name, text)) {
			/* Duplicate found: Update button state and icon entry */
			gtk_widget_set_sensitive(assistant->next_button, FALSE);
			gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "dialog-error-symbolic");
			return;
		}

		profile_list = profile_list->next;
	}

	/* Update button state and icon entry */
	gtk_widget_set_sensitive(assistant->next_button, !EMPTY_STRING(text));
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, NULL);
}

/**
 * \brief Pre profile page
 * \param assistant assistant structure
 */
static void profile_pre(struct assistant *assistant)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(assistant->profile_name));

	/* Set button state */
	gtk_widget_set_sensitive(assistant->next_button, !EMPTY_STRING(text));

	/* Grab focus */
	gtk_widget_grab_focus(assistant->profile_name);

	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Create a profile"));
}

/**
 * \brief Post profile page
 * \param assistant assistant structure
 * \return TRUE to continue, FALSE on error
 */
static gboolean profile_post(struct assistant *assistant)
{
	const gchar *name = gtk_entry_get_text(GTK_ENTRY(assistant->profile_name));

	/* Create new profile based on user input */
	assistant->profile = profile_new(name);

	return TRUE;
}

/**
 * \brief Destroy listbox
 * \param widget listbox
 * \param user_data UNUSED
 */
static void router_listbox_destroy(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
}

/**
 * \brief Scan for routers using ssdp
 * \param user_data UNUSED
 * \return G_SOURCE_REMOVE
 */
static gboolean scan(gpointer user_data)
{
	GList *routers = ssdp_get_routers();
	GList *list;

	//return G_SOURCE_REMOVE;

	/* For all children of list box, call router_listbox_destroy() */
	gtk_container_foreach(GTK_CONTAINER(assistant->router_listbox), router_listbox_destroy, NULL);

	for (list = routers; list != NULL; list = list->next) {
		GtkWidget *new_device;
		struct router_info *router_info = list->data;
		gchar *tmp;

		tmp = g_strdup_printf("<b>%s</b>\n<small>on %s</small>", router_info->name, router_info->host);

		new_device = gtk_label_new("");
		gtk_label_set_justify(GTK_LABEL(new_device), GTK_JUSTIFY_CENTER);
		g_object_set_data(G_OBJECT(new_device), "host", router_info->host);
		gtk_label_set_markup(GTK_LABEL(new_device), tmp);
		g_free(tmp);
		gtk_widget_show(new_device);

		gtk_list_box_prepend(GTK_LIST_BOX(assistant->router_listbox), new_device);
	}

	if (routers) {
		/* Pre-select first row */
		GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(assistant->router_listbox), 0);
		gtk_list_box_select_row(GTK_LIST_BOX(assistant->router_listbox), row);
	}

	return G_SOURCE_REMOVE;
}

/**
 * \brief Router row activated callback
 * \param box listbox entry containing detected routers
 * \param row selected row
 * \param user_data UNUSED
 */
void router_listbox_row_activated_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	if (row != NULL) {
		/* We have a selected row, get child and set host internally */
		GtkWidget *child = gtk_container_get_children(GTK_CONTAINER(row))->data;
		gchar *host = g_object_get_data(G_OBJECT(child), "host");

		g_object_set_data(G_OBJECT(assistant->router_stack), "server", host);
	}

	gtk_widget_set_sensitive(assistant->next_button, row != NULL);
}

/**
 * \brief Router row selected callback
 * \param box listbox entry containing detected routers
 * \param row selected row
 * \param user_data UNUSED
 */
void router_row_selected_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	router_listbox_row_activated_cb(box, row, user_data);
}

/**
 * \brief Router entry changed callback
 * \param entry entry widget
 * \param user_data UNUSED
 */
void router_entry_changed(GtkEditable *entry, gpointer user_data)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(assistant->server));
	gboolean valid;

	/* Check for valid ip entry */
	valid = g_hostname_is_ip_address(text);

	/* Update next button state */
	gtk_widget_set_sensitive(assistant->next_button, valid);
}

/**
 * \brief React on router stack changes and update button state
 * \param entry entry widget
 * \param user_data UNUSED
 */
static void router_stack_changed_cb(GtkEditable *entry, gpointer user_data)
{
	const gchar *name = gtk_stack_get_visible_child_name(GTK_STACK(assistant->router_stack));

	if (!name) {
		return;
	}

	/* Check active page */
	if (!strcmp(name, "manual")) {
		/* Call router ip entry changed callback to check for a valid ip */
		router_entry_changed(NULL, NULL);
	} else {
		GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(assistant->router_listbox), 0);

		/* Update next button state */
		gtk_widget_set_sensitive(assistant->next_button, FALSE);

		/* Reselect active row */
#if GTK_CHECK_VERSION(3,12,0)
		gtk_list_box_unselect_all(GTK_LIST_BOX(assistant->router_listbox));
#endif
		gtk_list_box_select_row(GTK_LIST_BOX(assistant->router_listbox), row);
	}
}

/**
 * \brief Pre router page
 * \param assistant assistant structure
 */
static void router_pre(struct assistant *assistant)
{
	/* Start scan for routers */
	g_idle_add(scan, NULL);

	/* Update next button state */
	gtk_widget_set_sensitive(assistant->next_button, FALSE);

	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Select router"));
}

/**
 * \brief Post router page
 * \param assistant assistant structure
 * \return TRUE to continue, FALSE on error
 */
static gboolean router_post(struct assistant *assistant)
{
	const gchar *server;
	gboolean present;

	/* Get user selected server */
	g_debug("%s(): child %s", __FUNCTION__, gtk_stack_get_visible_child_name(GTK_STACK(assistant->router_stack)));
	if (!strcmp(gtk_stack_get_visible_child_name(GTK_STACK(assistant->router_stack)), "manual")) {
		server = gtk_entry_get_text(GTK_ENTRY(assistant->server));
		g_object_set_data(G_OBJECT(assistant->router_stack), "server", (gchar*) server);
	} else {
		server = g_object_get_data(G_OBJECT(assistant->router_stack), "server");
	}
	g_debug("%s(): server %s", __FUNCTION__, server);

	profile_set_host(assistant->profile, server);

	/* Check if router is present */
	present = router_present(assistant->profile->router_info);
	if (!present) {
		emit_message(_("Login failed"), _("Please change the router ip and try again"));
	}

	return present;
}

/**
 * \brief Pre password page
 * \param assistant assistant structure
 */
static void password_pre(struct assistant *assistant)
{
	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Router password"));
}

/**
 * \brief Post password page
 * \param assistant assistant structure
 * \return TRUE to continue, FALSE on error
 */
static gboolean password_post(struct assistant *assistant)
{
	const gchar *user = gtk_entry_get_text(GTK_ENTRY(assistant->user));
	const gchar *password = gtk_entry_get_text(GTK_ENTRY(assistant->password));
	gchar **numbers;

	/* Create new profile based on user input */
	profile_set_login_user(assistant->profile, user);
	profile_set_login_password(assistant->profile, password);

	/* Release any previous lock */
	router_release_lock();

	/* Get settings */
	if (!router_login(assistant->profile) || !router_get_settings(assistant->profile)) {
		return FALSE;
	}

	/* Store router serial number for detection purpose */
	g_settings_set_string(assistant->profile->settings, "serial", assistant->profile->router_info->serial);
	/* Set initial fax report dir */
	g_settings_set_string(assistant->profile->settings, "fax-report-dir", g_get_home_dir());
	/* Set initial softphone number */
	numbers = router_get_numbers(assistant->profile);
	if (numbers && numbers[0]) {
		g_settings_set_string(assistant->profile->settings, "phone-number", numbers[0]);
	}

	return TRUE;
}

/**
 * \brief Pre ftp password page
 * \param assistant assistant structure
 */
static void ftp_password_pre(struct assistant *assistant)
{
	const gchar *user = gtk_entry_get_text(GTK_ENTRY(assistant->user));
	const gchar *password = gtk_entry_get_text(GTK_ENTRY(assistant->password));

	/* Set credentials from router to FTP for simplicity */
	gtk_entry_set_text(GTK_ENTRY(assistant->ftp_user), EMPTY_STRING(user) ? "ftpuser" : user);
	gtk_entry_set_text(GTK_ENTRY(assistant->ftp_password), password);

	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("FTP password"));
}

/**
 * \brief Post ftp password page
 * \param assistant assistant structure
 * \return TRUE to continue, FALSE on error
 */
static gboolean ftp_password_post(struct assistant *assistant)
{
	const gchar *host = g_object_get_data(G_OBJECT(assistant->router_stack), "server");
	const gchar *ftp_user = gtk_entry_get_text(GTK_ENTRY(assistant->ftp_user));
	const gchar *ftp_password = gtk_entry_get_text(GTK_ENTRY(assistant->ftp_password));
	gchar *message;
	struct ftp *ftp;

	/* Test ftp login */
	ftp = ftp_init(host);
	if (ftp) {
		if (!ftp_login(ftp, ftp_user, ftp_password)) {
			/* Error: Could not login to ftp */
			message = g_strdup(_("Please check your ftp user/password."));
			emit_message(_("Login failed"), message);
			ftp_shutdown(ftp);
			return FALSE;
		}
		ftp_shutdown(ftp);

		/* Store FTP credentials */
		g_settings_set_string(assistant->profile->settings, "ftp-user", ftp_user);
		password_manager_set_password(assistant->profile, "ftp-password", ftp_password);
	}

	return TRUE;
}

/**
 * \brief Pre finish page
 */
static void finish_pre(struct assistant *assistant)
{
	/* Set title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Ready to go"));
}

/**
 * \brief Post finish page
 * Add new profile to list, trigger a reconnect of network monitor and set journal visible
 * \param assistant assistant private structure
 * \return TRUE
 */
static gboolean finish_post(struct assistant *assistant)
{
	/* Enable telnet & capi port */
	if (router_dial_number(assistant->profile, ROUTER_DIAL_PORT_AUTO, ROUTER_ENABLE_TELNET)) {
		router_hangup(assistant->profile, ROUTER_DIAL_PORT_AUTO, ROUTER_ENABLE_TELNET);
	}
	if (router_dial_number(assistant->profile, ROUTER_DIAL_PORT_AUTO, ROUTER_ENABLE_CAPI)) {
		router_hangup(assistant->profile, ROUTER_DIAL_PORT_AUTO, ROUTER_ENABLE_CAPI);
	}

	/* Add new profile and commit it */
	profile_add(assistant->profile);
	profile_commit();

	/* Trigger network reconnect */
	net_monitor_reconnect();

	/* Set journal visible if needed */
	journal_set_visible(TRUE);

	return TRUE;
}

/** Assistant pages: Name, Pre/Post functions */
struct assistant_page assistant_pages[] = {
	{"welcome", welcome_pre, NULL},
	{"profile", profile_pre, profile_post},
	{"router", router_pre, router_post},
	{"password", password_pre, password_post},
	{"ftp_password", ftp_password_pre, ftp_password_post},
	{"finish", finish_pre, finish_post},
	{NULL, NULL, NULL}
};

/**
 * \brief Control function for next button clicked
 * Responsible for top buttons and pre and post states of child pages
 * \param next next button widget
 * \param user_data UNUSED
 */
void back_button_clicked_cb(GtkWidget *next, gpointer user_data)
{
	g_debug("%s(): current page %d", __FUNCTION__, assistant->current_page);
	/* In case we are on the first page, exit assistant */
	if (assistant->current_page <= 0) {
		struct profile *profile = profile_get_active();

		gtk_widget_destroy(assistant->window);

		g_slice_free(struct assistant, assistant);
		assistant = NULL;

		/* In case no profile is active and assistant is aborted, exit application */
		if (!profile) {
			app_quit();
		}

		return;
	}

	/* Decrease current page counter */
	assistant->current_page--;

	/* If we have two next pages, ensure that next button label is correct */
	if (assistant->current_page == assistant->max_page - 2) {
		gtk_button_set_label(GTK_BUTTON(assistant->next_button), _("Next"));
	}

	/* If we have no previous page, change back button text to Quit */
	if (assistant->current_page == 0) {
		gtk_button_set_label(GTK_BUTTON(assistant->back_button), _("Quit"));
	}

	/* Ensure that all buttons are sensitive */
	gtk_widget_set_sensitive(assistant->next_button, TRUE);
	gtk_widget_set_sensitive(assistant->back_button, TRUE);

	/* Run pre page function if necessary */
	if (assistant_pages[assistant->current_page].pre) {
		assistant_pages[assistant->current_page].pre(assistant);
	}

	/* Set transition type back to slide *right* and set active child */
	gtk_stack_set_transition_type(GTK_STACK(assistant->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT);
	gtk_stack_set_visible_child_name(GTK_STACK(assistant->stack), assistant_pages[assistant->current_page].name);
}

/**
 * \brief Control function for next button clicked
 * Responsible for top buttons and pre and post states of child pages
 * \param next next button widget
 * \param user_data UNUSED
 */
void next_button_clicked_cb(GtkWidget *next, gpointer user_data)
{
	g_debug("%s(): current page %d", __FUNCTION__, assistant->current_page);
	gtk_button_set_label(GTK_BUTTON(assistant->back_button), _("Back"));

	/* In case we have a post page function and it returns FALSE, exit immediately */
	if (assistant_pages[assistant->current_page].post && !assistant_pages[assistant->current_page].post(assistant)) {
		return;
	}

	/* Check if we have reached the end */
	if (assistant->current_page >= assistant->max_page - 1) {
		gtk_widget_destroy(assistant->window);
		g_slice_free(struct assistant, assistant);
		assistant = NULL;
		return;
	}

	/* Increate current page counter */
	assistant->current_page++;

	/* Ensure that all buttons are sensitive */
	gtk_widget_set_sensitive(assistant->next_button, TRUE);
	gtk_widget_set_sensitive(assistant->back_button, TRUE);

	/* Run pre page function if necessary */
	if (assistant_pages[assistant->current_page].pre) {
		assistant_pages[assistant->current_page].pre(assistant);
	}

	/* Set transition type back to slide *left* and set active child */
	gtk_stack_set_transition_type(GTK_STACK(assistant->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT);
	gtk_stack_set_visible_child_name(GTK_STACK(assistant->stack), assistant_pages[assistant->current_page].name);

	/* In case it is that last page, change next button label */
	if (assistant->current_page >= assistant->max_page - 1) {
		gtk_button_set_label(GTK_BUTTON(assistant->next_button), _("Done"));
	}
}

/**
 * \brief Create router setup assistant
 */
void app_assistant(void)
{
	GtkBuilder *builder;
	GtkWidget *placeholder;
	GList *childrens;
	gint max = 0;

	/* Only open assitant once at a time */
	if (assistant) {
		return;
	}

	builder = gtk_builder_new_from_resource("/org/tabos/roger/assistant.glade");
	if (!builder) {
		g_warning("Could not load assistant ui");
		return;
	}

	/* Allocate private structure */
	assistant = g_slice_alloc(sizeof(struct assistant));

	/* Connect to builder objects */
	assistant->window = GTK_WIDGET(gtk_builder_get_object(builder, "assistant"));
	assistant->header = GTK_WIDGET(gtk_builder_get_object(builder, "headerbar"));

	assistant->stack = GTK_WIDGET(gtk_builder_get_object(builder, "stack"));
	assistant->back_button = GTK_WIDGET(gtk_builder_get_object(builder, "back_button"));
	assistant->next_button = GTK_WIDGET(gtk_builder_get_object(builder, "next_button"));

	assistant->profile_name = GTK_WIDGET(gtk_builder_get_object(builder, "profile_name_entry"));

	assistant->router_stack = GTK_WIDGET(gtk_builder_get_object(builder, "router_stack"));
	g_signal_connect(assistant->router_stack, "notify::visible-child", G_CALLBACK(router_stack_changed_cb), NULL);

	assistant->router_listbox = GTK_WIDGET(gtk_builder_get_object(builder, "router_listbox"));
	placeholder = gtk_label_new(_("No router detected"));
	gtk_widget_show(placeholder);
	gtk_list_box_set_placeholder(GTK_LIST_BOX(assistant->router_listbox), placeholder);

	assistant->server = GTK_WIDGET(gtk_builder_get_object(builder, "ip_address_entry"));
	assistant->user = GTK_WIDGET(gtk_builder_get_object(builder, "user_entry"));
	assistant->password = GTK_WIDGET(gtk_builder_get_object(builder, "password_entry"));

	assistant->ftp_user = GTK_WIDGET(gtk_builder_get_object(builder, "ftp_user_entry"));
	assistant->ftp_password = GTK_WIDGET(gtk_builder_get_object(builder, "ftp_password_entry"));

	ui_set_suggested_style(assistant->next_button);

	/* Set internal start point & limit */
	childrens = gtk_container_get_children(GTK_CONTAINER(assistant->stack));
	assistant->current_page = 0;
	if (childrens) {
		max = g_list_length(childrens);
	}
	assistant->max_page = max;

	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Welcome"));
	gtk_button_set_label(GTK_BUTTON(assistant->back_button), _("Quit"));

	/* Show window */
	gtk_widget_show_all(assistant->window);
}
