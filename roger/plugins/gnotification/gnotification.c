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

#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libroutermanager/plugins.h>
#include <libroutermanager/call.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/libfaxophone/phone.h>
#include <libroutermanager/libfaxophone/ringtone.h>
#include <libroutermanager/fax_phone.h>
#include <libroutermanager/router.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/lookup.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/settings.h>

#include <roger/main.h>
#include <roger/application.h>
#include <roger/phone.h>
#include <roger/pref.h>

#define ROUTERMANAGER_TYPE_GNOTIFICATION_PLUGIN (routermanager_gnotification_plugin_get_type ())
#define ROUTERMANAGER_GNOTIFICATION_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_GNOTIFICATION_PLUGIN, RouterManagerGNotificationPlugin))

typedef struct {
	guint signal_id;
} RouterManagerGNotificationPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER_CONFIGURABLE(ROUTERMANAGER_TYPE_GNOTIFICATION_PLUGIN, RouterManagerGNotificationPlugin, routermanager_gnotification_plugin)

static GSettings *gnotification_settings = NULL;
static gchar **selected_outgoing_numbers = NULL;
static gchar **selected_incoming_numbers = NULL;
static guint missed_calls = 0;

/**
 * \brief Close gnotification window
 */
static gboolean gnotification_close(gpointer notification)
{
	g_application_withdraw_notification(G_APPLICATION(roger_app), notification);
	return FALSE;
}

