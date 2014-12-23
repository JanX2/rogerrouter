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

#if 0
/**
 * \brief Notify accept clicked
 * \param notify gnotification window
 * \param action clicked action
 * \param user_data user data - phone number
 */
static void notify_accept_clicked_cb(NotifyGNotification *notify, gchar *action, gpointer user_data)
{
	struct connection *connection = user_data;
	struct contact *contact;

	g_assert(connection != NULL);

	/** Ask for contact information */
	contact = contact_find_by_number(connection->remote_number);

	notify_gnotification_close(connection->notification, NULL);
	connection->notification = NULL;

	app_show_phone_window(contact);

	phone_pickup(connection->priv ? connection->priv : active_capi_connection);

	phone_add_connection(connection->priv ? connection->priv : active_capi_connection);
}

/**
 * \brief Deny incoming call
 * \param notify gnotification window
 * \param action clicked action
 * \param user_data popup widget pointer
 */
static void notify_deny_clicked_cb(NotifyGNotification *notify, gchar *action, gpointer user_data)
{
	struct connection *connection = user_data;

	g_assert(connection != NULL);

	notify_gnotification_close(connection->notification, NULL);
	connection->notification = NULL;

	phone_hangup(connection->priv ? connection->priv : active_capi_connection);
}
#endif

/**
 * \brief Close gnotification window
 */
static gboolean gnotification_close(gpointer notification)
{
	g_debug("close");
	g_application_withdraw_notification(G_APPLICATION(application), "new-call");
	return FALSE;
}

gboolean gnotification_update(gpointer data) {
	struct contact *contact = data;
	struct connection *connection;
	GNotification *notify;

	g_assert(contact != NULL);
	g_assert(contact->priv != NULL);

	connection = contact->priv;
	notify = connection->notification;

	if (notify && !EMPTY_STRING(contact->name)) {
		gchar *text;

		text = g_markup_printf_escaped(_("Name:\t%s\nNumber:\t%s\nCompany:\t%s\nStreet:\t%s\nCity:\t\t%s%s%s"),
		                               contact->name,
		                               contact->number,
		                               "",
		                               contact->street,
		                               contact->zip,
		                               contact->zip ? " " : "",
		                               contact->city);

		g_notification_set_body(notify, text);
		g_application_send_notification(G_APPLICATION(application), "new-call", notify);

		g_free(text);
	}

	return FALSE;
}

/**
 * \brief Reverse lookup of connection data and gnotification redraw
 * \param data connection pointer
 * \return NULL
 */
static gpointer gnotification_reverse_lookup_thread(gpointer data)
{
	struct connection *connection = data;
	struct contact *contact;

	contact = g_slice_new0(struct contact);

	contact->number = g_strdup(connection->remote_number);
	contact->priv = data;

	routermanager_lookup(contact->number, &contact->name, &contact->street, &contact->zip, &contact->city);

	g_idle_add(gnotification_update, contact);
	g_debug("done");

	return NULL;
}

/**
 * \brief "connection-notify" callback function
 * \param obj app object
 * \param connection connection structure
 * \param unused_pointer unused user pointer
 */
void gnotifications_connection_notify_cb(AppObject *obj, struct connection *connection, gpointer unused_pointer)
{
	GNotification *notify = NULL;
	gchar *text = NULL;
	struct contact *contact;
	gchar **numbers = NULL;
	gint count;
	gboolean found = FALSE;
	gboolean intern = FALSE;

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
		g_debug("type: %d, number '%s' not found", connection->type, call_scramble_number(connection->local_number));

		gchar *tmp = call_full_number(connection->local_number, FALSE);

		/* Match numbers against local number and check if we should show a gnotification */
		for (count = 0; count < g_strv_length(numbers); count++) {
			g_debug("type: %d, number '%s'/'%s' <-> '%s'", connection->type, call_scramble_number(connection->local_number), call_scramble_number(tmp), call_scramble_number(numbers[count]));
			if (!strcmp(tmp, numbers[count])) {
				found = TRUE;
				break;
			}
		}
		g_free(tmp);
	}

