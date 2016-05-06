/**
 * The libroutermanager project
 * Copyright (c) 2012-2014 Jan-Michael Brummer
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

#include <libroutermanager/router.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/contact.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/file.h>
#include <libroutermanager/csv.h>
#include <libroutermanager/password.h>
#include <libroutermanager/routermanager.h>

/** Active router structure */
static struct router *active_router = NULL;
/** Global router plugin list */
static GSList *router_list = NULL;
/** Router login blocked shield */
static gboolean router_login_blocked = FALSE;

/** Mapping between config value and port type */
struct phone_port router_phone_ports[PORT_MAX] = {
	{"name-analog1", PORT_ANALOG1, -1},
	{"name-analog2", PORT_ANALOG2, -1},
	{"name-analog3", PORT_ANALOG3, -1},
	{"name-isdn1", PORT_ISDN1, -1},
	{"name-isdn2", PORT_ISDN2, -1},
	{"name-isdn3", PORT_ISDN3, -1},
	{"name-isdn4", PORT_ISDN4, -1},
	{"name-isdn5", PORT_ISDN5, -1},
	{"name-isdn6", PORT_ISDN6, -1},
	{"name-isdn7", PORT_ISDN7, -1},
	{"name-isdn8", PORT_ISDN8, -1},
	{"name-dect1", PORT_DECT1, -1},
	{"name-dect2", PORT_DECT2, -1},
	{"name-dect3", PORT_DECT3, -1},
	{"name-dect4", PORT_DECT4, -1},
	{"name-dect5", PORT_DECT5, -1},
	{"name-dect6", PORT_DECT6, -1},
	{"name-sip0", PORT_IP1, -1},
	{"name-sip1", PORT_IP2, -1},
	{"name-sip2", PORT_IP3, -1},
	{"name-sip3", PORT_IP4, -1},
	{"name-sip4", PORT_IP5, -1},
	{"name-sip5", PORT_IP6, -1},
	{"name-sip6", PORT_IP7, -1},
	{"name-sip7", PORT_IP8, -1},
	{"name-sip8", PORT_IP9, -1},
	{"name-sip9", PORT_IP10, -1}
};

/**
 * \brief Get list of phone names
 * \param profile profile structure
 * \return phone name list
 */
GSList *router_get_phone_list(struct profile *profile)
{
	GSList *list = NULL;
	gchar *fon;
	gint index;
	struct phone *phone;

	if (!profile) {
		return list;
	}

	phone = g_slice_new(struct phone);

	phone->name = g_strdup("Softphone");
	phone->type = PORT_SOFTPHONE;

	list = g_slist_append(list, phone);

	for (index = 0; index < PORT_MAX - 2; index++) {
		fon = g_settings_get_string(profile->settings, router_phone_ports[index].name);
		if (!EMPTY_STRING(fon)) {
			phone = g_slice_new(struct phone);

			phone->name = g_strdup(fon);
			phone->type = router_phone_ports[index].type;

			list = g_slist_append(list, phone);
		}
	}

	return list;
}

/**
 * \brief Free one phone list entry
 * \param data pointer to phone structure
 */
static void free_phone_list(gpointer data)
{
	struct phone *phone = data;

	g_free(phone->name);
}

/**
 * \brief Free full phone list
 * \param phone_list phone list
 */
void router_free_phone_list(GSList *phone_list)
{
	g_slist_free_full(phone_list, free_phone_list);
}

/**
 * \brief Get array of phone numbers
 * \param profile profile structure
 * \return phone number array
 */
gchar **router_get_numbers(struct profile *profile)
{
	return g_settings_get_strv(profile->settings, "numbers");
}

/**
 * \brief Check if router is present
 * \param router_info router information structure
 * \return present state
 */