gboolean gnotification_show(struct connection *connection, struct contact *contact)
{
	GNotification *notify = NULL;
	gchar *title;
	gchar *text = NULL;
	gchar *map = NULL;

	if (connection->type != CONNECTION_TYPE_INCOMING && connection->type != CONNECTION_TYPE_OUTGOING) {
		g_warning("Unhandled case in connection notify - gnotification!");

		return FALSE;
	}

	if (!EMPTY_STRING(contact->street) && !EMPTY_STRING(contact->city)) {
		gchar *map_un;

		map_un = g_strdup_printf("http://maps.google.de/?q=%s,%s %s",
					contact->street,
					contact->zip,
					contact->city);
		GRegex *pro = g_regex_new(" ", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		map = g_regex_replace_literal(pro, map_un, -1, 0, "%20", 0, NULL);
		g_regex_unref(pro);
		g_free(map_un);
	}

	/* Create notification message */
	text = g_markup_printf_escaped(_("Name:\t\t%s\nNumber:\t\t%s\nCompany:\t%s\nStreet:\t\t%s\nCity:\t\t%s%s%s\nMap:\t\t%s"),
	                               contact->name ? contact->name : "",
	                               contact->number ? contact->number : "",
	                               contact->company ? contact->company : "",
	                               contact->street ? contact->street : "",
	                               contact->zip ? contact->zip : "",
	                               contact->zip ? " " : "",
	                               contact->city ? contact->city : "",
	                               map ? map : ""
	                              );

	if (connection->type == CONNECTION_TYPE_INCOMING) {
		title = g_strdup_printf(_("Incoming call (on %s)"), connection->local_number);
	} else {
		title = g_strdup_printf(_("Outgoing call (on %s)"), connection->local_number);
	}

	notify = g_notification_new(title);
	g_free(title);

	g_notification_set_body(notify, text);
	g_free(text);

	connection->notification = g_strdup_printf("%s%s", connection->local_number, contact->number);

	if (connection->type == CONNECTION_TYPE_INCOMING) {
		g_notification_add_button_with_target(notify, _("Accept"), "app.pickup", "i", connection->id);
		g_notification_add_button_with_target(notify, _("Decline"), "app.hangup", "i", connection->id);
	} else if (connection->type == CONNECTION_TYPE_OUTGOING) {
		gint duration = g_settings_get_int(gnotification_settings, "duration");

		g_timeout_add_seconds(duration, gnotification_close, connection->notification);
	}

	g_notification_set_priority(notify, G_NOTIFICATION_PRIORITY_URGENT);
	g_application_send_notification(G_APPLICATION(roger_app), connection->notification, notify);
	g_object_unref(notify);

	return EMPTY_STRING(contact->name);
}

void gnotification_show_missed_calls(void)
{
	GNotification *notify = NULL;
	gchar *text = NULL;

	g_application_withdraw_notification(G_APPLICATION(roger_app), "missed-calls");

	/* Create notification message */
	text = g_strdup_printf(_("You have missed calls"));

	notify = g_notification_new(_("Missed call(s)"));

	g_notification_set_body(notify, text);
	g_free(text);

	g_notification_add_button_with_target(notify, _("Open journal"), "app.journal", NULL);
	g_application_send_notification(G_APPLICATION(roger_app), "missed-calls", notify);
	g_object_unref(notify);
}

/**
 * \brief "connection-notify" callback function
 * \param obj app object
 * \param connection connection structure
 * \param unused_pointer unused user pointer
 */
void gnotifications_connection_notify_cb(AppObject *obj, struct connection *connection, gpointer unused_pointer)
{
	gchar **numbers = NULL;
	gint count;
	gboolean found = FALSE;

	/* Get gnotification numbers */
	if (connection->type & CONNECTION_TYPE_OUTGOING) {
		numbers = g_settings_get_strv(gnotification_settings, "outgoing-numbers");
	} else if (connection->type & CONNECTION_TYPE_INCOMING) {
		numbers = g_settings_get_strv(gnotification_settings, "incoming-numbers");
	}

	/* If numbers are NULL, exit */
	if (!numbers || !g_strv_length(numbers)) {
		return;
	}

	/* Match numbers against local number and check if we should show a gnotification */
	for (count = 0; count < g_strv_length(numbers); count++) {
		if (!strcmp(connection->local_number, numbers[count])) {
			found = TRUE;
			break;
		}
	}

	if (!found && connection->local_number[0] != '0') {
		gchar *scramble_local = call_scramble_number(connection->local_number);
		gchar *tmp = call_full_number(connection->local_number, FALSE);
		gchar *scramble_tmp = call_scramble_number(tmp);

		g_debug("type: %d, number '%s' not found", connection->type, scramble_local);

		/* Match numbers against local number and check if we should show a gnotification */
		for (count = 0; count < g_strv_length(numbers); count++) {
			gchar *scramble_number = call_scramble_number(numbers[count]);

			g_debug("type: %d, number '%s'/'%s' <-> '%s'", connection->type, scramble_local, scramble_tmp, scramble_number);
			g_free(scramble_number);

			if (!strcmp(tmp, numbers[count])) {
				found = TRUE;
				break;
			}
		}

		g_free(tmp);
		g_free(scramble_local);
		g_free(scramble_tmp);
	}

	/* No match found? -> exit */
	if (!found) {
		return;
	}

	/* If its a disconnect or a connect, close previous gnotification window */
	if ((connection->type & CONNECTION_TYPE_DISCONNECT) || (connection->type & CONNECTION_TYPE_CONNECT)) {
		ringtone_stop();

		g_application_withdraw_notification(G_APPLICATION(roger_app), connection->notification);

		if (connection->type == CONNECTION_TYPE_MISSED) {
			missed_calls++;
			gnotification_show_missed_calls();
		}

		return;
	}

	if (g_settings_get_boolean(gnotification_settings, "play-ringtones")) {
		ringtone_play(connection->type);
	}

	/** Ask for contact information */
	struct contact *contact = contact_find_by_number(connection->remote_number);

	if (EMPTY_STRING(contact->name)) {
		routermanager_lookup(contact->number, &contact->name, &contact->street, &contact->zip, &contact->city);
	}

	gnotification_show(connection, contact);
}

/**
 * \brief Activate plugin (init libnotify and connect to call-notify signal)
 * \param plugin peas plugin
 */
void impl_activate(PeasActivatable *plugin)
{
	RouterManagerGNotificationPlugin *notify_plugin = ROUTERMANAGER_GNOTIFICATION_PLUGIN(plugin);

	gnotification_settings = rm_settings_plugin_new("org.tabos.roger.plugins.gnotification", "gnotification");

	gchar **incoming_numbers = g_settings_get_strv(gnotification_settings, "incoming-numbers");
	gchar **outgoing_numbers = g_settings_get_strv(gnotification_settings, "outgoing-numbers");

	if ((!incoming_numbers || !g_strv_length(incoming_numbers)) && (!outgoing_numbers || !g_strv_length(outgoing_numbers))) {
		g_settings_set_strv(gnotification_settings, "incoming-numbers", (const gchar * const *) router_get_numbers(profile_get_active()));
		g_settings_set_strv(gnotification_settings, "outgoing-numbers", (const gchar * const *) router_get_numbers(profile_get_active()));
	}

	/* Connect to "call-notify" signal */
	notify_plugin->priv->signal_id = g_signal_connect(G_OBJECT(app_object), "connection-notify", G_CALLBACK(gnotifications_connection_notify_cb), NULL);
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerGNotificationPlugin *notify_plugin = ROUTERMANAGER_GNOTIFICATION_PLUGIN(plugin);

	g_application_withdraw_notification(G_APPLICATION(roger_app), "missed-calls");

	/* If signal handler is connected: disconnect */
	if (g_signal_handler_is_connected(G_OBJECT(app_object), notify_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), notify_plugin->priv->signal_id);
	}
}

