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

#include <gio/gio.h>

#include <libroutermanager/action.h>
#include <libroutermanager/fax_phone.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/router.h>
#include <libroutermanager/net_monitor.h>
#include <libroutermanager/password.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/audio.h>

/** Internal profile list */
static GSList *profile_list = NULL;
/** Internal pointer to active profile */
static struct profile *profile_active = NULL;
/** Global application settings */
static GSettings *settings = NULL;

/**
 * \brief Add profile to profile list
 * \param profile profile structure pointer
 */
void profile_add(struct profile *profile)
{
	profile_list = g_slist_prepend(profile_list, profile);
}

/**
 * \brief Remove profile from list
 * \param profile profile to remove
 */
void profile_remove(struct profile *profile)
{
	profile_list = g_slist_remove(profile_list, profile);

	/* Remove gsettings entry */
	profile_commit();
}

/**
 * \brief Get active profile
 * \return active profile
 */
struct profile *profile_get_active(void)
{
	return profile_active;
}

/**
 * \brief Get profile list
 * \return profile list
 */
GSList *profile_get_list(void)
{
	return profile_list;
}

/**
 * \brief Create new profile structure
 * \param name profile name
 * \param host router host
 * \param password router password
 * \return new profile structure poiner
 */
struct profile *profile_new(const gchar *name, const gchar *host, const gchar *user, const gchar *password)
{
	struct profile *profile;
	gchar *settings_path;

	/* Create new profile structure */
	profile = g_slice_new0(struct profile);

	/* Add entries */
	profile->name = g_strdup(name);
	profile->router_info = g_slice_new0(struct router_info);
	profile->router_info->host = g_strdup(host);
	profile->router_info->user = g_strdup(user);
	profile->router_info->password = g_strdup(password);

	/* Setup profiles settings */
	settings_path = g_strconcat("/org/tabos/routermanager/profile/", name, "/", NULL);
	profile->settings = g_settings_new_with_path(ROUTERMANAGER_SCHEME_PROFILE, settings_path);

	g_settings_set_string(profile->settings, "host", host);
	g_settings_set_string(profile->settings, "login-user", user);
	password_manager_set_password(profile, "login-password", password);

	g_free(settings_path);

	/* Return new profile */
	return profile;
}

/**
 * \brief Update profiles in gsettting
 */
void profile_commit(void)
{
	GSList *plist = profile_get_list();
	GSList *list;
	gchar **profiles;
	gint counter = 0;

	profiles = g_malloc(sizeof(gchar *) * (g_slist_length(plist) + 1));
	for (list = plist; list != NULL; list = list->next) {
		struct profile *current_profile = list->data;
		profiles[counter++] = g_strdup(current_profile->name);
	}
	profiles[counter] = NULL;

	g_settings_set_strv(settings, "profiles", (const gchar * const *)profiles);
}

/**
 * \brief Load profile by name
 * \param name profile name to load
 */
static void profile_load(const gchar *name)
{
	struct profile *profile;
	gchar *settings_path;

	profile = g_slice_new0(struct profile);

	profile->name = g_strdup(name);

	settings_path = g_strconcat("/org/tabos/routermanager/profile/", name, "/", NULL);
	profile->settings = g_settings_new_with_path(ROUTERMANAGER_SCHEME_PROFILE, settings_path);

	profile->router_info = g_slice_new0(struct router_info);
	profile->router_info->host = g_settings_get_string(profile->settings, "host");
	profile->router_info->user = g_settings_get_string(profile->settings, "login-user");
	profile->router_info->password = password_manager_get_password(profile, "login-password");

	g_free(settings_path);

	profile_add(profile);
}

/**
 * \brief Set active profile
 * \param profile profile structure
 *
 * Set active profile in detail:
 *  * Set internal active profile
 *  * Connect user plugin bindings
 *  * Load and initialize action
 *  * faxophone setup
 *  * Load journal
 */
void profile_set_active(struct profile *profile)
{
	if (profile_active) {
		action_shutdown(profile_active);
	}

	profile_active = profile;

	/* If we have no active profile, exit */
	if (!profile_active) {
		/* Clear journal list */
		emit_journal_loaded(NULL);
		return;
	}

	router_present(profile->router_info);

	plugins_user_plugins();

	/* Init audio */
	audio_init(profile);

	/* Load and initialize action */
	action_init(profile);

	/* Init faxophone */
	faxophone_setup();

	/* Load journal list */
	router_load_journal(profile_active);
}

/**
 * \brief Detect active profile (scan available routers and select correct profile)
 * \return TRUE on success, otherwise FALSE
 */
gboolean profile_detect_active(void)
{
	GSList *list = profile_get_list();
	struct profile *profile;
	struct router_info *router_info;

	/* Compare our profile list against available routers */
	while (list) {
		profile = list->data;

		router_info = g_slice_new0(struct router_info);
		router_info->host = g_settings_get_string(profile->settings, "host");

		/* Check if router is present */
		if (router_present(router_info) == TRUE && !strcmp(router_info->serial, g_settings_get_string(profile->settings, "serial"))) {
			g_debug("Configured router found: %s", router_info->name);
			g_debug("Current router firmware: %d.%d.%d", router_info->box_id, router_info->maj_ver_id, router_info->min_ver_id);

			router_info_free(router_info);

			profile_set_active(profile);
			return TRUE;
		}

		router_info_free(router_info);

		list = list->next;
	}

	g_debug("No already configured router found!");

	return FALSE;
}

/**
 * \brief Initialize profiles (load profiles)
 * \return TRUE
 */
gboolean profile_init(void)
{
	settings = g_settings_new(ROUTERMANAGER_SCHEME);
	gchar **profiles = g_settings_get_strv(settings, "profiles");
	gint count;

	/* Load all available profiles */
	for (count = 0; count < g_strv_length(profiles); count++) {
		profile_load(profiles[count]);
	}

	g_strfreev(profiles);

	return TRUE;
}

/**
 * \brief Free profile entry
 * \param data pointer to profile structure
 */
static void profile_free_entry(gpointer data)
{
	struct profile *profile = data;

	/* Free entries */
	g_free(profile->name);

	/* Free router information structure */
	router_info_free(profile->router_info);

	/* Free actions */
	action_shutdown(profile);

	/* Free profiles settings */
	g_clear_object(&profile->settings);

	/* Free structure */
	g_slice_free(struct profile, profile);
}

/**
 * \brief Shutdown profile (free profile list, free settings)
 */
void profile_shutdown(void)
{
	/* Set active profile to NULL */
	profile_active = NULL;

	if (profile_list) {
		/* Free profile list */
		g_slist_free_full(profile_list, profile_free_entry);
	}

	/* Clear settings object */
	if (settings) {
		g_clear_object(&settings);
	}
}
