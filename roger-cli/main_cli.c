/**
 * Roger Router CLI
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
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>

#include <libroutermanager/appobject.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/logging.h>
#include <libroutermanager/router.h>
#include <libroutermanager/gstring.h>

#include <libroutermanager/libfaxophone/faxophone.h>
#include <libroutermanager/libfaxophone/fax.h>
#include <libroutermanager/libfaxophone/phone.h>
#include <libroutermanager/fax_phone.h>

#include <config.h>

#undef _
#define _(x) x

static gboolean debug = FALSE;
static gboolean journal = FALSE;
static gboolean sendfax = FALSE;
static gboolean call = FALSE;
static gchar *file_name = NULL;
static gchar *number = NULL;

static GOptionEntry entries[] = {
	{"debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Enable debug messages", NULL},
	{"journal", 'j', 0, G_OPTION_ARG_NONE, &journal, "Prints journal", NULL},
	{"sendfax", 's', 0, G_OPTION_ARG_NONE, &sendfax, "Send fax", NULL},
	{"file", 'f', 0, G_OPTION_ARG_STRING, &file_name, "PDF/PS file", NULL},
	{"number", 'n', 0, G_OPTION_ARG_STRING, &number, "Remote phone number", NULL},
	{"call", 'c', 0, G_OPTION_ARG_NONE, &call, "Call number", NULL},
	{NULL}
};

/** Internal main loop */
GMainLoop *main_loop = NULL;
static gboolean success = FALSE;

/**
 * \brief Journal loaded callback
 * \param obj app object
 * \param journal journal call list
 * \param unused unused user pointer
 */
void journal_loaded_cb(AppObject *obj, GSList *journal, gpointer unused)
{
	GSList *list;

	/* Just dump journal to cli */
	for (list = journal; list != NULL; list = list->next) {
		struct call *call = list->data;

		g_printf("%15s %20s %20s %20s %20s %20s %5s\n",
		         call->date_time,
		         call->remote->name,
		         call->remote->number,
		         call->remote->city,
		         call->local->name,
		         call->local->number,
		         call->duration);
	}

	/* Exit main loop */
	g_main_loop_quit(main_loop);
}

/**
 * \brief Convert incoming fax (ps-format) to tiff (fax-format)
 * \param file_name incoming file name
 * \return tiff file name
 */
gchar *convert_fax_to_tiff(gchar *file_name)
{
	GError *error = NULL;
	gchar *args[12];
	gchar *output;
	gchar *tiff;

	if (strstr(file_name, ".tif")) {
		return file_name;
	}

	tiff = g_strdup_printf("%s.tif", file_name);

	/* convert ps to tiff */
	args[0] = "gs";
	args[1] = "-q";
	args[2] = "-dNOPAUSE";
	args[3] = "-dSAFER";
	args[4] = "-dBATCH";

	if (g_settings_get_int(profile_get_active()->settings, "fax-controller") < 3) {
		args[5] = "-sDEVICE=tiffg4";
	} else {
		args[5] = "-sDEVICE=tiffg32d";
	}

	args[6] = "-sPAPERSIZE=a4";
	args[7] = "-dFIXEDMEDIA";
	if (g_settings_get_int(profile_get_active()->settings, "fax-resolution")) {
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

/**
 * \brief CAPI connection established callback - just print message
 * \param object appobject
 * \param connection capi connection pointer
 * \param user_data user data pointer (NULL)
 */
static void capi_connection_established_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	g_message(_("Connected"));
}

/**
 * \brief CAPI connection terminated callback - just print message
 * \param object appobject
 * \param connection capi connection pointer
 * \param user_data user data pointer (NULL)
 */
static void capi_connection_terminated_cb(AppObject *object, struct capi_connection *connection, gpointer user_data)
{
	g_message(_("Disconnected"));
}

/**
 * \brief FAX connection status - show status message
 * \param object appobject
 * \param status fax connection status
 * \param connection capi connection pointer
 * \param user_data user data pointer (NULL)
 */
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
			snprintf(buffer, sizeof(buffer), "%d/%d", fax_status->page_current, fax_status->page_total);

			g_message(_("Transfer starting:"));
			g_message("%s", buffer);
			break;
		case PHASE_D:
			snprintf(buffer, sizeof(buffer), "%d", fax_status->page_current);
			g_message(_("Transferring page"));
			g_message("%s", buffer);
			break;
		case PHASE_E:
			if (!fax_status->error_code) {
				g_message("%s", "Fax transfer successful");
				success = TRUE;
			} else {
				g_message("%s", "Fax transfer failed");
				success = FALSE;
			}
			phone_hangup(connection);
			fax_status->done = TRUE;
			g_main_loop_quit(main_loop);
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

		snprintf(text, sizeof(text), "%d%%", percent);
		g_message("Transfer at %s", text);
	}
}


/**
 * \brief Main entry point for command line version
 * \param argc argument count
 * \param argv argument vector
 * \return exit code 0
 */
int main(int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context;
	gchar *tiff = NULL;
	int ret = 0;

#if !GLIB_CHECK_VERSION(2, 36, 0)
	/* Init g_type */
	g_type_init();
#endif

	context = g_option_context_new("-");
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		exit(1);
	}

	if (!journal && !(sendfax && file_name && number) && !(call && number)) {
		exit(-3);
	}

	/* Initialize routermanager */
	routermanager_init(debug, NULL);
	if (!profile_get_active()) {
		return 0;
	}

	/* Only show messages >= INFO */
	log_set_level(G_LOG_LEVEL_INFO);

	if (journal) {
		/* Connect "journal-loaded" callback */
		g_signal_connect(G_OBJECT(app_object), "journal-loaded", G_CALLBACK(journal_loaded_cb), NULL);

		/* Request router journal */
		router_load_journal(profile_get_active());
	}

	if (sendfax && file_name && number) {
		g_signal_connect(app_object, "connection-status", G_CALLBACK(fax_connection_status_cb), NULL);
		g_signal_connect(app_object, "connection-established", G_CALLBACK(capi_connection_established_cb), NULL);
		g_signal_connect(app_object, "connection-terminated", G_CALLBACK(capi_connection_terminated_cb), NULL);

		tiff = convert_fax_to_tiff(file_name);
		if (tiff) {
			gpointer connection = fax_dial(tiff, number, router_get_suppress_state(profile_get_active()));
			if (!connection) {
				g_error("could not create connection!");
				exit(-2);
			}
		} else {
			g_warning("Error converting print file to TIFF!");
			exit(-4);
		}

	}

	if (call && number) {
		g_signal_connect(app_object, "connection-established", G_CALLBACK(capi_connection_established_cb), NULL);
		g_signal_connect(app_object, "connection-terminated", G_CALLBACK(capi_connection_terminated_cb), NULL);

		phone_dial(number, FALSE);
	}

	/* Create and start g_main_loop */
	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);

	/* Shutdown routermanager */
	routermanager_shutdown();

	if (sendfax && tiff) {
		g_unlink(tiff);
	}

	if (sendfax && !success) {
		ret = -6;
	}

	return ret;
}
