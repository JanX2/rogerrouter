/*
 * The rm project
 * Copyright (c) 2012-2017 Jan-Michael Brummer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>

#include <rm/rmrouter.h>
#include <rm/rmobjectemit.h>
#include <rm/rmcontact.h>
#include <rm/rmstring.h>
#include <rm/rmfile.h>
#include <rm/rmcsv.h>
#include <rm/rmpassword.h>
#include <rm/rmmain.h>
#include <rm/rmcallentry.h>
#include <rm/rmnumber.h>
#include <rm/rmjournal.h>

/** Active router structure */
static RmRouter *active_router = NULL;
/** Global router plugin list */
static GSList *rm_router_list = NULL;
/** Router login blocked shield */
static gboolean rm_router_login_blocked = FALSE;

/**
 * \brief Free one phone list entry
 * \param data pointer to phone structure
 */
static void free_phone_list(gpointer data)
{
	RmPhoneInfo *phone = data;

	g_free(phone->name);
}

/**
 * \brief Free full phone list
 * \param phone_list phone list
 */
void rm_router_free_phone_list(GSList *phone_list)
{
	g_slist_free_full(phone_list, free_phone_list);
}

/**
 * \brief Get array of phone numbers
 * \param profile profile structure
 * \return phone number array
 */
gchar **rm_router_get_numbers(RmProfile *profile)
{
	return g_settings_get_strv(profile->settings, "numbers");
}

/**
 * \brief Check if router is present
 * \param router_info router information structure
 * \return present state
 */
gboolean rm_router_present(RmRouterInfo *router_info)
{
	GSList *list;

	g_debug("%s(): called", __FUNCTION__);
	if (!rm_router_list) {
		return FALSE;
	}

	for (list = rm_router_list; list != NULL; list = list->next) {
		RmRouter *router = list->data;

		if (router->present(router_info)) {
			active_router = router;
			return TRUE;
		}
	}

	return FALSE;
}

void rm_router_set_active(RmProfile *profile)
{
	if (active_router) {
		rm_router_present(profile->router_info);
		active_router->set_active(profile);
	}
}

/**
 * \brief Login to router
 * \param profile profile information structure
 * \return login state
 */
gboolean rm_router_login(RmProfile *profile)
{
	gboolean result;

	if (!active_router) {
		return FALSE;
	}

	if (rm_router_login_blocked) {
		g_debug("%s(): called, but blocked", __FUNCTION__);
		return FALSE;
	}

	result = active_router->login(profile);
	if (!result) {
		g_warning(R_("Login data are wrong or permissions are missing.\nPlease check your login data."));
		rm_object_emit_message(R_("Login failed"), R_("Login data are wrong or permissions are missing.\nPlease check your login data."));
		rm_router_login_blocked = TRUE;
	}

	return result;
}

/**
 * \brief Release router login blocked
 */
void rm_router_release_lock(void)
{
	g_debug("%s(): called", __FUNCTION__);
	rm_router_login_blocked = FALSE;
}

/**
 * \brief Check whether router login is blocked
 */
gboolean rm_router_is_locked(void)
{
	g_debug("%s(): called", __FUNCTION__);
	return rm_router_login_blocked;
}

/**
 * \brief Router logout
 * \param profile profile information structure
 * \return logout state
 */
gboolean rm_router_logout(RmProfile *profile)
{
	return active_router ? active_router->logout(profile, FALSE) : FALSE;
}

/**
 * \brief Get router host
 * \param profile profile information structure
 * \return router host or "" if no profile is active
 */
gchar *rm_router_get_host(RmProfile *profile)
{
	return profile ? g_settings_get_string(profile->settings, "host") : "";
}

/**
 * \brief Get login password
 * \param profile router information structure
 * \return login password
 */
gchar *rm_router_get_login_password(RmProfile *profile)
{
	return rm_password_get(profile, "login-password");
}

/**
 * \brief Get login user
 * \param profile router information structure
 * \return login user
 */