gboolean router_present(struct router_info *router_info)
{
	GSList *list;

	g_debug("%s(): called", __FUNCTION__);
	if (!router_list) {
		return FALSE;
	}

	for (list = router_list; list != NULL; list = list->next) {
		struct router *router = list->data;

		if (router->present(router_info)) {
			active_router = router;
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * \brief Login to router
 * \param profile profile information structure
 * \return login state
 */
gboolean router_login(struct profile *profile)
{
	gboolean result;

	if (!active_router) {
		return FALSE;
	}

	if (router_login_blocked) {
		g_debug("%s(): called, but blocked", __FUNCTION__);
		return FALSE;
	}

	result = active_router->login(profile);
	if (!result) {
		g_warning(_("Login data are wrong or permissions are missing.\nPlease check your login data."));
		emit_message(_("Login failed"), _("Login data are wrong or permissions are missing.\nPlease check your login data."));
		router_login_blocked = TRUE;
	}

	return result;
}

/**
 * \brief Release router login blocked
 */
void router_release_lock(void)
{
	g_debug("%s(): called", __FUNCTION__);
	router_login_blocked = FALSE;
}

/**
 * \brief Check whether router login is blocked
 */
gboolean router_is_locked(void)
{
	g_debug("%s(): called", __FUNCTION__);
	return router_login_blocked;
}

/**
 * \brief Router logout
 * \param profile profile information structure
 * \return logout state
 */
gboolean router_logout(struct profile *profile)
{
	return active_router ? active_router->logout(profile, FALSE) : FALSE;
}

/**
 * \brief Get router host
 * \param profile profile information structure
 * \return router host or "" if no profile is active
 */
gchar *router_get_host(struct profile *profile)
{
	return profile ? g_settings_get_string(profile->settings, "host") : "";
}

/**
 * \brief Get login password
 * \param profile router information structure
 * \return login password
 */
gchar *router_get_login_password(struct profile *profile)
{
	return password_manager_get_password(profile, "login-password");
}

/**
 * \brief Get login user
 * \param profile router information structure
 * \return login user
 */
gchar *router_get_login_user(struct profile *profile)
{
	return g_settings_get_string(profile->settings, "login-user");
}

/**
 * \brief Get router FTP password
 * \param profile router information structure
 * \return router ftp password
 */
gchar *router_get_ftp_password(struct profile *profile)
{
	return password_manager_get_password(profile, "ftp-password");
}

/**
 * \brief Get router FTP user
 * \param profile router information structure
 * \return router ftp user
 */
gchar *router_get_ftp_user(struct profile *profile)
{
	return g_settings_get_string(profile->settings, "ftp-user");
}

/*
 * \brief Get international call prefix
 * \param profile router information structure
 * \return international call prefix
 */
gchar *router_get_international_prefix(struct profile *profile)
{
	if (!profile || !profile->settings) {
		return NULL;
	}

	return g_settings_get_string(profile->settings, "international-call-prefix");
}

/*
 * \brief Get national call prefix
 * \param profile router information structure
 * \return national call prefix
 */
gchar *router_get_national_prefix(struct profile *profile)
{
	return g_settings_get_string(profile->settings, "national-call-prefix");
}

/*
 * \brief Get router area code
 * \param profile router information structure
 * \return area code
 */
gchar *router_get_area_code(struct profile *profile)
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
gchar *router_get_country_code(struct profile *profile)
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
gboolean router_get_settings(struct profile *profile)
{
	return active_router ? active_router->get_settings(profile) : FALSE;
}

/**
 * \brief Get router name
 * \param profile profile information structure
 * \return router name
 */
const gchar *router_get_name(struct profile *profile)
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
const gchar *router_get_version(struct profile *profile)
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
gboolean router_load_journal(struct profile *profile)
{
	return active_router ? active_router->load_journal(profile, NULL) : FALSE;
}
/**
 * \brief Clear router journal
 * \param profile profile information structure
 * \return clear journal return state
 */
gboolean router_clear_journal(struct profile *profile)
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
gboolean router_dial_number(struct profile *profile, gint port, const gchar *number)
{
	gchar *target = call_canonize_number(number);
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
gboolean router_hangup(struct profile *profile, gint port, const gchar *number)
{
	return active_router->hangup(profile, port, number);
}

/**
 * \brief Register new router
 * \param router new router structure
 */
gboolean routermanager_router_register(struct router *router)
{
	router_list = g_slist_prepend(router_list, router);

	return TRUE;
}

/**
 * \brief Initialize router (if available set internal router structure)
 * \return TRUE on success, otherwise FALSE
 */
gboolean router_init(void)
{
	if (g_slist_length(router_list)) {
		return TRUE;
	}

	g_warning("No router plugin registered!");
	return FALSE;
}

/**
 * \brief Shutdown router
 * \return TRUE
 */
void router_shutdown(void)
{
	/* Free router list */
	if (router_list != NULL) {
		g_slist_free(router_list);
		router_list = NULL;
	}

	/* Unset active router */
	active_router = NULL;
}

/**
 * \brief Router needs to process a new loaded journal (emit journal-process signal and journal-loaded)
 * \param journal journal list
 */
void router_process_journal(GSList *journal)
{
	GSList *list;

	/* Parse offline journal and combine new entries */
	journal = csv_load_journal(journal);

	/* Store it back to disk */
	csv_save_journal(journal);

	/* Try to lookup entries in address book */
	for (list = journal; list; list = list->next) {
		struct call *call = list->data;

		emit_contact_process(call->remote);
	}

	/* Emit "journal-loaded" signal */
	emit_journal_loaded(journal);

}

/**
 * \brief Load fax file
 * \param profile profile structure
 * \param filename fax filename
 * \param len pointer to store data length to
 * \return fax data
 */
gchar *router_load_fax(struct profile *profile, const gchar *filename, gsize *len)
{
	return filename[0] == '/' ? file_load((gchar*)filename, len) : active_router->load_fax(profile, filename, len);
}

/**
 * \brief Load voice file
 * \param profile profile structure
 * \param name voice filename
 * \param len pointer to store data length to
 * \return voice data
 */
gchar *router_load_voice(struct profile *profile, const gchar *name, gsize *len)
{
	return active_router->load_voice(profile, name, len);
}

/**
 * \brief Get external IP address
 * \param profile profile structure
 * \return IP address
 */
gchar *router_get_ip(struct profile *profile)
{
	return active_router->get_ip(profile);
}

/**
 * \brief Reconnect network connection
 * \param profile profile structure
 * \param TRUE on success, otherwise FALSE
 */
gboolean router_reconnect(struct profile *profile)
{
	return active_router->reconnect(profile);
}

/**
 * \brief Delete fax file on router
 * \param profile profile structure
 * \param filename fax filename to delete
 * \param TRUE on success, otherwise FALSE
 */
gboolean router_delete_fax(struct profile *profile, const gchar *filename)
{
	return active_router->delete_fax(profile, filename);
}

/**
 * \brief Delete voice file on router
 * \param profile profile structure
 * \param filename voice filename to delete
 * \param TRUE on success, otherwise FALSE
 */
gboolean router_delete_voice(struct profile *profile, const gchar *filename)
{
	return active_router->delete_voice(profile, filename);
}

/**
 * \brief Free router info structure
 * \param info router_info structure
 * \return TRUE if structure is freed, otherwise FALSE
 */
gboolean router_info_free(struct router_info *info)
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

		g_slice_free(struct router_info, info);

		return TRUE;
	}

	return FALSE;
}

/**
 * \brief Check if router is using cable as annex
 * \param profile profile structure
 * \return TRUE if cable is used, otherwise FALSE
 */
gboolean router_is_cable(struct profile *profile)
{
	gboolean is_cable = FALSE;

	if (!EMPTY_STRING(profile->router_info->annex) && !strcmp(profile->router_info->annex, "Kabel")) {
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
GSList *router_load_fax_reports(struct profile *profile, GSList *journal)
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
		journal = call_add(journal, CALL_TYPE_FAX_REPORT, date_time, "", split[2], ("Fax-Report"), split[1], "0:01", g_strdup(uri));

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
GSList *router_load_voice_records(struct profile *profile, GSList *journal)
{
	GDir *dir;
	GError *error = NULL;
	const gchar *file_name;
	const gchar *user_plugins = g_get_user_data_dir();
	gchar *dir_name = g_build_filename(user_plugins, "roger", G_DIR_SEPARATOR_S, NULL);

	if (!dir_name) {
		return journal;
	}

	dir = g_dir_open(dir_name, 0, &error);
	if (!dir) {
		g_debug("Could not open voice records directory");
		return journal;
	}

	while ((file_name = g_dir_read_name(dir))) {
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
		journal = call_add(journal, CALL_TYPE_RECORD, date_time, "", num, ("Record"), split[3], "0:01", g_strdup(uri));

		g_free(uri);
		g_strfreev(split);
	}

	return journal;
}

/**
 * \brief Get phone port
 * \param profile router information structure
 * \return phone port
 */
gint router_get_phone_port(struct profile *profile)
{
	return g_settings_get_int(profile->settings, "port");
}

/**
 * \brief Set phone port
 * \param profile router information structure
 * \param port phone port
 */
void router_set_phone_port(struct profile *profile, gint port)
{
	g_settings_set_int(profile->settings, "port", port);
}

/**
 * \brief Get number suppress state
 * \param profile router information structure
 * \return suppress state
 */
gboolean router_get_suppress_state(struct profile *profile)
{
	return g_settings_get_boolean(profile->settings, "suppress");
}
