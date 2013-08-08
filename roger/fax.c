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

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <libcallibre/appobject.h>
#include <libcallibre/profile.h>
#include <libcallibre/libfaxophone/fax.h>
#include <libcallibre/libfaxophone/phone.h>

#include <roger/phone.h>
#include <roger/main.h>
#include <roger/pref.h>
#include <roger/print.h>

static GtkWidget *progress_bar;
static GtkWidget *remote_label;
static GtkWidget *page_current_label;
static GtkWidget *status_current_label;

static void capi_connection_established_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;
	g_debug("capi_connection_established()");

	snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Connected"));
}

static void capi_connection_terminated_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	struct phone_state *state = user_data;
	g_debug("capi_connection_terminated()");

	snprintf(state->phone_status_text, sizeof(state->phone_status_text), _("Disconnected"));

	phone_remove_connection(connection);
	phone_remove_timer(state);
}

void fax_connection_status_cb(AppObject *object, gint status, struct capi_connection *connection, gpointer user_data)
{
	struct fax_status *fax_status;
	gchar buffer[256];

	fax_status = connection->priv;
	if (!fax_status) {
		g_warning("No status available");
		return;
	}

	if (!status && !fax_status->done) {
		switch (fax_status->phase) {
			case PHASE_B:
				g_debug("Ident: %s", fax_status->remote_ident);
				gtk_label_set_text(GTK_LABEL(remote_label), (fax_status->remote_ident));
				snprintf(buffer, sizeof(buffer), "%d/%d", fax_status->page_current, fax_status->page_total);

				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0);
				gtk_label_set_text(GTK_LABEL(page_current_label), buffer);
				gtk_label_set_text(GTK_LABEL(status_current_label), _("Transfer starting"));
				break;
			case PHASE_D:
				snprintf(buffer, sizeof(buffer), "%d", fax_status->page_current);
				g_debug("Pages: %s", buffer);
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), (float)fax_status->page_current / (float)fax_status->page_total);
				gtk_label_set_text(GTK_LABEL(page_current_label), buffer);
				gtk_label_set_text(GTK_LABEL(status_current_label), _("Transferring page"));
				break;
			case PHASE_E:
				if (!fax_status->error_code) {
					g_debug("Fax transfer successful");
					gtk_label_set_text(GTK_LABEL(status_current_label), _("Fax transfer successful"));
				} else {
					gtk_label_set_text(GTK_LABEL(status_current_label), _("Fax transfer failed"));
					g_debug("Fax transfer failed");
				}
				create_fax_report(fax_status, g_settings_get_string(profile_get_active()->settings, "fax-report-dir"));
				phone_hangup(connection);
				fax_status->done = TRUE;
				break;
			default:
				g_debug("Unhandled phase (%d)", fax_status->phase);
				break;
		}
	} else if (status == 1) {
		float percentage = 0.0f;
		gchar text[6];
		int percent = 0;
		static int old_percent = 0;

		percentage = (float) fax_status->bytes_sent / (float) fax_status->bytes_total;

		if (percentage > 1.0f) {
			percentage = 1.0f;
		}

		percent = percentage * 100;
		if (old_percent == percent) {
			return;
		}
		old_percent = percent;

		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), percentage);

		snprintf(text, sizeof(text), "%d%%", percent);
		//g_debug("Transfer at %s", text);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), text);
	}
}

gboolean fax_window_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	struct phone_state *state = data;

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

	g_unlink(state->priv);

	return FALSE;
}