gchar *rm_router_get_login_user(RmProfile *profile)
{
	return g_settings_get_string(profile->settings, "login-user");
}

/**
 * \brief Get router FTP password
 * \param profile router information structure
 * \return router ftp password
 */
gchar *rm_router_get_ftp_password(RmProfile *profile)
{
	return rm_password_get(profile, "ftp-password");
}

/**
 * \brief Get router FTP user
 * \param profile router information structure
 * \return router ftp user
 */
gchar *rm_router_get_ftp_user(RmProfile *profile)
{
	return g_settings_get_string(profile->settings, "ftp-user");
}

/*
 * \brief Get international access code
 * \param profile router information structure
 * \return international access code
 */
gchar *rm_router_get_international_access_code(RmProfile *profile)
{
	if (!profile || !profile->settings) {
		return NULL;
	}

	return g_settings_get_string(profile->settings, "international-access-code");
}

/*
 * \brief Get national call prefix
 * \param profile router information structure
 * \return national call prefix
 */
gchar *rm_router_get_national_prefix(RmProfile *profile)
{
	if (!profile || !profile->settings) {
		return NULL;
	}

	return g_settings_get_string(profile->settings, "national-access-code");
}

/*
 * \brief Get router area code
 * \param profile router information structure
 * \return area code
 */
gchar *rm_router_get_area_code(RmProfile *profile)
{
	if (!profile || !profile->settings) {
		return NULL;
	}

	return g_settings_get_string(profile->settings, "area-code");
}

/*
 * \brief Get router country code
 * \param profile router information structure
 * \return country code
 */
gchar *rm_router_get_country_code(RmProfile *profile)
{
	if (!profile || !profile->settings) {
		return NULL;
	}

	return g_settings_get_string(profile->settings, "country-code");
}

/**
 * \brief Get router settings
 * \param profile profile information structure
 * \return router settings
 */
gboolean rm_router_get_settings(RmProfile *profile)
{
	return active_router ? active_router->get_settings(profile) : FALSE;
}

/**
 * \brief Get router name
 * \param profile profile information structure
 * \return router name
 */
const gchar *rm_router_get_name(RmProfile *profile)
{
	if (!profile || !profile->router_info) {
		return NULL;
	}

	return profile->router_info->name;
}

/**
 * \brief Get router version
 * \param profile profile information structure
 * \return router version
 */
const gchar *rm_router_get_version(RmProfile *profile)
{
	if (!profile || !profile->router_info) {
		return NULL;
	}

	return profile->router_info->version;
}

/**
 * \brief Get router journal
 * \param profile profile information structure
 * \return get journal return state
 */
gboolean rm_router_load_journal(RmProfile *profile)
{
	return active_router ? active_router->load_journal(profile) : FALSE;
}
/**
 * \brief Clear router journal
 * \param profile profile information structure
 * \return clear journal return state
 */
gboolean rm_router_clear_journal(RmProfile *profile)
{
	return active_router->clear_journal(profile);
}

/**
 * \brief Dial number
 * \param profile profile information structure
 * \param port dial port
 * \param number number to dial
 * \return return state of dial function
 */
gboolean rm_router_dial_number(RmProfile *profile, gint port, const gchar *number)
{
	gchar *target = rm_number_canonize(number);
	gboolean ret;

	ret = active_router->dial_number(profile, port, target);
	g_free(target);

	return ret;
}

/**
 * \brief Hangup call
 * \param profile profile information structure
 * \param port dial port
 * \param number number to dial
 * \return return state of hangup function
 */
gboolean rm_router_hangup(RmProfile *profile, gint port, const gchar *number)
{
	return active_router->hangup(profile, port, number);
}

/**
 * \brief Register new router
 * \param router new router structure
 */
gboolean rm_router_register(RmRouter *router)
{
	rm_router_list = g_slist_prepend(rm_router_list, router);

	return TRUE;
}

/**
 * \brief Initialize router (if available set internal router structure)
 * \return TRUE on success, otherwise FALSE
 */
