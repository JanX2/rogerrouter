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
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>

#include <libroutermanager/appobject.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/contact.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/address-book.h>
#include <libroutermanager/call.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/router.h>
#include <libroutermanager/fax_phone.h>
#include <libroutermanager/libfaxophone/phone.h>
#include <libroutermanager/libfaxophone/isdn-convert.h>

#include <roger/journal.h>
#include <roger/pref.h>
#include <roger/phone.h>
#include <roger/fax.h>
#include <roger/main.h>
#include <roger/uitools.h>
#include <roger/icons.h>
#include <roger/application.h>
#include <roger/journal.h>

static GSList *phone_active_connections = NULL;

static GtkWidget *phone_window = NULL;

void phone_add_connection(gpointer connection)
{
	phone_active_connections = g_slist_prepend(phone_active_connections, connection);
}

void phone_remove_connection(gpointer connection)
{
	phone_active_connections = g_slist_remove(phone_active_connections, connection);
}

gpointer phone_get_active_connection(void)
{
	if (phone_active_connections) {
		return phone_active_connections->data;
	}

	return NULL;
}

/**
 * \brief Create elapsed time string from two times
 * \return time string
 */
static gchar *timediff_str(struct phone_state *state)
{
	gint duration;
	static gchar buf[9];
	gint hours;
	gint minutes;
	gint seconds;
	gdouble time;

	if (!state->phone_session_timer) {
		strncpy(buf, "00:00:00", sizeof(buf));
		return buf;
	}

	time = g_timer_elapsed(state->phone_session_timer, NULL);

	duration = (gint) time;
	seconds = duration % 60;
	minutes = (duration / 60) % 60;
	hours = duration / (60 * 60);

	if (snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hours, minutes, seconds) >= 9) {
		buf[8] = '\0';
	}

	return buf;
}

static gboolean phone_session_timer_cb(gpointer data)
{
	struct phone_state *state = data;
	gchar *time_diff = timediff_str(state);
	gpointer connection = phone_get_active_connection();
	gchar buf[64];

	snprintf(buf, sizeof(buf), _("Time: %s"), time_diff);

	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(state->phone_status_label), buf);

	if (connection) {
		/* Flush recording buffer (if needed) */
		phone_flush(connection);
	}

	return TRUE;
}

void phone_setup_timer(struct phone_state *state)
{
	if (state->phone_session_timer && state->phone_session_timer_id) {
		g_debug("Skip timer setup");
		return;
	}

	g_debug("phone_setup_timer()");
	state->phone_session_timer = g_timer_new();
	g_timer_start(state->phone_session_timer);
	state->phone_session_timer_id = g_timeout_add(200, phone_session_timer_cb, state);
}

void phone_remove_timer(struct phone_state *state)
{
	/* Make sure that the status bar information is updated */
	phone_session_timer_cb(state);

	/* Remove timer source */
	if (state->phone_session_timer_id) {
		g_source_remove(state->phone_session_timer_id);
		state->phone_session_timer_id = 0;
	}

	/* Remove gtimer */
	if (state->phone_session_timer) {
		g_timer_destroy(state->phone_session_timer);
		state->phone_session_timer = NULL;
	}
}

static void capi_connection_established_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;
	gint len = g_slist_length(phone_active_connections);
	g_debug("capi_connection_established()");

	if (len == 1) {
		snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Connected"));
		phone_setup_timer(state);
	}
}

static void capi_connection_terminated_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;
	GSList *list;
	g_debug("capi_connection_terminated()");
	gint len;

	for (list = phone_active_connections; list; list = list->next) {
		if (list->data == connection) {
			break;
		}
	}

	if (!list) {
		g_debug("IGNORING");
		return;
	}

	phone_remove_connection(connection);

	len = g_slist_length(phone_active_connections);
	if (!len) {
		snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Disconnected"));
		phone_remove_timer(state);
	} else {
		phone_hold(phone_get_active_connection(), FALSE);
	}
}

