/*
 * Roger Router
 * Copyright (c) 2012-2017 Jan-Michael Brummer
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

#include <rm/rm.h>

#include <roger/main.h>
#include <roger/journal.h>
#include <roger/application.h>
#include <roger/uitools.h>

/** Assistant structure, keeping track of internal widgets and states */
typedef struct {
	/*< private >*/
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
	RmProfile *profile;
} Assistant;

/** Assistant page structure, holding name and pre/post functions */
typedef struct assistant_page {
	/*< private >*/
	gchar *name;
	void (*pre)(Assistant *assistant);
	gboolean (*post)(Assistant *assistant);
} AssistantPage;

/**
 * welcome_pre:
 * @assistant: a #Assistant
 *
 * Pre welcome page
 */
static void assistant_welcome_pre(Assistant *assistant)
{
	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Welcome"));
}

/**
 * assistant_profile_entry_changed:
 * @entry: @aGtkEditable
 * @user_data: a #Assistant
 *
 * Profile entry changed callback
 */
static void assistant_profile_entry_changed(GtkEditable *entry, gpointer user_data)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
	GSList *profile_list = rm_profile_get_list();
	RmProfile *profile;
	Assistant *assistant = user_data;

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
	gtk_widget_set_sensitive(assistant->next_button, !RM_EMPTY_STRING(text));
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, NULL);
}

/**
 * assistant_profile_pre:
 * @assistant: a #Assistant
 *
 * Pre profile page
 */
static void assistant_profile_pre(Assistant *assistant)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(assistant->profile_name));

	/* Set button state */
	gtk_widget_set_sensitive(assistant->next_button, !RM_EMPTY_STRING(text));

	/* Grab focus */
	gtk_widget_grab_focus(assistant->profile_name);

	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Create a profile"));
}

/**
 * assistant_profile_post:
 * @assistant: a #Assistant
 *
 * Post profile page
 *
 * Returns: %TRUE to continue, %FALSE on error
 */
static gboolean assistant_profile_post(Assistant *assistant)
{
	const gchar *name = gtk_entry_get_text(GTK_ENTRY(assistant->profile_name));

	/* Create new profile based on user input */
	assistant->profile = rm_profile_add(name);

	return TRUE;
}

/**
 * assistant_router_listbox_destroy:
 * @widget: a #GtkWidget
 * @user_data: UNUSED
 *
 * Destroy listbox
 */
static void assistant_router_listbox_destroy(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
}

/**
 * assistant_scan_router:
 * @user_data: a #Assistant
 *
 * Scan for routers using ssdp
 *
 * Returns: #G_SOURCE_REMOVE
 */
static gboolean assistant_scan_router(gpointer user_data)
{
	GList *routers = rm_ssdp_get_routers();
	GList *list;
	Assistant *assistant = user_data;

	/* For all children of list box, call router_listbox_destroy() */
	gtk_container_foreach(GTK_CONTAINER(assistant->router_listbox), assistant_router_listbox_destroy, NULL);

	for (list = routers; list != NULL; list = list->next) {
		GtkWidget *new_device;
		RmRouterInfo *router_info = list->data;
		gchar *tmp;

		tmp = g_strdup_printf("<b>%s</b>\n<small>%s %s</small>", router_info->name, _("on"), router_info->host);

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
 * assistant_router_listbox_row_activated_cb:
 * @box: a äGtkListBox
 * @row: a #GtkListBoxRow
 * @user_data: a #Assistant
 *
 * Router row activated callback
 */
static void assistant_router_listbox_row_activated_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	Assistant *assistant = user_data;

	if (row != NULL) {
		/* We have a selected row, get child and set host internally */
		GtkWidget *child = gtk_container_get_children(GTK_CONTAINER(row))->data;
		gchar *host = g_object_get_data(G_OBJECT(child), "host");

		g_object_set_data(G_OBJECT(assistant->router_stack), "server", host);
	}

	gtk_widget_set_sensitive(assistant->next_button, row != NULL);
}

/**
 * assistant_router_listbox_row_selected_cb:
 * @box: a #GtkListBox
 * @row: selected #GtkListBoxRow
 * @user_data: UNUSED
 *
 * Router row selected callback
 */
static void assistant_router_listbox_row_selected_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	assistant_router_listbox_row_activated_cb(box, row, user_data);
}

