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
#include <glib/gstdio.h>

#include <libroutermanager/appobject.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/libfaxophone/fax.h>
#include <libroutermanager/libfaxophone/phone.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/gstring.h>

#include <roger/phone.h>
#include <roger/main.h>
#include <roger/uitools.h>
#include <roger/print.h>
#include <roger/fax.h>
#include <roger/application.h>
#include <roger/journal.h>

#define FAX_SFF 1

/**
 * \brief CAPI connection established callback - Starts timer if needed
 * \param object appobject
 * \param connection capi connection pointer
 * \param user_data phone state pointer
 */
static void capi_connection_established_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;

	g_debug("%s(): called", __FUNCTION__);

	if (state->connection == connection) {
		phone_setup_timer(state);
	}
}

/**
 * \brief CAPI connection terminated callback - Stops timer
 * \param object appobject
 * \param connection capi connection pointer
 * \param user_data phone state pointer
 */
static void capi_connection_terminated_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;

	g_debug("%s(): called", __FUNCTION__);

	if (state->connection != connection) {
		return;
	}

#ifdef FAX_SFF
	struct fax_ui *fax_ui = state->priv;
	struct profile *profile = profile_get_active();

	if (g_settings_get_boolean(profile->settings, "fax-sff")) {
		int reason;

		g_debug("%s(): 0x%x/0x%x", __FUNCTION__, connection->reason, connection->reason_b3);

		switch (connection->reason) {
		case 0x3490:
		case 0x349f:
			reason = connection->reason_b3;
			break;
		default:
			reason = connection->reason;
			break;
		}

		if (reason == 0) {
			g_debug("Fax transfer successful");
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), _("Fax transfer successful"));
		} else {
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), _("Fax transfer failed"));
			g_debug("Fax transfer failed");
		}
	}
#endif

	/* Remove timer */
	phone_remove_timer(state);

	phone_allow_dial(state, TRUE);

	state->connection = NULL;
}

gboolean fax_update_ui(gpointer user_data)
{
	struct fax_ui *fax_ui = user_data;
	struct fax_status *fax_status = fax_ui->status;
	gchar buffer[256];
	float percentage = 0.0f;
	int percent = 0;
	static int old_percent = 0;
	gchar *tmp;

	/* update percentage */
	if (fax_status->progress_status) {
		percentage = (float) fax_status->bytes_sent / (float) fax_status->bytes_total;

		if (percentage > 1.0f) {
			percentage = 1.0f;
		}

		percent = percentage * 100;
		if (old_percent != percent) {
			old_percent = percent;

			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fax_ui->progress_bar), percentage);
		}
	} else if (!fax_status->done) {
		/* Update status information */
		switch (fax_status->phase) {
		case PHASE_B:
			g_debug("%s(): PHASE_B", __FUNCTION__);
			tmp = g_convert_utf8(fax_status->remote_ident, -1);
			g_debug("%s(): Ident: %s", __FUNCTION__, tmp);
			gtk_label_set_text(GTK_LABEL(fax_ui->remote_label), tmp);
			g_free(tmp);

			snprintf(buffer, sizeof(buffer), _("Transferring page %d of %d"), fax_status->page_current, fax_status->page_total);
			g_debug("%s(): %s", __FUNCTION__, buffer);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fax_ui->progress_bar), 0);
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), buffer);
			break;
		case PHASE_D:
			snprintf(buffer, sizeof(buffer), _("Transferring page %d of %d"), fax_status->page_current, fax_status->page_total);
			g_debug("%s(): %s", __FUNCTION__, buffer);

			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fax_ui->progress_bar), (float)fax_status->page_current / (float)fax_status->page_total);
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), buffer);
			break;
		case PHASE_E:
			if (!fax_status->error_code) {
				g_debug("%s(): Fax transfer successful", __FUNCTION__);
				gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar),  _("Fax transfer successful"));
			} else {
				gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), _("Fax transfer failed"));
				g_debug("%s(): Fax transfer failed", __FUNCTION__);
			}
			if (g_settings_get_boolean(profile_get_active()->settings, "fax-report")) {
				create_fax_report(fax_status, g_settings_get_string(profile_get_active()->settings, "fax-report-dir"));
			}
			phone_hangup(fax_status->connection);
			fax_status->done = TRUE;
			break;
		default:
			g_debug("%s(): Unhandled phase (%d)", __FUNCTION__, fax_status->phase);
			break;
		}
	}

	return FALSE;
}

