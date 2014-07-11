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

static void capi_connection_established_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;
	struct fax_ui *fax_ui = state->priv;

	g_debug("capi_connection_established()");

	fax_ui->fax_connection = connection;

	snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Connected"));
}

static void capi_connection_terminated_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;
	struct fax_ui *fax_ui = state->priv;
	struct profile *profile = profile_get_active();
	int reason;

	if (fax_ui->fax_connection != NULL && fax_ui->fax_connection != connection) {
		return;
	}

	if (fax_ui->fax_connection == connection) {
		g_debug("capi_connection_terminated_cb(): 0x%x/0x%x", connection->reason, connection->reason_b3);

		switch (connection->reason) {
			case 0x3490:
			case 0x349f:
				reason = connection->reason_b3;
				break;
			default:
				reason = connection->reason;
				break;
		}

		if (g_settings_get_boolean(profile->settings, "fax-sff")) {
		if (reason == 0) {
			g_debug("Fax transfer successful");
			gtk_label_set_text(GTK_LABEL(fax_ui->status_current_label), _("Fax transfer successful"));
		} else {
			gtk_label_set_text(GTK_LABEL(fax_ui->status_current_label), _("Fax transfer failed"));
			g_debug("Fax transfer failed");
		}
		}
	}

	snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Disconnected"));

	phone_remove_connection(connection);
	phone_remove_timer(state);
	fax_ui->fax_connection = NULL;
}

gboolean fax_update_ui(gpointer user_data)
{
	struct fax_ui *fax_ui = user_data;
	struct fax_status *fax_status = fax_ui->status;
	gchar buffer[256];
	float percentage = 0.0f;
	gchar text[6];
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

			snprintf(text, sizeof(text), "%d%%", percent);
			//g_debug("Transfer at %s", text);
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), text);
		}
	} else if (!fax_status->done) {
		/* Update status information */
		switch (fax_status->phase) {
		case PHASE_B:
			g_debug("PHASE_B");
			tmp = g_convert_utf8(fax_status->remote_ident, -1);
			g_debug("Ident: %s", tmp);
			gtk_label_set_text(GTK_LABEL(fax_ui->remote_label), tmp);
			g_free(tmp);

			snprintf(buffer, sizeof(buffer), "%d/%d", fax_status->page_current, fax_status->page_total);
			g_debug("Pages: %s", buffer);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fax_ui->progress_bar), 0);
			gtk_label_set_text(GTK_LABEL(fax_ui->page_current_label), buffer);
			gtk_label_set_text(GTK_LABEL(fax_ui->status_current_label), _("Transfer starting"));
			break;
		case PHASE_D:
			snprintf(buffer, sizeof(buffer), "%d/%d", fax_status->page_current, fax_status->page_total);
			g_debug("Pages: %s", buffer);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fax_ui->progress_bar), (float)fax_status->page_current / (float)fax_status->page_total);
			gtk_label_set_text(GTK_LABEL(fax_ui->page_current_label), buffer);
			gtk_label_set_text(GTK_LABEL(fax_ui->status_current_label), _("Transferring page"));
			break;
		case PHASE_E:
			if (!fax_status->error_code) {
				g_debug("Fax transfer successful");
				gtk_label_set_text(GTK_LABEL(fax_ui->status_current_label), _("Fax transfer successful"));
			} else {
				gtk_label_set_text(GTK_LABEL(fax_ui->status_current_label), _("Fax transfer failed"));
				g_debug("Fax transfer failed");
			}
			create_fax_report(fax_status, g_settings_get_string(profile_get_active()->settings, "fax-report-dir"));
			phone_hangup(fax_status->connection);
			fax_status->done = TRUE;
			break;
		default:
			g_debug("Unhandled phase (%d)", fax_status->phase);
			break;
		}
	}

	return FALSE;
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

gboolean fax_window_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	struct phone_state *state = data;
	struct fax_ui *fax_ui = state->priv;

	if (state && state->connection) {
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, "A call is in progress. Still close this window?");
		gint result = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		if (result != GTK_RESPONSE_YES) {
			return TRUE;
		}

		phone_remove_timer(state);

	}

	g_signal_handlers_disconnect_by_data(app_object, state);

	g_unlink(fax_ui->file);

	return FALSE;
}

void fax_window_clear(gpointer priv)
{
	struct fax_ui *fax_ui = priv;

	gtk_label_set_text(GTK_LABEL(fax_ui->remote_label), "");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fax_ui->progress_bar), 0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), "");
	gtk_label_set_text(GTK_LABEL(fax_ui->page_current_label), "");
	gtk_label_set_text(GTK_LABEL(fax_ui->status_current_label), "");
}