void phone_connection_notify_cb(AppObject *object, struct connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;

	if (connection->type == CONNECTION_TYPE_OUTGOING) {
		g_debug("Outgoing");
		if (!state->connection && state->number && !strcmp(state->number, connection->remote_number)) {
			snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Dialing"));
			state->connection = connection;
			g_debug("Connection: %p", connection);
			phone_setup_timer(state);
		}
	} else if (connection->type == (CONNECTION_TYPE_OUTGOING | CONNECTION_TYPE_CONNECT)) {
		g_debug("Connect, check %p == %p?", state->connection, connection);
		if (state->connection != connection) {
			return;
		}

		g_debug("Established");
		snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Established"));
	} else {
		if (connection->type & CONNECTION_TYPE_DISCONNECT) {
			g_debug("Disconnect, check %p == %p?", state->connection, connection);
			if (state->connection != connection) {
				return;
			}

			g_debug("Terminated");
			snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Terminated"));
			phone_remove_timer(state);

			state->connection = NULL;
			state->number = NULL;
		}
	}
}

static void phone_connection_failed(void)
{
	GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(phone_window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Connection problem"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(error_dialog), _("This is most likely due to a closed CAPI port or an invalid phone number.\n\nSolution: Dial #96*3* with your phone and check your numbers within the settings"));

	gtk_dialog_run(GTK_DIALOG(error_dialog));
	gtk_widget_destroy(error_dialog);
}

static void pickup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct profile *profile = profile_get_active();
	struct phone_state *state = user_data;

	state->number = gtk_entry_get_text(GTK_ENTRY(state->name_entry));
	if (EMPTY_STRING(state->number) || !(isdigit(state->number[0]) || state->number[0] == '*' || state->number[0] == '#' || state->number[0] == '+')) {
		state->number = g_object_get_data(G_OBJECT(state->name_entry), "number");
	}

	if (!EMPTY_STRING(state->number)) {
		g_debug("Want to dial: '%s'", call_scramble_number(state->number));

		switch (state->type) {
		case PHONE_TYPE_VOICE:
			if (g_settings_get_int(profile->settings, "port") == PORT_SOFTPHONE) {
				gint len = g_slist_length(phone_active_connections);
				if (len < 2) {
					/* Hold active call */
					if (len == 1) {
						phone_hold(phone_get_active_connection(), TRUE);
					}

					gpointer connection = phone_dial(state->number, g_settings_get_boolean(profile->settings, "suppress"));

					if (connection) {
						phone_add_connection(connection);
					} else {
						phone_connection_failed();
					}
				}
			} else {
				g_debug("router_dial_number()");
				gchar *number;

				number = g_strdup_printf("%s%s", g_settings_get_boolean(profile->settings, "suppress") ? "*31#" : "", state->number);

				router_dial_number(profile, g_settings_get_int(profile->settings, "port"), number);
				g_free(number);
			}
			break;
		case PHONE_TYPE_FAX: {
			struct fax_ui *fax_ui = state->priv;

			gpointer connection = fax_dial(fax_ui->file, state->number);
			if (connection) {
				phone_add_connection(connection);
				snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Dialing"));
				fax_window_clear(fax_ui);
				phone_setup_timer(state);
			} else {
				phone_connection_failed();
			}
			break;
		}
		default:
			g_warning("Unknown type selected");
			break;
		}
	}
}

static void hangup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct profile *profile = profile_get_active();
	struct phone_state *state = user_data;
	const gchar *number = gtk_entry_get_text(GTK_ENTRY(state->name_entry));

	if (state->type == PHONE_TYPE_FAX || g_settings_get_int(profile->settings, "port") == PORT_SOFTPHONE) {
		if (!phone_get_active_connection()) {
			return;
		}

		g_debug("Hangup requested for %p", phone_get_active_connection());
		phone_hangup(phone_get_active_connection());
	} else {
		router_hangup(profile, g_settings_get_int(profile->settings, "port"), number);
	}

	snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Idle"));
}