static void show_active_call_dialog(GtkWidget *window)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, _("A call is in progress, hangup first"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void fax_connection_status_cb(AppObject *object, gint status, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;
	struct fax_ui *fax_ui = state->priv;

	if (connection && connection->priv) {
		memcpy(fax_ui->status, connection->priv, sizeof(struct fax_status));
	} else {
		memset(fax_ui->status, 0, sizeof(struct fax_status));
	}

	g_idle_add_full(G_PRIORITY_HIGH_IDLE, fax_update_ui, (gpointer)fax_ui, NULL);
}

/**
 * \brief Fax window delete event callback
 * \param widget window widget
 * \param event event structure
 * \param data pointer to phone_state structure
 * \return FALSE if window should be deleted, otherwise TRUE
 */
gboolean fax_window_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	struct phone_state *state = data;
	struct fax_ui *fax_ui = state->priv;

	if (state && state->connection) {
		show_active_call_dialog(widget);

		/* Keep window */
		return TRUE;
	}

	/* Disconnect all signal handlers */
	g_signal_handlers_disconnect_by_data(app_object, state);

	g_unlink(fax_ui->file);

	return FALSE;
}

/**
 * \brief Create fax menu which contains fax selection and suppress number toggle
 * \param profile pointer to current profile
 * \param state phone state pointer
 * \return newly create phone menu
 */
static GtkWidget *fax_create_menu(struct profile *profile, struct phone_state *state)
{
	GtkWidget *menu;
	GtkWidget *item;

	/* Create menu */
#ifdef OLD_MENU
	menu = gtk_menu_new();
#else
	GtkWidget *box;

	menu = gtk_popover_new(NULL);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin(box, 12, 12, 12, 12);
	gtk_container_add(GTK_CONTAINER(menu), box);
#endif

#ifdef OLD_MENU
	/* Fill menu */
	item = gtk_menu_item_new_with_label(_("Faxes"));
	gtk_widget_set_sensitive(item, FALSE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Add fax as it is always present and has index 0 */
	item = gtk_radio_menu_item_new_with_label(NULL, g_settings_get_string(profile->settings, "fax-header"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
	g_object_set_data(G_OBJECT(item), "phone_state", state);

	/* Add separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Add suppress check item */
	item = gtk_check_menu_item_new_with_label(_("Suppress number"));
	g_settings_bind(profile->settings, "suppress", item, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	gtk_widget_show_all(menu);
#else
	/* Fill menu */
	item = gtk_label_new(_("Fax devices"));
	gtk_widget_set_sensitive(item, FALSE);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

	/* Add fax as it is always present and has index 0 */
	item = gtk_radio_button_new_with_label(NULL, g_settings_get_string(profile->settings, "fax-header"));
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item), TRUE);
	g_object_set_data(G_OBJECT(item), "phone_state", state);

	/* Add separator */
	item = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

	/* Add suppress check item */
	item = gtk_check_button_new_with_label(_("Suppress number"));
	g_settings_bind(profile->settings, "suppress", item, "active", G_SETTINGS_BIND_DEFAULT);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);

	gtk_widget_show_all(box);
#endif

	return menu;
}

void fax_window_clear(gpointer priv)
{
	struct fax_ui *fax_ui = priv;

	gtk_label_set_text(GTK_LABEL(fax_ui->remote_label), "");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fax_ui->progress_bar), 0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), "");
}

GtkWidget *fax_information_frame(struct phone_state *state)
{
	GtkWidget *frame;
	GtkWidget *grid;
	GtkWidget *remote_station_label;
	GtkWidget *progress_label;
	//GtkWidget *pages_label;
	struct fax_ui *fax_ui = state->priv;
	gint pos_y = 0;

	/* Create frame */
	frame = gtk_frame_new(NULL);
	//gtk_widget_set_valign(frame, GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(frame, TRUE);

	/* Create inner grid */
	grid = gtk_grid_new();
	gtk_widget_set_hexpand(grid, FALSE);
	gtk_widget_set_margin(grid, 12, 6, 12, 12);
	gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
	gtk_container_add(GTK_CONTAINER(frame), grid);

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

	/* Add remote station line */
	pos_y++;
	remote_station_label = ui_label_new(_("To:"));
	gtk_grid_attach(GTK_GRID(grid), remote_station_label, 0, pos_y, 1, 1);

	fax_ui->remote_label = gtk_label_new("");
	gtk_grid_attach(GTK_GRID(grid), fax_ui->remote_label, 1, pos_y, 1, 1);

	/* Add progress line */
	pos_y++;
	progress_label = ui_label_new(_("Status:"));
	gtk_grid_attach(GTK_GRID(grid), progress_label, 0, pos_y, 1, 1);

	fax_ui->progress_bar = gtk_progress_bar_new();
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), TRUE);
	gtk_widget_set_hexpand(fax_ui->progress_bar, TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), "");
	gtk_grid_attach(GTK_GRID(grid), fax_ui->progress_bar, 1, pos_y, 1, 1);

#ifdef FAX_SFF
	if (g_settings_get_boolean(profile_get_active()->settings, "fax-sff")) {
		gtk_widget_set_no_show_all(fax_ui->remote_label, TRUE);
	}
#endif

	return frame;
}