void app_show_fax_window(gchar *fax_file)
{
	GtkWidget *window;
	GtkWidget *grid;
	GtkWidget *frame_grid;
	GtkWidget *frame;
	GtkWidget *status_label;
	GtkWidget *progress_label;
	GtkWidget *remote_station_label;
	GtkWidget *pages_label;
	gint pos_y = 0;
	struct phone_state *state;
	struct fax_ui *fax_ui;
	struct profile *profile = profile_get_active();

	fax_ui = g_malloc0(sizeof(struct fax_ui));
	fax_ui->file = fax_file;
	fax_ui->status = g_malloc0(sizeof(struct fax_status));

	state = g_malloc0(sizeof(struct phone_state));
	state->type = PHONE_TYPE_FAX;
	state->priv = fax_ui;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Fax"));
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	grid = gtk_grid_new();
	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	/**
	 * Connect to: <ENTRY> <PICKUP-BUTTON> <HANGUP-BUTTON>
	 * My number:  <ENTRY> <PHOTO>
	 *
	 * -- Information -------------------
	 * | Status         : <LABEL>       |
	 * | Pages          : <LABEL>       |
	 * | Remote-Station : <LABEL>       |
	 * | Progress       : <-----------> |
	 * ----------------------------------
	 */

	pos_y++;
	frame = gtk_frame_new(_("Information"));

	frame_grid = gtk_grid_new();
	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(frame_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(frame_grid), 5);
	gtk_container_set_border_width(GTK_CONTAINER(frame_grid), 2);

	gtk_grid_attach(GTK_GRID(grid), frame, 0, pos_y, 3, 1);

	pos_y++;
	status_label = ui_label_new(_("Status:"));
	gtk_grid_attach(GTK_GRID(frame_grid), status_label, 0, pos_y, 1, 1);

	fax_ui->status_current_label = gtk_label_new("");
	gtk_grid_attach(GTK_GRID(frame_grid), fax_ui->status_current_label, 1, pos_y, 1, 1);

	pos_y++;
	pages_label = ui_label_new(_("Current page:"));
	gtk_grid_attach(GTK_GRID(frame_grid), pages_label, 0, pos_y, 1, 1);

	fax_ui->page_current_label = gtk_label_new("");
	gtk_grid_attach(GTK_GRID(frame_grid), fax_ui->page_current_label, 1, pos_y, 1, 1);

	pos_y++;
	remote_station_label = ui_label_new(_("Remote station:"));
	gtk_grid_attach(GTK_GRID(frame_grid), remote_station_label, 0, pos_y, 1, 1);

	fax_ui->remote_label = gtk_label_new("");
	gtk_grid_attach(GTK_GRID(frame_grid), fax_ui->remote_label, 1, pos_y, 1, 1);

	pos_y++;
	progress_label = ui_label_new(_("Progress:"));
	gtk_grid_attach(GTK_GRID(frame_grid), progress_label, 0, pos_y, 1, 1);

	fax_ui->progress_bar = gtk_progress_bar_new();
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), TRUE);
	gtk_widget_set_hexpand(fax_ui->progress_bar, TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), "0%");
	gtk_grid_attach(GTK_GRID(frame_grid), fax_ui->progress_bar, 1, pos_y, 1, 1);

	gtk_container_add(GTK_CONTAINER(frame), frame_grid);

	state->phone_status_label = gtk_label_new(_("Connection: Idle | Duration: 00:00:00"));
	gtk_grid_attach(GTK_GRID(grid), state->phone_status_label, 0, 2, 3, 1);

	/* We set the dial frame last, so that all other widgets are in place */
	frame = phone_dial_frame(window, NULL, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 0, 3, 1);

	gtk_container_add(GTK_CONTAINER(window), grid);
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);

	if (g_settings_get_boolean(profile->settings, "fax-sff")) {
		gtk_widget_set_no_show_all(pages_label, TRUE);
		gtk_widget_set_no_show_all(fax_ui->page_current_label, TRUE);
		gtk_widget_set_no_show_all(remote_station_label, TRUE);
		gtk_widget_set_no_show_all(fax_ui->remote_label, TRUE);
	}

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(fax_window_delete_event_cb), state);

	g_signal_connect(app_object, "connection-status", G_CALLBACK(fax_connection_status_cb), state);
	g_signal_connect(app_object, "connection-established", G_CALLBACK(capi_connection_established_cb), state);
	g_signal_connect(app_object, "connection-terminated", G_CALLBACK(capi_connection_terminated_cb), state);

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

	if (g_settings_get_boolean(profile->settings, "fax-sff")) {
		args[5] = "-sDEVICE=cfax";
		out_file = g_strdup_printf("%s.sff", file_name);
	} else {
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