void menu_set_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	GtkAllocation allocation;
	GtkRequisition request;

	gtk_widget_get_preferred_size(gtk_widget_get_toplevel(GTK_WIDGET(menu)), NULL, &request);

	gdk_window_get_origin(gtk_widget_get_window(user_data), x, y);

	gtk_widget_get_allocation(user_data, &allocation);
	*x += allocation.x;
	*y += allocation.y + allocation.height;

	if (request.width < allocation.width) {
		gtk_widget_set_size_request(GTK_WIDGET(menu), allocation.width, -1);
	}

	*push_in = TRUE;
}

static void phone_set_dial_number(GtkMenuItem *item, gpointer user_data)
{
	GtkWidget *entry = g_object_get_data(G_OBJECT(item), "entry");
	gchar *name = g_object_get_data(G_OBJECT(item), "name");

	g_object_set_data(G_OBJECT(entry), "number", user_data);
	if (!EMPTY_STRING(name)) {
		gtk_entry_set_text(GTK_ENTRY(entry), name);
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "go-down-symbolic");
	} else {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, NULL);
	}
}

GdkPixbuf *image_get_scaled(GdkPixbuf *image, gint req_width, gint req_height)
{
	GdkPixbuf *scaled = NULL;
	gint width, height;
	gint orig_width, orig_height;
	gfloat factor;

	if (req_width != -1 && req_height != -1) {
		orig_width = req_width;
		orig_height = req_height;
	} else {
		gtk_icon_size_lookup(GTK_ICON_SIZE_DIALOG, &orig_width, &orig_height);
	}

	if (!image) {
#if defined(__APPLE__)
		image = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "avatar-default", orig_width, 0, NULL);
#else
		image = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "avatar-default-symbolic", orig_width, 0, NULL);
#endif
	}

	width = gdk_pixbuf_get_width(image);
	height = gdk_pixbuf_get_height(image);
	if (width > height) {
		factor = (float)width / (float)height;
		width = orig_width;
		height = orig_height / factor;
	} else {
		factor = (float)height / (float)width;
		height = orig_height;
		width = orig_width / (float)factor;
	}

	scaled = gdk_pixbuf_scale_simple(image, width, height, GDK_INTERP_BILINEAR);

	return scaled;
}

static void contact_number_menu(GtkWidget *entry, struct contact *contact)
{
	GtkWidget *menu;
	GSList *list;
	GtkWidget *item;
	gchar *tmp;

	gchar *prev_number = g_object_get_data(G_OBJECT(entry), "number");

	menu = gtk_menu_new();

	g_object_set_data(G_OBJECT(entry), "contact", contact);

	gtk_entry_set_text(GTK_ENTRY(entry), contact->name);
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "go-down-symbolic");

	for (list = contact->numbers; list; list = list->next) {
		struct phone_number *number = list->data;

		switch (number->type) {
		case PHONE_NUMBER_HOME:
			tmp = g_strdup_printf("%s:\t%s", _("HOME"), number->number);
			break;
		case PHONE_NUMBER_WORK:
			tmp = g_strdup_printf("%s:\t%s", _("WORK"), number->number);
			break;
		case PHONE_NUMBER_MOBILE:
			tmp = g_strdup_printf("%s:\t%s", _("MOBILE"), number->number);
			break;
		case PHONE_NUMBER_FAX_HOME:
			tmp = g_strdup_printf("%s:\t%s", _("HOME FAX"), number->number);
			break;
		case PHONE_NUMBER_FAX_WORK:
			tmp = g_strdup_printf("%s:\t%s", _("WORK FAX"), number->number);
			break;
		default:
			tmp = g_strdup_printf("%s:\t%s", _("UNKNOWN"), number->number);
			break;
		}
		item = gtk_check_menu_item_new_with_label(tmp);

		if (!EMPTY_STRING(prev_number) && !strcmp(prev_number, number->number)) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		}

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_object_set_data(G_OBJECT(item), "entry", entry);
		g_object_set_data(G_OBJECT(item), "name", contact->name);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(phone_set_dial_number), number->number);
		g_free(tmp);
	}

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, menu_set_position, entry, 0, gtk_get_current_event_time());
}