/**
 * assistant_router_entry_changed:
 * @entry: a #GtkEditable
 * @user_data: a #Assistant
 *
 * Router entry changed callback
 */
void assistant_router_entry_changed(GtkEditable *entry, gpointer user_data)
{
	Assistant *assistant = user_data;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(assistant->server));
	gboolean valid;

	/* Check for valid ip entry */
	valid = g_hostname_is_ip_address(text);

	/* Update next button state */
	gtk_widget_set_sensitive(assistant->next_button, valid);
}

/**
 * assistant_router_stack_changed_cb:
 * @entry: a #GtkEditable
 * @user_data: a #Assistant
 *
 * React on router stack changes and update button state
 */
static void assistant_router_stack_changed_cb(GtkEditable *entry, gpointer user_data)
{
	Assistant *assistant = user_data;
	const gchar *name = gtk_stack_get_visible_child_name(GTK_STACK(assistant->router_stack));

	if (!name) {
		return;
	}

	/* Check active page */
	if (!strcmp(name, "manual")) {
		/* Call router ip entry changed callback to check for a valid ip */
		assistant_router_entry_changed(NULL, assistant);
	} else {
		GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(assistant->router_listbox), 0);

		/* Update next button state */
		gtk_widget_set_sensitive(assistant->next_button, FALSE);

		/* Reselect active row */
		gtk_list_box_unselect_all(GTK_LIST_BOX(assistant->router_listbox));
		gtk_list_box_select_row(GTK_LIST_BOX(assistant->router_listbox), row);
	}
}

/**
 * assistant_router_pre:
 * @assistant: a #Assistant
 *
 * Pre router page
 */
static void assistant_router_pre(Assistant *assistant)
{
	/* Start scan for routers */
	g_idle_add(assistant_scan_router, assistant);

	/* Update next button state */
	gtk_widget_set_sensitive(assistant->next_button, FALSE);

	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Select router"));
}

/**
 * assistant_router_post:
 * @assistant: a #Assistant
 *
 * Post router page
 *
 * Returns: %TRUE to continue, %FALSE on error
 */
static gboolean assistant_router_post(Assistant *assistant)
{
	const gchar *server;
	gboolean present;

	/* Get user selected server */
	if (!strcmp(gtk_stack_get_visible_child_name(GTK_STACK(assistant->router_stack)), "manual")) {
		server = gtk_entry_get_text(GTK_ENTRY(assistant->server));
		g_object_set_data(G_OBJECT(assistant->router_stack), "server", (gchar*)server);
	} else {
		server = g_object_get_data(G_OBJECT(assistant->router_stack), "server");
	}

	rm_profile_set_host(assistant->profile, server);

	/* Check if router is present */
	present = rm_router_present(assistant->profile->router_info);
	if (!present) {
		rm_object_emit_message(_("Login failed"), _("Please change the router ip and try again"));
	}

	return present;
}

/**
 * assistant_password_pre:
 * @assistant: a #Assistant
 *
 * Pre password page
 */
static void assistant_password_pre(Assistant *assistant)
{
	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Router password"));
}

/**
 * assistant_password_post:
 * @assistant: a #Assistant
 *
 * Post password page
 *
 * Returns: %TRUE to continue, %FALSE on error
 */
static gboolean assistant_password_post(Assistant *assistant)
{
	const gchar *user = gtk_entry_get_text(GTK_ENTRY(assistant->user));
	const gchar *password = gtk_entry_get_text(GTK_ENTRY(assistant->password));

	/* Create new profile based on user input */
	rm_profile_set_login_user(assistant->profile, user);
	rm_profile_set_login_password(assistant->profile, password);

	/* Release any previous lock */
	rm_router_release_lock();

	rm_router_set_active(assistant->profile);

	/* Get settings */
	if (!rm_router_login(assistant->profile) || !rm_router_get_settings(assistant->profile)) {
		return FALSE;
	}

	return TRUE;
}

/**
 * assistant_ftp_password_pre:
 * @assistant: a #Assistant
 *
 * Pre ftp password page
 */
