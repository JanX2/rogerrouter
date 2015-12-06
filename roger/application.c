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

GtkApplication *roger_app;
static gboolean startup_called = FALSE;
GSettings *app_settings = NULL;

struct cmd_line_option_state {
	gboolean debug;
	gboolean start_hidden;
	gboolean quit;
	gchar *number;
	gboolean assistant;
};

static struct cmd_line_option_state option_state;

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
	g_application_quit(G_APPLICATION(roger_app));
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

static void application_shutdown(GObject *object)
{
	routermanager_shutdown();
}

static void addressbook_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_show_contacts();
}

static void dialnumber_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	app_show_phone_window(NULL, NULL);
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
	gint32 id = g_variant_get_int32(parameter);
	struct connection *connection = connection_find_by_id(id);
	struct contact *contact;

	g_assert(connection != NULL);

	/** Ask for contact information */
	contact = contact_find_by_number(connection->remote_number);

	/* Close notifications */
	//notify_gnotification_close(connection->notification, NULL);
	connection->notification = NULL;

	/* Show phone window */
	app_show_phone_window(contact, connection);
}

static void hangup_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	gint32 id = g_variant_get_int32(parameter);
	struct connection *connection = connection_find_by_id(id);

	g_assert(connection != NULL);

	//notify_gnotification_close(connection->notification, NULL);
	connection->notification = NULL;

	if (!connection->priv) {
		connection->priv = active_capi_connection;
	}

	phone_hangup(connection->priv);
}

static void journal_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	GtkWidget *journal_win = journal_get_window();

	if (gtk_widget_get_visible(GTK_WIDGET(journal_win))) {
		gtk_window_present(GTK_WINDOW(journal_win));
	} else {
		gtk_widget_set_visible(GTK_WIDGET(journal_win), !gtk_widget_get_visible(GTK_WIDGET(journal_win)));
	}
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
	{"journal", journal_activated, NULL, NULL, NULL},
};

static void application_startup(GtkApplication *application)
{
	startup_called = TRUE;
}

static gboolean show_message(gpointer message_ptr)
{
	gchar *message = message_ptr;

	GtkWidget *dialog = gtk_message_dialog_new(roger_app ? gtk_application_get_active_window(roger_app) : NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", message ? message : "");

	g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_window_present(GTK_WINDOW(dialog));

	return FALSE;
}

static void app_object_message_cb(AppObject *object, gint type, gchar *message)
{
	g_idle_add(show_message, message);
}

static void app_init(GtkApplication *app)
{
	GError *error = NULL;
	GMenu *menu;
	GMenu *section;
	GMenu *sub_section;

#if GTK_CHECK_VERSION(3,14,0)
	const gchar *accels[] = {NULL, NULL, NULL, NULL};

	accels[0] = "<Control>p";
	gtk_application_set_accels_for_action(app, "app.phone", accels);

	accels[0] = "<Control>c";
	gtk_application_set_accels_for_action(app, "app.addressbook", accels);

	accels[0] = "<Control>q";
	accels[1] = "<Control>w";
	gtk_application_set_accels_for_action(app, "app.quit", accels);

	accels[0] = "<Control>s";
	gtk_application_set_accels_for_action(app, "app.preferences", accels);
#else
	gtk_application_add_accelerator(app, "<Control>p", "app.phone", NULL);
	gtk_application_add_accelerator(app, "<Control>c", "app.addressbook", NULL);
	gtk_application_add_accelerator(app, "<Control>q", "app.quit", NULL);
	gtk_application_add_accelerator(app, "<Control>s", "app.preferences", NULL);
#endif

	g_action_map_add_action_entries(G_ACTION_MAP(app), apps_entries, G_N_ELEMENTS(apps_entries), app);

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

	gtk_application_set_app_menu(GTK_APPLICATION(app), G_MENU_MODEL(menu));

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

	if (option_state.start_hidden) {
		journal_set_hide_on_start(TRUE);
		journal_set_hide_on_quit(TRUE);
	}

	journal_window(G_APPLICATION(app));

	if (net_is_online() && !profile_get_active()) {
		assistant();
	}

#if 0
	struct connection *connection = connection_add_call(2, CONNECTION_TYPE_INCOMING, "6173097", "08001888444");

	emit_connection_notify(connection);
#endif
}

G_GNUC_NORETURN static gboolean option_version_cb(const gchar *option_name, const gchar *value, gpointer data, GError **error)
{
	g_print("%s %s\n", PACKAGE_NAME, VERSION);
	exit(0);
}

const GOptionEntry all_options[] = {
	{"debug", 'd', 0, G_OPTION_ARG_NONE, &option_state.debug, "Enable debug", NULL},
	{"hidden", 'i', 0, G_OPTION_ARG_NONE, &option_state.start_hidden, "Start with hidden window", NULL},
	{"quit", 'q', 0, G_OPTION_ARG_NONE, &option_state.quit, "Quit", NULL},
	{"call", 'c', 0, G_OPTION_ARG_STRING, &option_state.number, "Remote phone number", NULL},
	{"version", 0, G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_CALLBACK, option_version_cb, NULL, NULL},
	{"assistant", 'a', 0, G_OPTION_ARG_NONE, &option_state.assistant, "Start assistant", NULL},
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

static void application_options_process(GtkApplication *app, const struct cmd_line_option_state *options)
{
	if (options->quit) {
		gdk_notify_startup_complete();
		exit(0);
	}

	g_settings_set_boolean(app_settings, "debug", options->debug);
}

static gint application_command_line_cb(GtkApplication *app, GApplicationCommandLine *command_line, gpointer data)
{
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

	application_options_process(app, &option_state);

	g_debug("startup_called: %d", startup_called);
	if (startup_called != FALSE) {
		app_init(app);
		gdk_notify_startup_complete();
		startup_called = FALSE;
	} else {
		extern GtkWidget *journal_win;
		gtk_widget_set_visible(GTK_WIDGET(journal_win), TRUE);
	}

	if (option_state.number) {
		struct contact *contact;
		gchar *full_number;

		g_debug("number: %s", option_state.number);
		full_number = call_full_number(option_state.number, FALSE);

		/** Ask for contact information */
		contact = contact_find_by_number(full_number);

		app_show_phone_window(contact, NULL);
		g_free(full_number);
	}

	if (option_state.assistant) {
		assistant();
	}

	g_strfreev(argv);

	return 0;
}

static void application_activated(GtkApplication *app, gpointer data)
{
	g_debug("called");
	journal_activated(NULL, NULL, NULL);
}

#define APP_GSETTINGS_SCHEMA "org.tabos.roger"

GtkApplication *application_new(void)
{
	g_set_prgname(PACKAGE_NAME);
	g_set_application_name(PACKAGE_NAME);

	roger_app = gtk_application_new("org.tabos.roger", G_APPLICATION_HANDLES_COMMAND_LINE);

	app_settings = rm_settings_new(APP_GSETTINGS_SCHEMA, NULL, "roger.conf");
	g_signal_connect(roger_app, "activate", G_CALLBACK(application_activated), roger_app);
	g_signal_connect(roger_app, "startup", G_CALLBACK(application_startup), roger_app);
	g_signal_connect(roger_app, "shutdown", G_CALLBACK(application_shutdown), roger_app);
	g_signal_connect(roger_app, "command-line", G_CALLBACK(application_command_line_cb), roger_app);

	return roger_app;
}