void app_show_fax_window(gchar *fax_file)
{
	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *menubutton;
	GtkWidget *grid;
	GtkWidget *frame;
	struct phone_state *state;
	struct fax_ui *fax_ui;
	struct profile *profile = profile_get_active();

	if (!profile) {
		return;
	}

	fax_ui = g_malloc0(sizeof(struct fax_ui));
	fax_ui->file = fax_file;
	fax_ui->status = g_malloc0(sizeof(struct fax_status));

	/* Allocate phone state structure */
	state = g_malloc0(sizeof(struct phone_state));
	state->type = PHONE_TYPE_FAX;
	state->priv = fax_ui;

	/* Create window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 450, 280);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(fax_window_delete_event_cb), state);

	/* Create header bar and set it to window */
	header = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);

	gchar *tmp = g_strdup_printf(_("Fax - %s"), g_settings_get_string(profile->settings, "fax-header"));
	gtk_header_bar_set_title(GTK_HEADER_BAR (header), tmp);
	g_free(tmp);

	gtk_header_bar_set_subtitle(GTK_HEADER_BAR (header), _("Time: 00:00:00"));
	gtk_window_set_titlebar((GtkWindow *)(window), header);

	/* Create and add menu button to header bar */
	menubutton = gtk_menu_button_new();
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(menubutton), GTK_ARROW_NONE);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), menubutton);

	/* Create menu and add it to menu button */
#ifdef OLD_MENU
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(menubutton), fax_create_menu(profile, state));
#else
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menubutton), fax_create_menu(profile, state));
#endif

	state->headerbar = header;

	/* Create grid and attach it to the window */
	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), grid);

	/* Set margin to 12px for all sides */
	gtk_widget_set_margin(grid, 12, 12, 12, 12);

	/* Set standard spacing */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 18);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	frame = fax_information_frame(state);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 1, 2, 1);

	/* We set the dial frame last, so that all other widgets are in place */
	frame = phone_create_name_frame(window, NULL, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 0, 1, 1);

	/* Add dial button frame */
	frame = phone_dial_button_frame(window, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 1, 0, 1, 1);

	g_signal_connect(app_object, "connection-status", G_CALLBACK(fax_connection_status_cb), state);
	g_signal_connect(app_object, "connection-established", G_CALLBACK(capi_connection_established_cb), state);
	g_signal_connect(app_object, "connection-terminated", G_CALLBACK(capi_connection_terminated_cb), state);

	/* Allow dial out */
	phone_allow_dial(state, TRUE);

	gtk_widget_show_all(window);
	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
}

gboolean app_show_fax_window_idle(gpointer data)
{
	gchar *fax_file = data;

	app_show_fax_window(fax_file);
	return FALSE;
}

gchar *convert_to_fax(gchar *file_name)
{
	GError *error = NULL;
	gchar *args[13];
	gchar *output;
	gchar *out_file;
	struct profile *profile = profile_get_active();

	/* convert ps to fax */
#ifdef G_OS_WIN32
	args[0] = g_settings_get_string(profile->settings, "ghostscript");
#elif __APPLE__
	args[0] = get_directory("bin/gs");
#else
	args[0] = get_directory("gs");
#endif
	g_debug("convert_fax_to_out_file(): args[0]=%s", args[0]);
	args[1] = "-q";
	args[2] = "-dNOPAUSE";
	args[3] = "-dSAFER";
	args[4] = "-dBATCH";

#ifdef FAX_SFF
	if (g_settings_get_boolean(profile->settings, "fax-sff")) {
		args[5] = "-sDEVICE=cfax";
		out_file = g_strdup_printf("%s.sff", file_name);
	} else
#endif
	{
		args[5] = "-sDEVICE=tiffg4";
		out_file = g_strdup_printf("%s.tif", file_name);
	}

	args[6] = "-dPDFFitPage";
	args[7] = "-dMaxStripSize=0";
	switch (g_settings_get_int(profile->settings, "fax-resolution")) {
	case 2:
		/* Super - fine */
		args[8] = "-r204x392";
		break;
	case 1:
		/* Fine */
		args[8] = "-r204x196";
		break;
	default:
		/* Standard */
		args[8] = "-r204x98";
		break;
	}
	output = g_strdup_printf("-sOutputFile=%s", out_file);
	args[9] = output;
	args[10] = "-f";
	args[11] = file_name;
	args[12] = NULL;

	if (!g_spawn_sync(NULL, args, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL, NULL, &error)) {
		g_warning("Error occurred: %s", error ? error->message : "");
		g_free(args[0]);
		g_free(out_file);
		return NULL;
	}
	g_free(args[0]);

	if (!g_file_test(out_file, G_FILE_TEST_EXISTS)) {
		g_free(out_file);
		return NULL;
	}

	return out_file;
}

void fax_process_cb(GtkWidget *widget, gchar *file_name, gpointer user_data)
{
	gchar *out_file;

	out_file = convert_to_fax(file_name);
	if (out_file) {
		g_idle_add(app_show_fax_window_idle, out_file);
	} else {
		g_warning("Error converting print file to FAX format!");
	}

	g_unlink(file_name);
}

void fax_process_init(void)
{
	g_signal_connect(app_object, "fax-process", G_CALLBACK(fax_process_cb), NULL);
}