static void assistant_ftp_password_pre(Assistant *assistant)
{
	const gchar *user = gtk_entry_get_text(GTK_ENTRY(assistant->user));
	const gchar *password = gtk_entry_get_text(GTK_ENTRY(assistant->password));

	/* Set credentials from router to FTP for simplicity */
	gtk_entry_set_text(GTK_ENTRY(assistant->ftp_user), RM_EMPTY_STRING(user) ? "ftpuser" : user);
	gtk_entry_set_text(GTK_ENTRY(assistant->ftp_password), password);

	/* Update title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("FTP password"));
}

/**
 * assistant_ftp_password_post:
 * @assistant: a #Assistant
 *
 * Post ftp password page
 *
 * Returns: %TRUE to continue, %FALSE on error
 */
static gboolean assistant_ftp_password_post(Assistant *assistant)
{
	const gchar *host = g_object_get_data(G_OBJECT(assistant->router_stack), "server");
	const gchar *ftp_user = gtk_entry_get_text(GTK_ENTRY(assistant->ftp_user));
	const gchar *ftp_password = gtk_entry_get_text(GTK_ENTRY(assistant->ftp_password));
	gchar *message;
	RmFtp *ftp;

	/* Test ftp login */
	ftp = rm_ftp_init(host);
	if (ftp) {
		if (!rm_ftp_login(ftp, ftp_user, ftp_password)) {
			/* Error: Could not login to ftp */
			message = g_strdup(_("Please check your ftp user/password."));
			rm_object_emit_message(_("Login failed"), message);
			rm_ftp_shutdown(ftp);
			return FALSE;
		}
		rm_ftp_shutdown(ftp);

		/* Store FTP credentials */
		g_settings_set_string(assistant->profile->settings, "ftp-user", ftp_user);
		rm_password_set(assistant->profile, "ftp-password", ftp_password);
	}

	return TRUE;
}

/**
 * assistant_finish_pre:
 * @assistant: a #Assistant
 *
 * Pre finish page
 */
static void assistant_finish_pre(Assistant *assistant)
{
	/* Set title */
	gtk_header_bar_set_title(GTK_HEADER_BAR(assistant->header), _("Ready to go"));
}

/**
 * assistant_finish_post:
 * @assistant: a #Assistant
 *
 * Post finish page
 * Add new profile to list, trigger a reconnect of network monitor and set journal visible
 *
 * Returns: %TRUE
 */
static gboolean assistant_finish_post(Assistant *assistant)
{
	/* Enable telnet & capi port */
	if (rm_router_dial_number(assistant->profile, ROUTER_DIAL_PORT_AUTO, ROUTER_ENABLE_TELNET)) {
		rm_router_hangup(assistant->profile, ROUTER_DIAL_PORT_AUTO, ROUTER_ENABLE_TELNET);
	}
	if (rm_router_dial_number(assistant->profile, ROUTER_DIAL_PORT_AUTO, ROUTER_ENABLE_CAPI)) {
		rm_router_hangup(assistant->profile, ROUTER_DIAL_PORT_AUTO, ROUTER_ENABLE_CAPI);
	}

	/* Trigger network reconnect */
	rm_netmonitor_reconnect();

	/* Update filter */
	journal_update_filter();

	/* Set journal visible if needed */
	journal_set_visible(TRUE);

	return TRUE;
}

/** Assistant pages: Name, Pre/Post functions */
AssistantPage assistant_pages[] = {
	{ "welcome", assistant_welcome_pre, NULL },
	{ "profile", assistant_profile_pre, assistant_profile_post },
	{ "router", assistant_router_pre, assistant_router_post },
	{ "password", assistant_password_pre, assistant_password_post },
	{ "ftp_password", assistant_ftp_password_pre, assistant_ftp_password_post },
	{ "finish", assistant_finish_pre, assistant_finish_post },
	{ NULL, NULL, NULL }
};

/**
 * assistant_back_button_clicked_cb:
 * @next: a #GtkWidget
 * @user_data: a #Assistant
 *
 * Control function for next button clicked
 * Responsible for top buttons and pre and post states of child pages
 */
static void assistant_back_button_clicked_cb(GtkWidget *next, gpointer user_data)
{
	Assistant *assistant = user_data;

	/* In case we are on the first page, exit assistant */
	if (assistant->current_page <= 0) {
		RmProfile *profile = rm_profile_get_active();

		gtk_widget_destroy(assistant->window);

		if (assistant->profile) {
			g_debug("%s(): Removing profile", __FUNCTION__);
			rm_profile_remove(assistant->profile);
		}

		g_slice_free(Assistant, assistant);
		assistant = NULL;

		/* In case no profile is active and assistant is aborted, exit application */
		if (!profile) {
			app_quit();
		}

		return;
	}

	/* Decrease current page counter */
	assistant->current_page--;

	if (!g_strcmp0(assistant_pages[assistant->current_page].name, "ftp_password") && !rm_router_need_ftp(assistant->profile)) {
		assistant->current_page--;
	}

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
 * assistant_next_button_clicked_cb:
 * @next: #GtkWidget
 * @user_data: a #Assistant
 *
 * Control function for next button clicked
 * Responsible for top buttons and pre and post states of child pages
 */
static void assistant_next_button_clicked_cb(GtkWidget *next, gpointer user_data)
{
	Assistant *assistant = user_data;

	gtk_button_set_label(GTK_BUTTON(assistant->back_button), _("Back"));

	/* In case we have a post page function and it returns FALSE, exit immediately */
	if (assistant_pages[assistant->current_page].post && !assistant_pages[assistant->current_page].post(assistant)) {
		return;
	}

	/* Check if we have reached the end */
	if (assistant->current_page >= assistant->max_page - 1) {
		gtk_widget_destroy(assistant->window);
		g_slice_free(Assistant, assistant);
		assistant = NULL;
		return;
	}

	/* Increate current page counter */
	assistant->current_page++;

	if (!g_strcmp0(assistant_pages[assistant->current_page].name, "ftp_password") && !rm_router_need_ftp(assistant->profile)) {
		assistant->current_page++;
	}

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
 * app_assistant:
 *
 * Create router setup assistant
 */
void app_assistant(void)
{
	Assistant *assistant = NULL;
	GtkBuilder *builder;
	GtkWidget *placeholder;
	GList *childrens;
	gint max = 0;

	builder = gtk_builder_new_from_resource("/org/tabos/roger/assistant.glade");
	if (!builder) {
		g_warning("Could not load assistant ui");
		return;
	}

	/* Allocate private structure */
	assistant = g_slice_alloc0(sizeof(Assistant));

	/* Connect to builder objects */
	assistant->window = GTK_WIDGET(gtk_builder_get_object(builder, "assistant"));
	gtk_application_add_window(GTK_APPLICATION(g_application_get_default()), GTK_WINDOW(assistant->window));
	assistant->header = GTK_WIDGET(gtk_builder_get_object(builder, "headerbar"));

	assistant->stack = GTK_WIDGET(gtk_builder_get_object(builder, "stack"));
	assistant->back_button = GTK_WIDGET(gtk_builder_get_object(builder, "back_button"));
	g_signal_connect(assistant->back_button, "clicked", G_CALLBACK(assistant_back_button_clicked_cb), assistant);
	assistant->next_button = GTK_WIDGET(gtk_builder_get_object(builder, "next_button"));
	g_signal_connect(assistant->next_button, "clicked", G_CALLBACK(assistant_next_button_clicked_cb), assistant);

	assistant->profile_name = GTK_WIDGET(gtk_builder_get_object(builder, "profile_name_entry"));
	g_signal_connect(assistant->profile_name, "changed", G_CALLBACK(assistant_profile_entry_changed), assistant);

	assistant->router_stack = GTK_WIDGET(gtk_builder_get_object(builder, "router_stack"));
	g_signal_connect(assistant->router_stack, "notify::visible-child", G_CALLBACK(assistant_router_stack_changed_cb), assistant);

	assistant->router_listbox = GTK_WIDGET(gtk_builder_get_object(builder, "router_listbox"));

	// TODO: One is enough...
	g_signal_connect(assistant->router_listbox, "row-activated", G_CALLBACK(assistant_router_listbox_row_activated_cb), assistant);
	g_signal_connect(assistant->router_listbox, "row-selected", G_CALLBACK(assistant_router_listbox_row_selected_cb), assistant);
	placeholder = gtk_label_new(_("No router detected"));
	gtk_widget_show(placeholder);
	gtk_list_box_set_placeholder(GTK_LIST_BOX(assistant->router_listbox), placeholder);

	assistant->server = GTK_WIDGET(gtk_builder_get_object(builder, "ip_address_entry"));
	g_signal_connect(assistant->server, "changed", G_CALLBACK(assistant_router_entry_changed), assistant);
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

