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

#include <config.h>

#include <ctype.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <rm/rmprofile.h>
#include <rm/rmobject.h>
#include <rm/rmfax.h>
#include <rm/rmmain.h>
#include <rm/rmstring.h>
#include <rm/rmnumber.h>
#include <rm/rmrouter.h>

#include <roger/journal.h>
#include <roger/contacts.h>
#include <roger/contactsearch.h>
#include <roger/print.h>

struct fax_ui {
	GtkWidget *window;
	GtkWidget *header_bar;
	GtkWidget *pickup_button;
	GtkWidget *hangup_button;
	GtkWidget *contact_search;
	GtkWidget *sender_label;
	GtkWidget *receiver_label;
	GtkWidget *progress_bar;
	//GtkWidget *contact_menu;
	//GtkWidget *box;
	GtkWidget *frame;
	//GtkWidget *scrolled_win;

	RmFax *fax;
	RmFaxStatus status;
	RmConnection *connection;
	gchar *file;
	gchar *number;
	gint status_timer_id;

	gchar *filter;
	gboolean discard;
};

gboolean fax_status_timer_cb(gpointer user_data)
{
	struct fax_ui *fax_ui = user_data;
	RmFaxStatus *fax_status = &fax_ui->status;
	gchar buffer[256];
	static gdouble old_percent = 0.0f;

	if (!rm_fax_get_status(fax_ui->fax, fax_ui->connection, fax_status)) {
		return TRUE;
	}

	if (old_percent != fax_status->percentage) {
		old_percent = fax_status->percentage;

		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fax_ui->progress_bar), fax_status->percentage);
	}

	/* Update status information */
	switch (fax_status->phase) {
	case FAX_PHASE_IDENTIFY:
		gtk_label_set_text(GTK_LABEL(fax_ui->receiver_label), fax_status->remote_ident);
		g_free(fax_status->remote_ident);

		/* Fall through */
	case FAX_PHASE_SIGNALLING:
		snprintf(buffer, sizeof(buffer), _("Transferring page %d of %d"), fax_status->page_current, fax_status->page_total);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), buffer);
		break;
	case FAX_PHASE_RELEASE:
		if (!fax_status->error_code) {
			g_debug("%s(): Fax transfer successful", __FUNCTION__);
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar),  _("Fax transfer successful"));
		} else {
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), _("Fax transfer failed"));
			g_debug("%s(): Fax transfer failed", __FUNCTION__);
		}
		if (g_settings_get_boolean(rm_profile_get_active()->settings, "fax-report")) {
			create_fax_report(fax_status, fax_ui->file, g_settings_get_string(rm_profile_get_active()->settings, "fax-report-dir"));
		}

		rm_fax_hangup(fax_ui->fax, fax_ui->connection);
		return TRUE;
		break;
	case FAX_PHASE_CALL:
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar),  _("Connecting..."));
		break;
	default:
		g_debug("%s(): Unhandled phase (%d)", __FUNCTION__, fax_status->phase);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar),  "");
		break;
	}

    gchar *time_diff;
    gchar *buf;
 
    time_diff = rm_connection_get_duration_time(fax_ui->connection);
    buf = g_strdup_printf(_("Time: %s"), time_diff);
    g_free(time_diff);
 
    if (roger_uses_headerbar()) {
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(fax_ui->header_bar), buf);
    } else {
        gchar *title = g_strdup_printf("%s - %s", rm_fax_get_name(fax_ui->fax), buf);
        gtk_window_set_title(GTK_WINDOW(fax_ui->window), title);
        g_free(title);
    }

    g_free(buf);


	return TRUE;
}

void fax_dial_buttons_set_dial(struct fax_ui *fax_ui, gboolean allow_dial)
{
	gtk_widget_set_sensitive(fax_ui->pickup_button, allow_dial);
	gtk_widget_set_sensitive(fax_ui->hangup_button, !allow_dial);
}