gboolean rm_router_init(void)
{
	if (g_slist_length(rm_router_list)) {
		return TRUE;
	}

	g_warning("No router plugin registered!");
	return FALSE;
}

/**
 * \brief Shutdown router
 * \return TRUE
 */
void rm_router_shutdown(void)
{
	/* Free router list */
	if (rm_router_list != NULL) {
		g_slist_free(rm_router_list);
		rm_router_list = NULL;
	}

	/* Unset active router */
	active_router = NULL;
}

/**
 * \brief Router needs to process a new loaded journal (emit journal-process signal and journal-loaded)
 * \param journal journal list
 */
void rm_router_process_journal(GSList *journal)
{
	GSList *list;

	/* Parse offline journal and combine new entries */
	journal = rm_journal_load(journal);

	/* Store it back to disk */
	rm_journal_save(journal);

	/* Try to lookup entries in address book */
	for (list = journal; list; list = list->next) {
		RmCallEntry *call = list->data;

		rm_object_emit_contact_process(call->remote);
	}

	/* Emit "journal-loaded" signal */
	rm_object_emit_journal_loaded(journal);

}

/**
 * \brief Load fax file
 * \param profile profile structure
 * \param filename fax filename
 * \param len pointer to store data length to
 * \return fax data
 */
gchar *rm_router_load_fax(RmProfile *profile, const gchar *filename, gsize *len)
{
	return active_router->load_fax(profile, filename, len);
}

/**
 * \brief Load voice file
 * \param profile profile structure
 * \param name voice filename
 * \param len pointer to store data length to
 * \return voice data
 */
gchar *rm_router_load_voice(RmProfile *profile, const gchar *name, gsize *len)
{
	return active_router->load_voice(profile, name, len);
}

/**
 * \brief Get external IP address
 * \param profile profile structure
 * \return IP address
 */
gchar *rm_router_get_ip(RmProfile *profile)
{
	return active_router->get_ip(profile);
}

/**
 * \brief Reconnect network connection
 * \param profile profile structure
 * \param TRUE on success, otherwise FALSE
 */
gboolean rm_router_reconnect(RmProfile *profile)
{
	return active_router->reconnect(profile);
}

/**
 * \brief Delete fax file on router
 * \param profile profile structure
 * \param filename fax filename to delete
 * \param TRUE on success, otherwise FALSE
 */
gboolean rm_router_delete_fax(RmProfile *profile, const gchar *filename)
{
	return active_router->delete_fax(profile, filename);
}

/**
 * \brief Delete voice file on router
 * \param profile profile structure
 * \param filename voice filename to delete
 * \param TRUE on success, otherwise FALSE
 */
gboolean rm_router_delete_voice(RmProfile *profile, const gchar *filename)
{
	return active_router->delete_voice(profile, filename);
}

/**
 * \brief Free router info structure
 * \param info router_info structure
 * \return TRUE if structure is freed, otherwise FALSE
 */
gboolean rm_router_info_free(RmRouterInfo *info)
{
	if (info) {
		g_free(info->name);
		g_free(info->serial);
		g_free(info->version);
		g_free(info->lang);
		g_free(info->annex);

		g_free(info->host);
		g_free(info->user);
		g_free(info->password);
		g_free(info->session_id);

		if (info->session_timer) {
			g_timer_destroy(info->session_timer);
		}

		g_slice_free(RmRouterInfo, info);

		return TRUE;
	}

	return FALSE;
}

/**
 * \brief Check if router is using cable as annex
 * \param profile profile structure
 * \return TRUE if cable is used, otherwise FALSE
 */
gboolean rm_router_is_cable(RmProfile *profile)
{
	gboolean is_cable = FALSE;

	if (!RM_EMPTY_STRING(profile->router_info->annex) && !strcmp(profile->router_info->annex, "Kabel")) {
		is_cable = TRUE;
	}

	return is_cable;
}

/**
 * \brief Load fax reports and add them to the journal
 * \param profile profile structure
 * \param journal journal list pointer
 * \return new journal list with attached fax reports
 */