void gnotification_settings_refresh_list(GtkListStore *list_store)
{
	gchar **numbers = router_get_numbers(profile_get_active());
	GtkTreeIter iter;
	gint count;
	gint index;

	selected_outgoing_numbers = g_settings_get_strv(gnotification_settings, "outgoing-numbers");
	selected_incoming_numbers = g_settings_get_strv(gnotification_settings, "incoming-numbers");

	for (index = 0; index < g_strv_length(numbers); index++) {
		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter, 0, numbers[index], -1);
		gtk_list_store_set(list_store, &iter, 1, FALSE, -1);
		gtk_list_store_set(list_store, &iter, 2, FALSE, -1);

		if (selected_outgoing_numbers) {
			for (count = 0; count < g_strv_length(selected_outgoing_numbers); count++) {
				if (!strcmp(numbers[index], selected_outgoing_numbers[count])) {
					gtk_list_store_set(list_store, &iter, 1, TRUE, -1);
					break;
				}
			}
		}

		if (selected_incoming_numbers) {
			for (count = 0; count < g_strv_length(selected_incoming_numbers); count++) {
				if (!strcmp(numbers[index], selected_incoming_numbers[count])) {
					gtk_list_store_set(list_store, &iter, 2, TRUE, -1);
					break;
				}
			}
		}
	}
}

static void gnotification_outgoing_toggle_cb(GtkCellRendererToggle *toggle, gchar *path_str, gpointer user_data)
{
	GtkTreeModel *model = user_data;
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
	GtkTreeIter iter;
	GValue iter_value = {0};
	GValue name_value = {0};
	gboolean dial;
	gint count = 0;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, 1, &dial, -1);

	dial ^= 1;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, dial, -1);

	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get_value(model, &iter, 1, &iter_value);
			gtk_tree_model_get_value(model, &iter, 0, &name_value);

			if (g_value_get_boolean(&iter_value)) {
				selected_outgoing_numbers = g_realloc(selected_outgoing_numbers, (count + 1) * sizeof(char *));
				selected_outgoing_numbers[count] = g_strdup(g_value_get_string(&name_value));
				count++;
			}

			g_value_unset(&iter_value);
			g_value_unset(&name_value);
		} while (gtk_tree_model_iter_next(model, &iter));
	} else {
		g_debug("FAILED");
	}

	/* Terminate array */
	selected_outgoing_numbers = g_realloc(selected_outgoing_numbers, (count + 1) * sizeof(char *));
	selected_outgoing_numbers[count] = NULL;

	gtk_tree_path_free(path);

	g_settings_set_strv(gnotification_settings, "outgoing-numbers", (const gchar * const *) selected_outgoing_numbers);
}