void app_show_fax_window(gchar *tiff_file)
{
	GtkWidget *window;
	GtkWidget *grid;
	GtkWidget *frame_grid;
	GtkWidget *frame;
	GtkWidget *separator;
	GtkWidget *status_label;
	GtkWidget *progress_label;
	//GtkWidget *progress_bar;
	GtkWidget *remote_station_label;
	//GtkWidget *remote_label;
	GtkWidget *pages_label;
	//GtkWidget *page_current_label;
	gint pos_y = 0;
	struct phone_state *state;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	grid = gtk_grid_new();

	state = g_malloc0(sizeof(struct phone_state));
	state->type = PHONE_TYPE_FAX;

	state->priv = tiff_file;

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	gtk_window_set_title(GTK_WINDOW(window), _("Fax"));
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

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
	separator = gtk_frame_new(_("Information"));

	frame_grid = gtk_grid_new();
	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(frame_grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(frame_grid), 5);
	gtk_container_set_border_width(GTK_CONTAINER(frame_grid), 2);

	gtk_grid_attach(GTK_GRID(grid), separator, 0, pos_y, 3, 1);

	pos_y++;
	status_label = ui_label_new(_("Status:"));
	gtk_grid_attach(GTK_GRID(frame_grid), status_label, 0, pos_y, 1, 1);

	status_current_label = gtk_label_new("");
	gtk_grid_attach(GTK_GRID(frame_grid), status_current_label, 1, pos_y, 1, 1);

	pos_y++;
	pages_label = ui_label_new(_("Current page:"));
	gtk_grid_attach(GTK_GRID(frame_grid), pages_label, 0, pos_y, 1, 1);

	page_current_label = gtk_label_new("");
	gtk_grid_attach(GTK_GRID(frame_grid), page_current_label, 1, pos_y, 1, 1);

	pos_y++;
	remote_station_label = ui_label_new(_("Remote station:"));
	gtk_grid_attach(GTK_GRID(frame_grid), remote_station_label, 0, pos_y, 1, 1);

	remote_label = gtk_label_new("");
	gtk_grid_attach(GTK_GRID(frame_grid), remote_label, 1, pos_y, 1, 1);

	pos_y++;
	progress_label = ui_label_new(_("Progress:"));
	gtk_grid_attach(GTK_GRID(frame_grid), progress_label, 0, pos_y, 1, 1);

	progress_bar = gtk_progress_bar_new();
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
	gtk_widget_set_hexpand(progress_bar, TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "0%");
	gtk_grid_attach(GTK_GRID(frame_grid), progress_bar, 1, pos_y, 1, 1);

	gtk_container_add(GTK_CONTAINER(separator), frame_grid);

	pos_y++;
	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid), separator, 0, pos_y, 3, 1);

	pos_y++;
	state->phone_statusbar = gtk_statusbar_new();
	gtk_statusbar_push(GTK_STATUSBAR(state->phone_statusbar), 0, _("Connection: Idle | Duration: 00:00:00"));
	gtk_grid_attach(GTK_GRID(grid), state->phone_statusbar, 0, pos_y, 2, 1);

	/* We set the dial frame last, so that all other widgets are in place */
	frame = phone_dial_frame(window, NULL, state);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 0, 3, 1);

	gtk_container_add(GTK_CONTAINER(window), grid);
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(fax_window_delete_event_cb), state);

	g_signal_connect(app_object, "connection-status", G_CALLBACK(fax_connection_status_cb), state);
	g_signal_connect(app_object, "connection-established", G_CALLBACK(capi_connection_established_cb), state);
	g_signal_connect(app_object, "connection-terminated", G_CALLBACK(capi_connection_terminated_cb), state);

	gtk_widget_show_all(window);
}

gchar *convert_fax_to_tiff(gchar *file_name)
{
	GError *error = NULL;
	gchar *args[12];
	gchar *output;
	gchar *tiff;
	struct profile *profile = profile_get_active();

	tiff = g_strdup_printf("%s.tif", file_name);

	/* convert ps to tiff */
#ifdef G_OS_WIN32
	args[0] = g_settings_get_string(profile->settings, "ghostscript");
#else
	args[0] = "gs";
#endif
	args[1] = "-q";
	args[2] = "-dNOPAUSE";
	args[3] = "-dSAFER";
	args[4] = "-dBATCH";

	if (g_settings_get_int(profile->settings, "fax-controller") < 3) {
		args[5] = "-sDEVICE=tiffg4";
	} else {
		args[5] = "-sDEVICE=tiffg32d";
	}

	args[6] = "-sPAPERSIZE=a4";
	args[7] = "-dFIXEDMEDIA";
	if (g_settings_get_int(profile->settings, "fax-resolution")) {
		args[8] = "-r204x196";
	} else {
		args[8] = "-r204x98";
	}
	output = g_strdup_printf("-sOutputFile=%s", tiff);
	args[9] = output;
	args[10] = file_name;
	args[11] = NULL;

	if (!g_spawn_sync(NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, NULL, &error)) {
		g_warning("Error occurred: %s", error ? error->message : "");
		g_free(tiff);
		return NULL;
	}

	if (!g_file_test(tiff, G_FILE_TEST_EXISTS)) {
		g_free(tiff);
		return NULL;
	}

	return tiff;
}

void fax_process_cb(GtkWidget *widget, gchar *file_name, gpointer user_data)
{
	gchar *tiff;

	tiff = convert_fax_to_tiff(file_name);
	if (tiff) {
		app_show_fax_window(tiff);
	} else {
		g_warning("Error converting print file to TIFF!");
	}

	g_unlink(file_name);
}

void fax_process_init(void)
{
	g_signal_connect(app_object, "fax-process", G_CALLBACK(fax_process_cb), NULL);
}
