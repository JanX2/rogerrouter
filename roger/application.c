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

#include <libroutermanager/profile.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/osdep.h>
#include <libroutermanager/router.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/net_monitor.h>
#include <libroutermanager/settings.h>

#include <roger/journal.h>
#include <roger/assistant.h>
#include <roger/main.h>
#include <roger/phone.h>
#include <roger/pref.h>
#include <roger/about.h>
#include <roger/fax.h>
#include <roger/contacts.h>

#include <config.h>

GtkApplication *application;
static gboolean startup_called = FALSE;
GSettings *app_settings = NULL;

struct cmd_line_option_state {
	gboolean debug;
	gboolean quit;
	gchar *number;
};

static struct cmd_line_option_state option_state;

typedef GtkApplication Application;
typedef GtkApplicationClass ApplicationClass;

G_DEFINE_TYPE(Application, application, GTK_TYPE_APPLICATION)

void app_show_contacts(void)
{
	contacts();
}

void app_show_preferences(void)
{
	preferences();
}

void app_show_help(void)
{
	os_execute("http://www.tabos.org/forum");
}

void app_quit(void)
{
	routermanager_shutdown();
	g_application_quit(G_APPLICATION(application));
}

void app_copy_ip(void)
{
	struct profile *profile = profile_get_active();
	gchar *ip;

	ip = router_get_ip(profile);
	if (ip) {
		g_debug("IP: '%s'", ip);
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_NONE), ip, -1);
		g_free(ip);
	} else {
		g_warning("Could not get ip");
	}
}

void app_reconnect(void)
{
	struct profile *profile = profile_get_active();

	router_reconnect(profile);
}

static void application_finalize(GObject *object)
{
	routermanager_shutdown();
	G_OBJECT_CLASS(application_parent_class)->finalize(object);
}

static void addressbook_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_show_contacts();
}

static void dialnumber_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_show_phone_window(NULL);
}

static void preferences_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_show_preferences();
}

static void donate_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	os_execute("http://www.tabos.org/roger");
}

static void forum_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_show_help();
}

static void about_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_show_about();
}

static void quit_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_quit();
}

static void copy_ip_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_copy_ip();
}

static void reconnect_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_reconnect();
}

#include <libroutermanager/libfaxophone/phone.h>
#include <libroutermanager/fax_phone.h>

static void pickup_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
#if 0
	gint32 id = g_variant_get_int32(parameter);
	struct connection *connection = connection_find_by_id(id);
	struct contact *contact;

	g_assert(connection != NULL);

	/** Ask for contact information */
	contact = contact_find_by_number(connection->remote_number);

	/* Close notifications */
	//notify_gnotification_close(connection->notification, NULL);
	connection->notification = NULL;

	if (!connection->priv)
		connection->priv = active_capi_connection;
	}

	/* Show phone window */
	app_show_phone_window(contact);

	/* Pickup phone and add connection */
	phone_pickup(connection->priv);
	phone_add_connection(connection->priv);
#endif
}

static void hangup_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
#if 0
	gint32 id = g_variant_get_int32(parameter);
	struct connection *connection = connection_find_by_id(id);

	g_assert(connection != NULL);

	//notify_gnotification_close(connection->notification, NULL);
	connection->notification = NULL;

	if (!connection->priv)
		connection->priv = active_capi_connection;
	}

	phone_hangup(connection->priv);
#endif
}

static GActionEntry apps_entries[] = {
	{"addressbook", addressbook_activated, NULL, NULL, NULL},
	{"preferences", preferences_activated, NULL, NULL, NULL},
	{"phone", dialnumber_activated, NULL, NULL, NULL},
	{"copy_ip", copy_ip_activated, NULL, NULL, NULL},
	{"reconnect", reconnect_activated, NULL, NULL, NULL},
	{"donate", donate_activated, NULL, NULL, NULL},
	{"forum", forum_activated, NULL, NULL, NULL},
	{"about", about_activated, NULL, NULL, NULL},
	{"quit", quit_activated, NULL, NULL, NULL},
	{"pickup", pickup_activated, "i", NULL, NULL},
	{"hangup", hangup_activated, "i", NULL, NULL},
};

static void application_startup(GApplication *application)
{
	startup_called = TRUE;
}

static void application_init(Application *app)
{
	/* Do nothing */
}

static gboolean show_message(gpointer message_ptr)
{
	gchar *message = message_ptr;

	GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", message ? message : "");

	g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_widget_show(dialog);

	return FALSE;
}

static void app_object_message_cb(AppObject *object, gint type, gchar *message)
{
	g_idle_add(show_message, message);
}