void name_entry_icon_press_cb(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
	struct contact *contact = g_object_get_data(G_OBJECT(entry), "contact");

	contact_number_menu(GTK_WIDGET(entry), contact);
}

static gboolean number_entry_match_selected_cb(GtkEntryCompletion *completion, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GValue value_contact = {0, };
	struct phone_state *state = user_data;
	GtkWidget *entry = state->name_entry;
	struct contact *contact;

	gtk_tree_model_get_value(model, iter, 1, &value_contact);
	contact = g_value_get_pointer(&value_contact);

	if (!contact->numbers) {
		return FALSE;
	}

	contact_number_menu(entry, contact);

	return FALSE;
}

gboolean number_entry_contains_completion_cb(GtkEntryCompletion *completion, const gchar *norm_key, GtkTreeIter *iter, gpointer user_data)
{
	GtkTreeModel *model = gtk_entry_completion_get_model(completion);
	const gchar *key = gtk_entry_get_text(GTK_ENTRY(gtk_entry_completion_get_entry(completion)));
	gchar *name;
	gboolean found = FALSE;

	gtk_tree_model_get(model, iter, 0, &name, -1);
	if (g_strcasestr(name, key)) {
		found = TRUE;
	}

	g_free(name);

	return found;
}

GtkWidget *phone_dial_frame(GtkWidget *window, struct contact *contact, struct phone_state *state)
{
	GtkEntryCompletion *completion;
	GtkListStore *list_store;
	GtkTreeIter iter;

	/* Name entry */
	state->name_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text(state->name_entry, _("Use autocompletion while typing names from your address book"));
	gtk_entry_set_placeholder_text(GTK_ENTRY(state->name_entry), _("Enter name or number"));
	gtk_widget_set_hexpand(state->name_entry, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(state->name_entry), TRUE);
	g_signal_connect(state->name_entry, "icon-press", G_CALLBACK(name_entry_icon_press_cb), NULL);

	if (contact) {
		struct contact *contact_copy = g_slice_new(struct contact);

		contact_copy = contact_dup(contact);

		emit_contact_process(contact_copy);

		if (!EMPTY_STRING(contact_copy->name)) {
			gtk_entry_set_text(GTK_ENTRY(state->name_entry), contact_copy->name);
			gtk_entry_set_icon_from_icon_name(GTK_ENTRY(state->name_entry), GTK_ENTRY_ICON_SECONDARY, "go-down-symbolic");
			g_object_set_data(G_OBJECT(state->name_entry), "contact", contact_copy);
			g_object_set_data(G_OBJECT(state->name_entry), "number", contact_copy->number);
		} else if (contact_copy->number) {
			gtk_entry_set_text(GTK_ENTRY(state->name_entry), contact_copy->number);
			g_object_set_data(G_OBJECT(state->name_entry), "number", contact_copy->number);
		}
	}

	/* Entry completion */
	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 0);
	list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);

	/* Example */
	GSList *contacts = NULL;
	GSList *list;
	contacts = address_book_get_contacts();

	for (list = contacts; list; list = list->next) {
		struct contact *contact = list->data;

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter, 0, contact->name, 1, contact, -1);
	}

	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(list_store));
	gtk_entry_completion_set_match_func(completion, number_entry_contains_completion_cb, NULL, NULL);
	gtk_entry_set_completion(GTK_ENTRY(state->name_entry), completion);
	g_signal_connect(completion, "match-selected", G_CALLBACK(number_entry_match_selected_cb), state);

	return state->name_entry;
}