static void fax_connection_changed_cb(RmObject *object, gint event, RmConnection *connection, gpointer user_data)
{
	struct fax_ui *fax_ui = user_data;

	g_debug("%s(): connection %p", __FUNCTION__, connection);

	if (!connection || fax_ui->connection != connection) {
		return;
	}

	if (event == RM_CONNECTION_TYPE_DISCONNECT) {
		g_debug("%s(): cleanup", __FUNCTION__);
		//g_unlink(fax_ui->file);

		g_source_remove(fax_ui->status_timer_id);
		fax_ui->status_timer_id = 0;

		fax_status_timer_cb(fax_ui);

		fax_dial_buttons_set_dial(fax_ui, TRUE);
		fax_ui->connection = NULL;
	}
}

void fax_pickup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	RmProfile *profile = rm_profile_get_active();
	struct fax_ui *fax_ui = user_data;
	gchar *scramble;

	/* Get selected number (either number format or based on the selected name) */
	fax_ui->number = g_strdup(contact_search_get_number(CONTACT_SEARCH(fax_ui->contact_search)));
	if (!RM_EMPTY_STRING(fax_ui->number) && !(isdigit(fax_ui->number[0]) || fax_ui->number[0] == '*' || fax_ui->number[0] == '#' || fax_ui->number[0] == '+')) {
		fax_ui->number = g_object_get_data(G_OBJECT(fax_ui->contact_search), "number");
	}

	if (RM_EMPTY_STRING(fax_ui->number)) {
		return;
	}

	scramble = rm_number_scramble(fax_ui->number);
	g_debug("%s(): Dialing '%s'", __FUNCTION__, scramble);
	g_free(scramble);

	fax_ui->fax = rm_profile_get_fax(profile);
	fax_ui->connection = rm_fax_send(fax_ui->fax, fax_ui->file, fax_ui->number, rm_router_get_suppress_state(profile));

	if (fax_ui->connection) {
		fax_dial_buttons_set_dial(fax_ui, FALSE);
		if (!fax_ui->status_timer_id) {
			fax_ui->status_timer_id = g_timeout_add(250, fax_status_timer_cb, fax_ui);
		}
		g_debug("%s(): connection %p", __FUNCTION__, fax_ui->connection);
	}
}

void fax_hangup_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
	struct fax_ui *fax_ui = user_data;

	if (!fax_ui->connection) {
		return;
	}

	rm_fax_hangup(fax_ui->fax, fax_ui->connection);
	fax_dial_buttons_set_dial(fax_ui, TRUE);
}

