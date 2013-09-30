/**
 * Roger Router
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

#include <libroutermanager/plugins.h>
#include <libroutermanager/call.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/libfaxophone/phone.h>
#include <libroutermanager/fax_phone.h>
#include <libroutermanager/router.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/lookup.h>
#include <libroutermanager/gstring.h>

#include <roger/main.h>
#include <roger/application.h>
#include <roger/phone.h>
#include <roger/pref.h>
#include <roger/uitools.h>

#define ROUTERMANAGER_TYPE_NOTIFICATION_GTK_PLUGIN (routermanager_notification_gtk_plugin_get_type ())
#define ROUTERMANAGER_NOTIFICATION_GTK_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_NOTIFICATION_GTK_PLUGIN, RouterManagerNotificationGtkPlugin))

typedef struct {
        guint signal_id;
} RouterManagerNotificationGtkPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER_CONFIGURABLE(ROUTERMANAGER_TYPE_NOTIFICATION_GTK_PLUGIN, RouterManagerNotificationGtkPlugin, routermanager_notification_gtk_plugin)

static GSettings *notification_gtk_settings = NULL;
static gchar **selected_outgoing_numbers = NULL;
static gchar **selected_incoming_numbers = NULL;

/**
 * \brief Notify accept clicked
 * \param notify notification_gtk window
 * \param user_data user data - connection pointer
 */
static void notify_accept_clicked_cb(GtkWidget *notify, gpointer user_data)
{
	struct connection *connection = user_data;
	struct contact contact_s;
	struct contact *contact = &contact_s;

	/** Ask for contact information */
	memset(contact, 0, sizeof(struct contact));
	contact_s.number = connection->remote_number;
	emit_contact_process(contact);

	g_assert(connection != NULL);

	gtk_widget_destroy(connection->notification);
	connection->notification = NULL;

	app_show_phone_window(contact);

	phone_pickup(connection->priv ? connection->priv : active_capi_connection);

	phone_add_connection(connection->priv ? connection->priv : active_capi_connection);
}

/**
 * \brief Deny incoming call
 * \param notify notification_gtk window
 * \param user_data connection pointer
 */
static void notify_deny_clicked_cb(GtkWidget *notify, gpointer user_data)
{
	struct connection *connection = user_data;

	g_assert(connection != NULL);

	gtk_widget_destroy(connection->notification);
	connection->notification = NULL;

	phone_hangup(connection->priv ? connection->priv : active_capi_connection);
}

/**
 * \brief Close notification_gtk window
 */
static gboolean notification_gtk_close(gpointer window)
{
	gtk_widget_destroy(window);

	return FALSE;
}

/**
 * \brief Reverse lookup of connection data and notification redraw
 * \param data connection pointer
 * \return NULL
 */
static gpointer notification_reverse_lookup_thread(gpointer data)
{
	struct connection *connection = data;
	gchar *name;
	gchar *address;
	gchar *zip;
	gchar *city;
	gchar *number;

	number = connection->remote_number;

	if (routermanager_lookup(number, &name, &address, &zip, &city)) {
		if (connection->notification) {
			GtkWidget *contact_name_label = g_object_get_data(connection->notification, "name");
			GtkWidget *contact_street_label = g_object_get_data(connection->notification, "street");
			GtkWidget *contact_city_label = g_object_get_data(connection->notification, "city");
			gchar *tmp;

			gtk_label_set_text(GTK_LABEL(contact_name_label), name ? name : "");
			gtk_label_set_text(GTK_LABEL(contact_street_label), address ? address : "");

			tmp = g_strdup_printf("%s%s%s", zip ? zip : "", zip ? " " : "", city ? city : "");
			gtk_label_set_text(GTK_LABEL(contact_city_label), tmp);
			g_free(tmp);
		}
	}

	return NULL;
}

/**
 * \brief "connection-notify" callback function
 * \param obj app object
 * \param connection connection structure
 * \param unused_pointer unused user pointer
 */