GtkWidget *phone_dial_button_frame(GtkWidget *window, struct contact *contact, struct phone_state *state)
{
	GtkWidget *dial_frame_grid;
	GtkWidget *pickup_button;
	GtkWidget *hangup_button;
	GtkWidget *image;

	dial_frame_grid = gtk_grid_new();

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(dial_frame_grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(dial_frame_grid), 12);

	/* Pickup button */
	pickup_button = gtk_button_new();
	image = get_icon(APP_ICON_CALL, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(pickup_button), image);
	gtk_style_context_add_class(gtk_widget_get_style_context(pickup_button), GTK_STYLE_CLASS_SUGGESTED_ACTION);

	gtk_widget_set_vexpand(pickup_button, TRUE);

	gtk_widget_set_tooltip_text(pickup_button, _("Pick up"));
	g_signal_connect(pickup_button, "clicked", G_CALLBACK(pickup_button_clicked_cb), state);
	gtk_widget_set_can_default(pickup_button, TRUE);
	gtk_window_set_default(GTK_WINDOW(window), pickup_button);
	gtk_grid_attach(GTK_GRID(dial_frame_grid), pickup_button, 0, 0, 1, 1);

	/* Hangup button */
	hangup_button = gtk_button_new();
	image = get_icon(APP_ICON_HANGUP, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(hangup_button), image);
	gtk_style_context_add_class(gtk_widget_get_style_context(hangup_button), GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);

	gtk_widget_set_tooltip_text(hangup_button, _("Hang up"));
	g_signal_connect(hangup_button, "clicked", G_CALLBACK(hangup_button_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(dial_frame_grid), hangup_button, 1, 0, 1, 1);

	return dial_frame_grid;
}

static inline GtkWidget *phone_create_button(const gchar *text_hi, const gchar *text_lo, GtkWidget *icon)
{
	GtkWidget *button;
	GtkWidget *label_hi;
	GtkWidget *label_lo;
	GtkWidget *grid;

	grid = gtk_grid_new();
	gtk_widget_set_hexpand(grid, TRUE);
	gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);

	button = gtk_button_new();
	if (!icon) {
		label_hi = gtk_label_new(text_hi);
		gtk_label_set_use_markup(GTK_LABEL(label_hi), TRUE);
		gtk_grid_attach(GTK_GRID(grid), label_hi, 0, 0, 1, 1);
	} else {
		gtk_grid_attach(GTK_GRID(grid), icon, 0, 0, 1, 1);
		gtk_widget_set_margin(icon, 4, 3, 4, 4);
	}

	label_lo = gtk_label_new(text_lo);
	gtk_label_set_use_markup(GTK_LABEL(label_lo), TRUE);
	gtk_grid_attach(GTK_GRID(grid), label_lo, 0, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(button), grid);

	return button;
}

static void phone_dtmf_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	gint num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "num"));
	struct capi_connection *connection = phone_get_active_connection();

	if (connection) {
		phone_send_dtmf_code(connection, num);
	} else {
		struct phone_state *state = user_data;
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(state->name_entry));
		gchar *tmp = g_strdup_printf("%s%c", text, num);

		gtk_entry_set_text(GTK_ENTRY(state->name_entry), tmp);
		g_free(tmp);
	}
}

static struct phone_label {
	gchar *text;
	gchar *subtext;
	gchar chr;
} phone_labels[] = {
	{"<b>1</b>", "", '1'},
	{"<b>2</b>", "<small>ABC</small>", '2'},
	{"<b>3</b>", "<small>DEF</small>", '3'},
	{"<b>4</b>", "<small>GHI</small>", '4'},
	{"<b>5</b>", "<small>JKL</small>", '5'},
	{"<b>6</b>", "<small>MNO</small>", '6'},
	{"<b>7</b>", "<small>PQRS</small>", '7'},
	{"<b>8</b>", "<small>TUV</small>", '8'},
	{"<b>9</b>", "<small>WXYZ</small>", '9'},
	{"<b>*</b>", "", '*'},
	{"<b>0</b>", "<small>+</small>", '0'},
	{"<b>#</b>", "", '#'},
	{NULL, NULL}
};

GtkWidget *phone_dialpad_frame(struct phone_state *state)
{
	GtkWidget *frame = gtk_frame_new(NULL);
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *button;
	gint index;

	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
	gtk_widget_set_margin(grid, 6, 6, 6, 6);

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);

	/* Add buttons */
	for (index = 0; phone_labels[index].text != NULL; index++) {
		button = phone_create_button(phone_labels[index].text, phone_labels[index].subtext, NULL);
		g_object_set_data(G_OBJECT(button), "num", GINT_TO_POINTER(phone_labels[index].chr));
		g_signal_connect(button, "clicked", G_CALLBACK(phone_dtmf_clicked_cb), state);
		gtk_grid_attach(GTK_GRID(grid), button, index % 3, index / 3, 1, 1);
	}

	gtk_container_add(GTK_CONTAINER(frame), grid);
	gtk_widget_show_all(frame);

	return frame;
}