#ifdef ACCEPT_INTERN
	if (!found && !strncmp(connection->local_number, "**", 2)) {
		intern = TRUE;
		found = TRUE;
	}
#endif

	/* No match found? -> exit */
	if (!found) {
		return;
	}

	/* If its a disconnect or a connect, close previous gnotification window */
	if ((connection->type & CONNECTION_TYPE_DISCONNECT) || (connection->type & CONNECTION_TYPE_CONNECT)) {
		ringtone_stop();
		if (connection->notification) {
			g_object_unref(connection->notification);
			connection->notification = NULL;
		}
		return;
	}

	if (g_settings_get_boolean(gnotification_settings, "play-ringtones")) {
		ringtone_play(connection->type);
	}

	/** Ask for contact information */
	contact = contact_find_by_number(connection->remote_number);

	/* Create gnotification message */
	if (!intern) {
		text = g_markup_printf_escaped(_("Name:\t%s\nNumber:\t%s\nCompany:\t%s\nStreet:\t%s\nCity:\t\t%s%s%s"),
		                               contact->name ? contact->name : "",
		                               contact->number ? contact->number : "",
		                               contact->company ? contact->company : "",
		                               contact->street ? contact->street : "",
		                               contact->zip ? contact->zip : "",
		                               contact->zip ? " " : "",
		                               contact->city ? contact->city : ""
		                              );
	} else {
		text = g_markup_printf_escaped(_("Number:\t%s"), connection->local_number);
	}
	g_debug("text: '%s'", text);

	if (connection->type == CONNECTION_TYPE_INCOMING) {
		GFile *file;
		GIcon *icon;

		file = g_file_new_for_path("notification-message-roger-in.svg");
		icon = g_file_icon_new(file);

		notify = g_notification_new(_("Incoming call"));
		g_notification_set_body(notify, text);
		g_notification_set_icon(notify, G_ICON(icon));

		g_notification_add_button_with_target(notify, _("Accept"), "app.pickup", "i", connection->id);
		g_notification_add_button_with_target(notify, _("Decline"), "app.hangup", "i", connection->id);


		//notify_gnotification_add_action(notify, "accept", _("Accept"), notify_accept_clicked_cb, connection, NULL);
		//notify_gnotification_add_action(notify, "deny", _("Decline"), notify_deny_clicked_cb, connection, NULL);
	} else if (connection->type == CONNECTION_TYPE_OUTGOING) {
		GFile *file;
		GIcon *icon;
		gint duration = g_settings_get_int(gnotification_settings, "duration");

		file = g_file_new_for_path("notification-message-roger-out.svg");
		icon = g_file_icon_new(file);

		notify = g_notification_new(_("Outgoing call"));
		g_notification_set_body(notify, text);
		g_notification_set_icon(notify, G_ICON(icon));

		g_timeout_add_seconds(duration, gnotification_close, notify);
	} else {
		g_warning("Unhandled case in connection notify - gnotification!");
		g_free(text);
		return;
	}

	if (contact->image) {
		GtkWidget *image;
		GIcon *icon = NULL;

		image = gtk_image_new_from_pixbuf(contact->image);
		g_debug("image: %p", image),
		gtk_image_get_gicon(GTK_IMAGE(image), &icon, NULL);
		g_debug("icon: %p", icon),
		g_notification_set_icon(notify, G_ICON(icon));
	}

	connection->notification = notify;

	g_notification_set_priority(notify, G_NOTIFICATION_PRIORITY_URGENT);
	g_application_send_notification(G_APPLICATION(application), "new-call", notify);
	g_object_unref(notify);

	if (EMPTY_STRING(contact->name)) {
		g_debug("Starting lookup thread");
		if (1==0)g_thread_new("gnotification reverse lookup", gnotification_reverse_lookup_thread, connection);
	}

	g_free(text);
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