GSList *rm_router_load_fax_reports(RmProfile *profile, GSList *journal)
{
	GDir *dir;
	GError *error = NULL;
	const gchar *file_name;
	gchar *dir_name = g_settings_get_string(profile->settings, "fax-report-dir");

	if (!dir_name) {
		return journal;
	}

	dir = g_dir_open(dir_name, 0, &error);
	if (!dir) {
		g_debug("Could not open fax report directory");
		return journal;
	}

	while ((file_name = g_dir_read_name(dir))) {
		RmCallEntry *call;
		gchar *uri;
		gchar **split;
		gchar *date_time;

		if (strncmp(file_name, "fax-report", 10)) {
			continue;
		}

		split = g_strsplit(file_name, "_", -1);
		if (g_strv_length(split) != 9) {
			g_strfreev(split);
			continue;
		}

		uri = g_build_filename(dir_name, file_name, NULL);

		date_time = g_strdup_printf("%s.%s.%s %2.2s:%2.2s", split[3], split[4], split[5] + 2, split[6], split[7]);

		call = rm_call_entry_new(RM_CALL_ENTRY_TYPE_FAX_REPORT, date_time, "", split[2], ("Fax-Report"), split[1], "0:01", g_strdup(uri));
		journal = rm_journal_add_call_entry(journal, call);

		g_free(uri);
		g_strfreev(split);
	}

	return journal;
}

/**
 * \brief Load voice records and add them to the journal
 * \param profile profile structure
 * \param journal journal list pointer
 * \return new journal list with attached fax reports
 */
GSList *rm_router_load_voice_records(RmProfile *profile, GSList *journal)
{
	GDir *dir;
	GError *error = NULL;
	const gchar *file_name;
	const gchar *user_plugins = g_get_user_data_dir();
	gchar *dir_name = g_build_filename(user_plugins, RM_NAME, G_DIR_SEPARATOR_S, NULL);

	if (!dir_name) {
		return journal;
	}

	dir = g_dir_open(dir_name, 0, &error);
	if (!dir) {
		g_debug("Could not open voice records directory");
		return journal;
	}

	while ((file_name = g_dir_read_name(dir))) {
		RmCallEntry *call;
		gchar *uri;
		gchar **split;
		gchar *date_time;
		gchar *num;

		/* %2.2d.%2.2d.%4.4d-%2.2d-%2.2d-%s-%s.wav",
			time_val->tm_mday, time_val->tm_mon, 1900 + time_val->tm_year,
			time_val->tm_hour, time_val->tm_min, connection->source, connection->target);
		*/

		if (!strstr(file_name, ".wav")) {
			continue;
		}

		split = g_strsplit(file_name, "-", -1);
		if (g_strv_length(split) != 5) {
			g_strfreev(split);
			continue;
		}

		uri = g_build_filename(dir_name, file_name, NULL);
		num = split[4];
		num[strlen(num) - 4] = '\0';

		//date_time = g_strdup_printf("%s.%s.%s %2.2s:%2.2s", split[3], split[4], split[5] + 2, split[6], split[7]);
		date_time = g_strdup_printf("%s %2.2s:%2.2s", split[0], split[1], split[2]);

		call = rm_call_entry_new(RM_CALL_ENTRY_TYPE_RECORD, date_time, "", num, ("Record"), split[3], "0:01", g_strdup(uri));
		journal = rm_journal_add_call_entry(journal, call);

		g_free(uri);
		g_strfreev(split);
	}

	return journal;
}

/**
 * \brief Get number suppress state
 * \param profile router information structure
 * \return suppress state
 */
gboolean rm_router_get_suppress_state(RmProfile *profile)
{
	return g_settings_get_boolean(profile->settings, "suppress");
}

gboolean rm_router_need_ftp(RmProfile *profile)
{
	g_debug("%s(): called", __FUNCTION__);
	return active_router ? active_router->need_ftp(profile) : TRUE;
}