void notification_gtk_connection_notify_cb(AppObject *obj, struct connection *connection, gpointer unused_pointer)
{
	GtkWidget *notify = NULL;
	GtkWidget *frame;
	GtkWidget *main_grid;
	GtkWidget *contact_grid;
	GtkWidget *phone_grid;
	GtkWidget *type_label;
	GtkWidget *local_label;
	GtkWidget *number_label;
	GtkWidget *company_label;
	GtkWidget *direction_label;
	GtkWidget *contact_company_label;
	GtkWidget *contact_number_label;
	GtkWidget *contact_name_label;
	GtkWidget *image;
	GtkWidget *street_label;
	GtkWidget *contact_street_label;
	GtkWidget *city_label;
	GtkWidget *contact_city_label;
	struct contact contact_s;
	struct contact *contact = &contact_s;
	gchar **numbers = NULL;
	gchar *tmp;
	gint count;
	gboolean found = FALSE;
	gint width, height;
	gint line = 0;

	/* Get notification numbers */
	if (connection->type & CONNECTION_TYPE_OUTGOING) {
		numbers = g_settings_get_strv(notification_gtk_settings, "outgoing-numbers");
	} else if (connection->type & CONNECTION_TYPE_INCOMING) {
		numbers = g_settings_get_strv(notification_gtk_settings, "incoming-numbers");
	}

	/* If numbers are NULL, exit */
	if (!numbers || !g_strv_length(numbers)) {
		g_debug("type: %d, numbers is empty", connection->type);
		return;
	}

	/* Match numbers against local number and check if we should show a notification */
	for (count = 0; count < g_strv_length(numbers); count++) {
		if (!strcmp(connection->local_number, numbers[count])) {
			found = TRUE;
			break;
		}
	}

	if (!found && connection->local_number[0] != '0') {
		g_debug("type: %d, number '%s' not found", connection->type, connection->local_number);

		gchar *tmp = call_full_number(connection->local_number, FALSE);

		/* Match numbers against local number and check if we should show a notification */
		for (count = 0; count < g_strv_length(numbers); count++) {
			g_debug("type: %d, number '%s'/'%s' <-> '%s'", connection->type, connection->local_number, tmp, numbers[count]);
			if (!strcmp(tmp, numbers[count])) {
				found = TRUE;
				break;
			}
		}
		g_free(tmp);
	}

	/* No match found? -> exit */
	if (!found) {
		return;
	}

	/* If its a disconnect or a connect, close previous notification window */
	if ((connection->type & CONNECTION_TYPE_DISCONNECT) || (connection->type & CONNECTION_TYPE_CONNECT)) {
		if (connection->notification) {
			gtk_widget_destroy(connection->notification);
			connection->notification = NULL;
		}
		return;
	}


	/** Ask for contact information */
	memset(contact, 0, sizeof(struct contact));
	contact_s.number = connection->remote_number;
	emit_contact_process(contact);

	notify = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	frame = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(frame), 10, 10, 10, 10);

	main_grid = gtk_grid_new();
	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(main_grid), 15);
	gtk_grid_set_column_spacing(GTK_GRID(main_grid), 15);

	contact_grid = gtk_grid_new();
	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(contact_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(contact_grid), 5);

	/* Type */
	type_label = gtk_label_new("");
	gtk_widget_set_hexpand(type_label, TRUE);
	gtk_grid_attach(GTK_GRID(contact_grid), type_label, 0, line, 3, 1);

	/* Local */
	line++;
	local_label = gtk_label_new("");
	gtk_widget_set_hexpand(local_label, TRUE);
	gtk_grid_attach(GTK_GRID(contact_grid), local_label, 0, line, 3, 1);

	image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_DIALOG);
	gtk_widget_set_hexpand(image, TRUE);
	gtk_widget_set_vexpand(image, TRUE);
	gtk_grid_attach(GTK_GRID(contact_grid), image, 0, 2, 1, 4);

	/* Name */
	line++;
	direction_label = gtk_label_new(_("Name:"));
	gtk_misc_set_alignment(GTK_MISC(direction_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), direction_label, 1, line, 1, 1);

	contact_name_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(contact_name_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_name_label, 2, line, 1, 1);

	/* Number */
	line++;
	number_label = gtk_label_new(_("Number:"));
	gtk_misc_set_alignment(GTK_MISC(number_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), number_label, 1, line, 1, 1);

	contact_number_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(contact_number_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_number_label, 2, line, 1, 1);

	/* Company */
	line++;
	company_label = gtk_label_new(_("Company:"));
	gtk_misc_set_alignment(GTK_MISC(company_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), company_label, 1, line, 1, 1);

	contact_company_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(contact_company_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_company_label, 2, line, 1, 1);

	/* Street */
	line++;
	street_label = gtk_label_new(_("Street:"));
	gtk_misc_set_alignment(GTK_MISC(street_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), street_label, 1, line, 1, 1);

	contact_street_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(contact_street_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_street_label, 2, line, 1, 1);

	/* City */
	line++;
	city_label = gtk_label_new(_("City:"));
	gtk_misc_set_alignment(GTK_MISC(city_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), city_label, 1, line, 1, 1);

	contact_city_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(contact_city_label), 0, 0.5);
	gtk_grid_attach(GTK_GRID(contact_grid), contact_city_label, 2, line, 1, 1);

	gtk_grid_attach(GTK_GRID(main_grid), contact_grid, 0, 0, 1, 1);

	gtk_container_add(GTK_CONTAINER(frame), main_grid);
	gtk_container_add(GTK_CONTAINER(notify), frame);

	tmp = connection->local_number ? g_strdup_printf(_("(on %s)"), connection->local_number) : g_strdup(_("(on ?)"));
	gtk_label_set_text(GTK_LABEL(local_label), tmp);
	g_free(tmp);

	gtk_label_set_text(GTK_LABEL(contact_name_label), contact->name ? contact->name : "");
	gtk_label_set_text(GTK_LABEL(contact_number_label), contact->number ? contact->number : "");
	gtk_label_set_text(GTK_LABEL(contact_company_label), contact->company ? contact->company : "");
	gtk_label_set_text(GTK_LABEL(contact_street_label), contact->street ? contact->street : "");
	tmp = g_strdup_printf("%s%s%s", contact->zip ? contact->zip : "", contact->zip ? " " : "", contact->city ? contact->city : "");
	gtk_label_set_text(GTK_LABEL(contact_city_label), tmp);
	g_free(tmp);

	g_object_set_data(G_OBJECT(notify), "name", contact_name_label);
	g_object_set_data(G_OBJECT(notify), "street", contact_street_label);
	g_object_set_data(G_OBJECT(notify), "city", contact_city_label);

	if (contact->image) {
		gtk_image_set_from_pixbuf(GTK_IMAGE(image), contact->image);
	}

	if (connection->type == CONNECTION_TYPE_INCOMING) {
		GtkWidget *accept_button;
		GtkWidget *decline_button;

		tmp = ui_bold_text(_("Incoming call"));
		gtk_label_set_markup(GTK_LABEL(type_label), tmp);
		g_free(tmp);

		phone_grid = gtk_grid_new();
		/* Set standard spacing to 5 */
		gtk_grid_set_row_spacing(GTK_GRID(phone_grid), 15);
		gtk_grid_set_column_spacing(GTK_GRID(phone_grid), 5);

		accept_button = gtk_button_new_with_label(_("Accept"));
		g_signal_connect(accept_button, "clicked", G_CALLBACK(notify_accept_clicked_cb), connection);
		gtk_widget_set_hexpand(accept_button, TRUE);
		gtk_grid_attach(GTK_GRID(phone_grid), accept_button, 0, 0, 1, 1);

		decline_button = gtk_button_new_with_label(_("Decline"));
		g_signal_connect(decline_button, "clicked", G_CALLBACK(notify_deny_clicked_cb), connection);
		gtk_widget_set_hexpand(decline_button, TRUE);
		gtk_grid_attach(GTK_GRID(phone_grid), decline_button, 1, 0, 1, 1);

		gtk_grid_attach(GTK_GRID(main_grid), phone_grid, 0, 1, 1, 1);
	} else if (connection->type == CONNECTION_TYPE_OUTGOING) {
		tmp = ui_bold_text(_("Outgoing call"));
		gtk_label_set_markup(GTK_LABEL(type_label), tmp);
		g_free(tmp);
		g_timeout_add_seconds(5, notification_gtk_close, notify);
	}

	gtk_widget_show_all(notify);

	gtk_window_get_size(GTK_WINDOW(notify), &width, &height);