static void hold_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct capi_connection *connection = phone_get_active_connection();

	if (connection) {
		phone_hold(connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	}
}

static void mute_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct capi_connection *connection = phone_get_active_connection();
	gboolean active = FALSE;

	if (connection) {
		GtkWidget *image = user_data;

		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

		if (active) {
			gtk_image_set_from_icon_name(GTK_IMAGE(image), "microphone-sensitivity-muted-symbolic", GTK_ICON_SIZE_BUTTON);
		} else {
			gtk_image_set_from_icon_name(GTK_IMAGE(image), "audio-input-microphone-symbolic", GTK_ICON_SIZE_BUTTON);
		}

		phone_mute(connection, active);
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), active);
}

static void record_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct capi_connection *connection = phone_get_active_connection();

	if (connection) {
		const gchar *user_plugins = g_get_user_data_dir();
		gchar *path = g_build_filename(user_plugins, "roger", G_DIR_SEPARATOR_S, NULL);

		g_mkdir_with_parents(path, 0700);

		phone_record(connection, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)), path);

		g_free(path);
	}
}

static void c_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	struct phone_state *state = user_data;
	GtkWidget *entry = state->name_entry;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	if (text && strlen(text)) {
		gchar *new = g_strdup(text);

		new[strlen(text) - 1] = '\0';
		gtk_entry_set_text(GTK_ENTRY(entry), new);
		g_free(new);
	}
}

GtkWidget *phone_control_frame(struct phone_state *state)
{
	GtkWidget *grid;
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *frame = gtk_frame_new(NULL);

	grid = gtk_grid_new();

	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
	gtk_widget_set_hexpand(grid, FALSE);
	gtk_widget_set_margin(grid, 6, 6, 6, 6);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

	button = phone_create_button("<b>C</b>", "<small>Clear</small>", NULL);
	g_signal_connect(button, "clicked", G_CALLBACK(c_clicked_cb), state);
	gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

	image = gtk_image_new_from_icon_name("audio-input-microphone-symbolic", GTK_ICON_SIZE_BUTTON);
	button = phone_create_button(NULL, "<small>Mute</small>", image);
	g_signal_connect(button, "clicked", G_CALLBACK(mute_clicked_cb), image);
	gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 1, 1);

	image = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
	button = phone_create_button(NULL, "<small>Hold</small>", image);

	g_signal_connect(button, "clicked", G_CALLBACK(hold_clicked_cb), NULL);
	gtk_grid_attach(GTK_GRID(grid), button, 0, 2, 1, 1);

	image = gtk_image_new_from_icon_name("media-record-symbolic", GTK_ICON_SIZE_BUTTON);
	button = phone_create_button(NULL, "<small>Record</small>", image);

	g_signal_connect(button, "clicked", G_CALLBACK(record_clicked_cb), NULL);
	gtk_grid_attach(GTK_GRID(grid), button, 0, 3, 1, 1);

	gtk_container_add(GTK_CONTAINER(frame), grid);
	gtk_widget_show_all(frame);

	return frame;
}

GtkWidget *phone_call_frame(struct phone_state *state)
{
	GtkWidget *frame = gtk_frame_new(_("Active calls"));
	state->call_label = gtk_label_new("");

	gtk_container_add(GTK_CONTAINER(frame), state->call_label);

	gtk_widget_show_all(frame);

	return frame;
}

gboolean phone_window_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	struct phone_state *state = data;

	if (state && phone_get_active_connection()) {
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("A call is in progress. Still close this window?"));
		gint result = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		if (result != GTK_RESPONSE_YES) {
			return TRUE;
		}

		phone_remove_timer(state);
	}

	g_signal_handlers_disconnect_by_data(app_object, state);

	phone_window = NULL;

	return FALSE;
}