gboolean app_show_fax_window_idle(gpointer data)
{
	GtkBuilder *builder;
	RmProfile *profile = rm_profile_get_active();
	struct fax_ui *fax_ui;
	gchar *fax_file = data ? data : g_strdup("/home/jbrummer/jbrummer-Fax-ID21.tiff");

	if (!profile) {
		//g_unlink(fax_file);
		return FALSE;
	}

	builder = gtk_builder_new_from_resource("/org/tabos/roger/fax.glade");
	if (!builder) {
		g_warning("Could not load fax ui");
		//g_unlink(fax_file);
		return FALSE;
	}

	/* Allocate fax window structure */
	fax_ui = g_slice_alloc0(sizeof(struct fax_ui));
	fax_ui->file = fax_file;

	fax_ui->fax = rm_profile_get_fax(profile);

	/* Connect to builder objects */
	fax_ui->window = GTK_WIDGET(gtk_builder_get_object(builder, "fax_window"));
	gtk_window_set_transient_for(GTK_WINDOW(fax_ui->window), GTK_WINDOW(journal_get_window()));

	fax_ui->pickup_button = GTK_WIDGET(gtk_builder_get_object(builder, "call_start_button"));
	fax_ui->hangup_button = GTK_WIDGET(gtk_builder_get_object(builder, "call_stop_button"));
	g_signal_connect(fax_ui->hangup_button, "clicked", G_CALLBACK(fax_hangup_button_clicked_cb), fax_ui);

	fax_ui->header_bar = GTK_WIDGET(gtk_builder_get_object(builder, "fax_headerbar"));

	fax_ui->frame = GTK_WIDGET(gtk_builder_get_object(builder, "fax_frame"));

	fax_ui->sender_label = GTK_WIDGET(gtk_builder_get_object(builder, "fax_sender_label"));
	gtk_label_set_text(GTK_LABEL(fax_ui->sender_label), g_strdup(g_settings_get_string(profile->settings, "fax-header")));
	fax_ui->receiver_label = GTK_WIDGET(gtk_builder_get_object(builder, "fax_receiver_label"));
	fax_ui->progress_bar = GTK_WIDGET(gtk_builder_get_object(builder, "fax_status_progress_bar"));
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(fax_ui->progress_bar), "");

	GtkWidget *grid2 = GTK_WIDGET(gtk_builder_get_object(builder, "fax_grid2"));
	fax_ui->contact_search = contact_search_new();
	gtk_grid_attach(GTK_GRID(grid2), fax_ui->contact_search, 0, 0, 1, 1);

	if (roger_uses_headerbar()) {
		/* Create header bar and set it to window */
		gtk_header_bar_set_title(GTK_HEADER_BAR(fax_ui->header_bar), rm_fax_get_name(fax_ui->fax));
		gtk_window_set_titlebar(GTK_WINDOW(fax_ui->window), fax_ui->header_bar);
	} else {
		GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(builder, "fax_grid"));
		gtk_window_set_title(GTK_WINDOW(fax_ui->window), rm_fax_get_name(fax_ui->fax));
		gtk_grid_attach(GTK_GRID(grid), fax_ui->header_bar, 0, 0, 3, 1);
	}
	//GtkWidget *grid2 = GTK_WIDGET(gtk_builder_get_object(builder, "fax_grid"));
	//gtk_grid_attach(GTK_GRID(grid2), contact_search_new(), 0, 0, 3, 1);

	g_signal_connect(fax_ui->pickup_button, "clicked", G_CALLBACK(fax_pickup_button_clicked_cb), fax_ui);


	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(G_OBJECT(builder));

	fax_dial_buttons_set_dial(fax_ui, TRUE);

	/* Connect connection signals */
	//g_signal_connect(rm_object, "connection-terminated", G_CALLBACK(fax_connection_terminated_cb), fax_ui);
	g_signal_connect(G_OBJECT(rm_object), "connection-changed", G_CALLBACK(fax_connection_changed_cb), fax_ui);

	gtk_widget_show_all(fax_ui->window);

	return FALSE;
}

gchar *convert_to_fax(gchar *file_name)
{
	GError *error = NULL;
	gchar *args[13];
	gchar *output;
	gchar *out_file;
	RmProfile *profile = rm_profile_get_active();

	/* convert ps to fax */
#ifdef G_OS_WIN32
	args[0] = g_settings_get_string(profile->settings, "ghostscript");
#elif __APPLE__
	args[0] = rm_get_directory("bin/gs");
#else
	args[0] = rm_get_directory("gs");
#endif
	g_debug("convert_fax_to_out_file(): args[0]=%s", args[0]);
	args[1] = "-q";
	args[2] = "-dNOPAUSE";
	args[3] = "-dSAFER";
	args[4] = "-dBATCH";

#ifdef FAX_SFF
	if (g_settings_get_boolean(profile->settings, "fax-sff")) {
		args[5] = "-sDEVICE=cfax";
		out_file = g_strdup_printf("%s/%s.sff", g_get_user_cache_dir(), g_path_get_basename(file_name));
	} else
#endif
	{
		args[5] = "-sDEVICE=tiffg4";
		out_file = g_strdup_printf("%s/%s.tif", g_get_user_cache_dir(), g_path_get_basename(file_name));
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
		g_warning("%s(): Error occurred: %s", __FUNCTION__, error ? error->message : "");
		g_free(args[0]);
		g_free(out_file);

		return NULL;
	}
	g_free(args[0]);

	if (!g_file_test(out_file, G_FILE_TEST_EXISTS)) {
		g_warning("%s(): Error converting print file to FAX format!", __FUNCTION__);
		g_free(out_file);

		return NULL;
	}

	return out_file;
}

void fax_process_cb(GtkWidget *widget, gchar *file_name, gpointer user_data)
{
	gchar *out_file;

	out_file = convert_to_fax(file_name);
	g_unlink(file_name);

	if (out_file) {
		g_idle_add(app_show_fax_window_idle, out_file);
	}
}

void fax_process_init(void)
{
	g_signal_connect(G_OBJECT(rm_object), "fax-process", G_CALLBACK(fax_process_cb), NULL);
}