static void gnotification_incoming_toggle_cb(GtkCellRendererToggle *toggle, gchar *path_str, gpointer user_data)
{
	GtkTreeModel *model = user_data;
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
	GtkTreeIter iter;
	GValue iter_value = {0};
	GValue name_value = {0};
	gboolean dial;
	gint count = 0;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, 2, &dial, -1);

	dial ^= 1;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 2, dial, -1);

	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get_value(model, &iter, 2, &iter_value);
			gtk_tree_model_get_value(model, &iter, 0, &name_value);

			if (g_value_get_boolean(&iter_value)) {
				selected_incoming_numbers = g_realloc(selected_incoming_numbers, (count + 1) * sizeof(char *));
				selected_incoming_numbers[count] = g_strdup(g_value_get_string(&name_value));
				count++;
			}

			g_value_unset(&iter_value);
			g_value_unset(&name_value);
		} while (gtk_tree_model_iter_next(model, &iter));
	} else {
		g_debug("FAILED");
	}

	/* Terminate array */
	selected_incoming_numbers = g_realloc(selected_incoming_numbers, (count + 1) * sizeof(char *));
	selected_incoming_numbers[count] = NULL;

	gtk_tree_path_free(path);

	g_settings_set_strv(gnotification_settings, "incoming-numbers", (const gchar * const *) selected_incoming_numbers);
}

GtkWidget *impl_create_configure_widget(PeasGtkConfigurable *config)
{
	GtkWidget *settings_grid;
	GtkWidget *scroll_window;
	GtkWidget *view;
	GtkListStore *list_store;
	GtkTreeModel *tree_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *enable_column;
	GtkTreeViewColumn *number_column;
	GtkWidget *play_ringtones_label;
	GtkWidget *play_ringtones_switch;
	GtkWidget *duration_label;
	GtkWidget *duration_spinbutton;
	GtkWidget *popup_grid;
	GtkAdjustment *adjustment;

	/* Settings grid */
	settings_grid = gtk_grid_new();

	/* Scrolled window */
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_window), 200);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_window), GTK_SHADOW_IN);
	gtk_widget_set_vexpand(scroll_window, TRUE);

	/* Treeview */
	view = gtk_tree_view_new();

	gtk_widget_set_hexpand(view, TRUE);
	gtk_widget_set_vexpand(view, TRUE);
	gtk_container_add(GTK_CONTAINER(scroll_window), view);

	list_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

	gnotification_settings_refresh_list(list_store);

	tree_model = GTK_TREE_MODEL(list_store);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(tree_model));

	renderer = gtk_cell_renderer_text_new();
	number_column = gtk_tree_view_column_new_with_attributes(_("Number"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), number_column);

	renderer = gtk_cell_renderer_toggle_new();
	enable_column = gtk_tree_view_column_new_with_attributes(_("Outgoing"), renderer, "active", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), enable_column);
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(gnotification_outgoing_toggle_cb), tree_model);

	renderer = gtk_cell_renderer_toggle_new();
	enable_column = gtk_tree_view_column_new_with_attributes(_("Incoming"), renderer, "active", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), enable_column);
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(gnotification_incoming_toggle_cb), tree_model);
	gtk_grid_attach(GTK_GRID(settings_grid), pref_group_create(scroll_window, _("Choose for which MSNs you want gnotifications"), TRUE, TRUE), 0, 0, 1, 1);

	popup_grid = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(popup_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(popup_grid), 15);

	duration_label = ui_label_new(_("Duration (outgoing)"));
	gtk_grid_attach(GTK_GRID(popup_grid), duration_label, 0, 0, 1, 1);

	adjustment = gtk_adjustment_new(0, 1, 60, 1, 10, 0);
	duration_spinbutton = gtk_spin_button_new(adjustment, 1, 0);
	gtk_widget_set_hexpand(duration_spinbutton, TRUE);
	g_settings_bind(gnotification_settings, "duration", duration_spinbutton, "value", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(popup_grid), duration_spinbutton, 1, 0, 1, 1);

	play_ringtones_label = ui_label_new(_("Play ringtones"));
	gtk_grid_attach(GTK_GRID(popup_grid), play_ringtones_label, 0, 2, 1, 1);

	play_ringtones_switch = gtk_switch_new();
	gtk_widget_set_halign(play_ringtones_switch, GTK_ALIGN_START);
	g_settings_bind(gnotification_settings, "play-ringtones", play_ringtones_switch, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_grid_attach(GTK_GRID(popup_grid), play_ringtones_switch, 1, 2, 1, 1);

	gtk_grid_attach(GTK_GRID(settings_grid), pref_group_create(popup_grid, _("Popup"), TRUE, TRUE), 0, 1, 1, 1);

	return settings_grid;
}