//#ifdef G_OS_WIN32
	//height += 32;
//#endif
	gtk_window_move(GTK_WINDOW(notify), gdk_screen_width() - width, gdk_screen_height() - height);

	gtk_window_stick(GTK_WINDOW(notify));
	gtk_window_set_keep_above(GTK_WINDOW(notify), TRUE);

	connection->notification = notify;

	if (EMPTY_STRING(contact->name)) {
		g_thread_new("notification reverse lookup", notification_reverse_lookup_thread, connection);
	}
}

/**
 * \brief Activate plugin (init libnotify and connect to call-notify signal)
 * \param plugin peas plugin
 */
void impl_activate(PeasActivatable *plugin)
{
	RouterManagerNotificationGtkPlugin *notify_plugin = ROUTERMANAGER_NOTIFICATION_GTK_PLUGIN(plugin);

	notification_gtk_settings = g_settings_new("org.tabos.roger.plugins.notification-gtk");

	gchar **incoming_numbers = g_settings_get_strv(notification_gtk_settings, "incoming-numbers");
	gchar **outgoing_numbers = g_settings_get_strv(notification_gtk_settings, "outgoing-numbers");

	if ((!incoming_numbers || !g_strv_length(incoming_numbers)) && (!outgoing_numbers || !g_strv_length(outgoing_numbers))) {
		g_settings_set_strv(notification_gtk_settings, "incoming-numbers", (const gchar * const*) router_get_numbers(profile_get_active()));
		g_settings_set_strv(notification_gtk_settings, "outgoing-numbers", (const gchar * const*) router_get_numbers(profile_get_active()));
	}

	/* Connect to "call-notify" signal */
	notify_plugin->priv->signal_id = g_signal_connect(G_OBJECT(app_object), "connection-notify", G_CALLBACK(notification_gtk_connection_notify_cb), NULL);
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerNotificationGtkPlugin *notify_plugin = ROUTERMANAGER_NOTIFICATION_GTK_PLUGIN(plugin);

	/* If signal handler is connected: disconnect */
	if (g_signal_handler_is_connected(G_OBJECT(app_object), notify_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), notify_plugin->priv->signal_id);
	}
}