void phone_toggled_cb(GtkCheckMenuItem *item, gpointer user_data)
{
	gint type = GPOINTER_TO_INT(user_data);

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {
		g_settings_set_int(profile_get_active()->settings, "port", type);
	}
}

void app_show_phone_window(struct contact *contact)
{
	GtkWidget *window;
	GtkWidget *grid;
	GtkWidget *frame;
	gint pos_y;
	struct phone_state *state;

	if (!profile_get_active()) {
		return;
	}

	if (phone_window) {
		gtk_window_present(GTK_WINDOW(phone_window));
		return;
	}

	state = g_malloc0(sizeof(struct phone_state));
	state->type = PHONE_TYPE_VOICE;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(window), _("Phone"));
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(journal_get_window()));
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	g_signal_connect(window, "delete_event", G_CALLBACK(phone_window_delete_event_cb), state);

	grid = gtk_grid_new();
	gtk_widget_set_margin(grid, 12, 12, 12, 12);

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 18);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	/* Yes, we start at pos_y = 1, pos_y = 0 will be set below */
	pos_y = 1;
	state->dialpad_frame = phone_dialpad_frame(state);
	gtk_widget_set_no_show_all(state->dialpad_frame, TRUE);
	gtk_grid_attach(GTK_GRID(grid), state->dialpad_frame, 0, pos_y, 1, 1);

	state->control_frame = phone_control_frame(state);
	gtk_widget_set_no_show_all(state->control_frame, TRUE);
	gtk_grid_attach(GTK_GRID(grid), state->control_frame, 1, pos_y, 1, 1);

	/* Create header bar and set it to window */
	GtkWidget *header = gtk_header_bar_new();

	gtk_widget_set_hexpand(header, TRUE);
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), _("Phone"));
	gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header), _("Time: 00:00:00"));
	gtk_window_set_titlebar((GtkWindow *)(window), header);

	GtkWidget *button = gtk_menu_button_new();
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(button), GTK_ARROW_NONE);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), button);

	GtkWidget *menu = gtk_menu_new();
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(button), menu);

	GtkWidget *item;
	item = gtk_menu_item_new_with_label(_("Phones"));
	gtk_widget_set_sensitive(item, FALSE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	GSList *phone_list = router_get_phone_list(profile_get_active());
	GSList *phone_radio_list = NULL;
	struct phone *phone;
	gint type = g_settings_get_int(profile_get_active()->settings, "port");

	item = gtk_radio_menu_item_new_with_label(phone_radio_list, _("Softphone"));
	phone_radio_list = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	if (!type) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
	}
	g_signal_connect(item, "toggled", G_CALLBACK(phone_toggled_cb), GINT_TO_POINTER(0));

	while (phone_list) {
		phone = phone_list->data;

		item = gtk_radio_menu_item_new_with_label(phone_radio_list, phone->name);
		if (type == phone->type) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		}
		g_signal_connect(item, "toggled", G_CALLBACK(phone_toggled_cb), GINT_TO_POINTER(phone->type));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		phone_list = phone_list->next;
	}

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_check_menu_item_new_with_label(_("Suppress number"));
	g_settings_bind(profile_get_active()->settings, "suppress", item, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	gtk_widget_show_all(menu);

	state->phone_status_label = header;

	/* We set the dial frame last, so that all other widgets are in place */
	frame = phone_dial_frame(window, contact, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 0, 1, 1);

	frame = phone_dial_button_frame(window, contact, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 1, 0, 1, 1);

	gtk_container_add(GTK_CONTAINER(window), grid);

	g_signal_connect(app_object, "connection-notify", G_CALLBACK(phone_connection_notify_cb), state);

	g_signal_connect(app_object, "connection-established", G_CALLBACK(capi_connection_established_cb), state);
	g_signal_connect(app_object, "connection-terminated", G_CALLBACK(capi_connection_terminated_cb), state);

	gtk_widget_show_all(window);

	phone_window = window;
}