static void app_init(Application *app)
{
	GError *error = NULL;
	GMenu *menu;
	GMenu *section;
	GMenu *sub_section;

	g_action_map_add_action_entries(G_ACTION_MAP(application), apps_entries, G_N_ELEMENTS(apps_entries), application);

	menu = g_menu_new();

	g_menu_append(menu, _("Contacts"), "app.addressbook");
	g_menu_append(menu, _("Phone"), "app.phone");

	section = g_menu_new();
	g_menu_append(section, _("Copy IP address"), "app.copy_ip");
	g_menu_append(section, _("Reconnect"), "app.reconnect");
	g_menu_append_submenu(menu, _("Functions"), G_MENU_MODEL(section));

	section = g_menu_new();
	g_menu_append(section, _("Preferences"), "app.preferences");
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));

	section = g_menu_new();

	sub_section = g_menu_new();
	g_menu_append(sub_section, _("Donate"), "app.donate");
	g_menu_append(sub_section, _("Forum"), "app.forum");
	g_menu_append_submenu(section, _("Help"), G_MENU_MODEL(sub_section));

	g_menu_append(section, _("About"), "app.about");
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));

	section = g_menu_new();
	g_menu_append(section, _("Quit"), "app.quit");

	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));

	gtk_application_set_app_menu(GTK_APPLICATION(application), G_MENU_MODEL(menu));

	const gchar *user_plugins = g_get_user_data_dir();
	gchar *path = g_build_filename(user_plugins, "roger", G_DIR_SEPARATOR_S, "plugins", NULL);

	routermanager_plugins_add_search_path(path);
	routermanager_plugins_add_search_path(get_directory(APP_PLUGINS));
	g_free(path);

	if (routermanager_init(option_state.debug, &error) == FALSE) {
		printf("routermanager() failed: %s\n", error ? error->message : "");

		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "RouterManager failed");
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", error ? error->message : "");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		g_clear_error(&error);
		return;
	}

	g_signal_connect(app_object, "message", G_CALLBACK(app_object_message_cb), NULL);

	fax_process_init();

	journal_window(G_APPLICATION(app), NULL);

	if (net_is_online() && !profile_get_active()) {
		assistant();
	}

#if 0
	struct connection *connection = connection_add_call(2, CONNECTION_TYPE_INCOMING, "456789", "81570");

	emit_connection_notify(connection);
#endif
}

static void application_class_init(ApplicationClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	object_class->finalize = application_finalize;
}

G_GNUC_NORETURN static gboolean option_version_cb(const gchar *option_name, const gchar *value, gpointer data, GError **error)
{
	g_print("%s %s\n", PACKAGE_NAME, VERSION);
	exit(0);
}

const GOptionEntry all_options[] = {
	{"debug", 'd', 0, G_OPTION_ARG_NONE, &option_state.debug, "Enable debug", NULL},
	{"quit", 'q', 0, G_OPTION_ARG_NONE, &option_state.quit, "Quit", NULL},
	{"call", 'c', 0, G_OPTION_ARG_STRING, &option_state.number, "Remote phone number", NULL},
	{"version", 0, G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_CALLBACK, option_version_cb, NULL, NULL},
	{NULL}
};

GOptionContext *application_options_get_context(void)
{
	GOptionContext *context;

	context = g_option_context_new("-");
	g_option_context_add_main_entries(context, all_options, GETTEXT_PACKAGE);
	g_option_context_set_translation_domain(context, GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(FALSE));

	return context;
}

static void application_options_process(Application *app, const struct cmd_line_option_state *options)
{
	if (options->quit) {
		gdk_notify_startup_complete();
		exit(0);
	}

	g_settings_set_boolean(app_settings, "debug", options->debug);
}

static gint application_command_line_cb(GApplication *app, GApplicationCommandLine *command_line, gpointer data)
{
	Application *application = data;
	GOptionContext *context;
	int argc;
	char **argv;

	argv = g_application_command_line_get_arguments(command_line, &argc);

	memset(&option_state, 0, sizeof(option_state));

	context = application_options_get_context();
	g_option_context_set_help_enabled(context, TRUE);
	if (g_option_context_parse(context, &argc, &argv, NULL) == FALSE) {
		g_option_context_free(context);
		g_strfreev(argv);
		return 1;
	}

	g_option_context_free(context);

	application_options_process(application, &option_state);

	if (startup_called != FALSE) {
		app_init(application);
		gdk_notify_startup_complete();
		startup_called = FALSE;
	}

	if (option_state.number) {
		struct contact *contact;
		gchar *full_number;

		g_debug("number: %s", option_state.number);
		full_number = call_full_number(option_state.number, FALSE);

		/** Ask for contact information */
		contact = contact_find_by_number(full_number);

		app_show_phone_window(contact);
		g_free(full_number);
	}

	g_strfreev(argv);

	return 0;
}

#define APP_GSETTINGS_SCHEMA "org.tabos.roger"

Application *application_new(void)
{
	g_set_prgname(PACKAGE_NAME);
	g_set_application_name(PACKAGE_NAME);

	application = g_object_new(application_get_type(),
	                           "application-id", "org.tabos."PACKAGE,
	                           "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
	                           NULL);

	app_settings = rm_settings_new(APP_GSETTINGS_SCHEMA, NULL, "roger.conf");
	g_signal_connect(application, "startup", G_CALLBACK(application_startup), application);
	g_signal_connect(application, "command-line", G_CALLBACK(application_command_line_cb), application);

	return application;
}