void notification_gtk_settings_refresh_list(GtkListStore *list_store)
{
	gchar **numbers = router_get_numbers(profile_get_active());
	GtkTreeIter iter;
	gint count;
	gint index;

	selected_outgoing_numbers = g_settings_get_strv(notification_gtk_settings, "outgoing-numbers");
	selected_incoming_numbers = g_settings_get_strv(notification_gtk_settings, "incoming-numbers");

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

static void notification_gtk_outgoing_toggle_cb(GtkCellRendererToggle *toggle, gchar *path_str, gpointer user_data)
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

	g_settings_set_strv(notification_gtk_settings, "outgoing-numbers", (const gchar * const*) selected_outgoing_numbers);
}

static void notification_gtk_incoming_toggle_cb(GtkCellRendererToggle *toggle, gchar *path_str, gpointer user_data)
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

	g_settings_set_strv(notification_gtk_settings, "incoming-numbers", (const gchar * const*) selected_incoming_numbers);
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

	/* Settings grid */
	settings_grid = gtk_grid_new();

	/* Scrolled window */
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_window), 380);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_window), GTK_SHADOW_IN);
	gtk_widget_set_vexpand(scroll_window, TRUE);

	/* Treeview */
	view = gtk_tree_view_new();

	gtk_widget_set_hexpand(view, TRUE);
	gtk_widget_set_vexpand(view, TRUE);
	gtk_container_add(GTK_CONTAINER(scroll_window), view);
	gtk_grid_attach(GTK_GRID(settings_grid), scroll_window, 0, 0, 1, 1);

	list_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

	notification_gtk_settings_refresh_list(list_store);

	tree_model = GTK_TREE_MODEL(list_store);
 
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(tree_model));

	renderer = gtk_cell_renderer_text_new();
	number_column = gtk_tree_view_column_new_with_attributes(_("Number"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), number_column);

	renderer = gtk_cell_renderer_toggle_new();
	enable_column = gtk_tree_view_column_new_with_attributes(_("Outgoing"), renderer, "active", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), enable_column);
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(notification_gtk_outgoing_toggle_cb), tree_model);

	renderer = gtk_cell_renderer_toggle_new();
	enable_column = gtk_tree_view_column_new_with_attributes(_("Incoming"), renderer, "active", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), enable_column);
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(notification_gtk_incoming_toggle_cb), tree_model);

	return pref_group_create(settings_grid, _("Choose for which MSNs you want notifications"), TRUE, TRUE);
}
