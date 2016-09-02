/**
 * The libroutermanager project
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#include <libroutermanager/rmaction.h>
//#include <libroutermanager/fax_phone.h>
#include <libroutermanager/rmprofile.h>
#include <libroutermanager/rmplugins.h>
#include <libroutermanager/rmmain.h>
#include <libroutermanager/router.h>
#include <libroutermanager/rmnetmonitor.h>
#include <libroutermanager/rmpassword.h>
#include <libroutermanager/rmobjectemit.h>
#include <libroutermanager/rmaudio.h>
#include <libroutermanager/rmsettings.h>

/**
 * SECTION:rmprofile
 * @Title: RmProfile
 * @Short_description: Profile handling functions
 *
 * Profiles are used to distinguish between several routers. This is useful for mobile devices using private and business environements.
 */

/** Internal profile list */
static GSList *rm_profile_list = NULL;
/** Internal pointer to active profile */
static struct profile *rm_profile_active = NULL;
/** Global application settings */
static GSettings *rm_settings = NULL;

/**
 * rm_profile_get_name:
 * @profile: a #RmProfile
 *
 * Get profiles name of @profile.
 *
 * Returns: profile name
 */
const gchar *rm_profile_get_name(RmProfile *profile)
{
	return profile->name;
}

/**
 * rm_profile_save:
 *
 * Update profiles in gsettting.
 */
static void rm_profile_save(void)
{
	GSList *plist = rm_profile_get_list();
	GSList *list;
	gchar **profiles;
	gint counter = 0;

	profiles = g_new0(gchar *, (g_slist_length(plist) + 1));
	for (list = plist; list != NULL; list = list->next) {
		RmProfile *current_profile = list->data;

		profiles[counter++] = g_strdup(rm_profile_get_name(current_profile));
	}
	profiles[counter] = NULL;

	g_settings_set_strv(rm_settings, "profiles", (const gchar * const *)profiles);

	g_strfreev(profiles);
}

/**
 * rm_profile_remove:
 * @profile: a #RmProfile
 *
 * Remove @profile from list.
 */
void rm_profile_remove(RmProfile *profile)
{
	rm_profile_list = g_slist_remove(rm_profile_list, profile);

	g_object_unref(profile->settings);

	router_info_free(profile->router_info);

	g_free(profile->name);

	/* Remove gsettings entry */
	rm_profile_save();
}

/**
 * rm_profile_get_active:
 *
 * Get active profile.
 *
 * Returns: active #RmProfile
 */
RmProfile *rm_profile_get_active(void)
{
	return rm_profile_active;
}

/**
 * rm_profile_get_list:
 *
 * Get profile list
 *
 * Returns: profile list
 */
GSList *rm_profile_get_list(void)
{
	return rm_profile_list;
}

/**
 * rm_profile_add:
 * @name: profile name
 *
 * Create and add a new profile structure.
 *
 * Returns: new #RmProfile
 */
RmProfile *rm_profile_add(const gchar *name)
{
	RmProfile *profile;
	gchar *settings_path;

	/* Create new profile structure */
	profile = g_slice_new0(RmProfile);

	/* Add entries */
	profile->name = g_strdup(name);
	profile->router_info = g_slice_new0(struct router_info);

	/* Setup profiles settings */
	settings_path = g_strconcat("/org/tabos/routermanager/profile/", name, "/", NULL);
	profile->settings = rm_settings_new_with_path(RM_SCHEME_PROFILE, settings_path);

	g_free(settings_path);

	rm_profile_list = g_slist_prepend(rm_profile_list, profile);

	rm_profile_save();

	/* Return new profile */
	return profile;
}

/**
 * rm_profile_load:
 * @name: profile name to load
 *
 * Load profile by name.
 */
static void rm_profile_load(const gchar *name)
{
	RmProfile *profile;
	gchar *settings_path;

	profile = g_slice_new0(RmProfile);

	profile->name = g_strdup(name);

	settings_path = g_strconcat("/org/tabos/routermanager/profile/", name, "/", NULL);
	profile->settings = rm_settings_new_with_path(RM_SCHEME_PROFILE, settings_path);

	profile->router_info = g_slice_new0(struct router_info);
	profile->router_info->host = g_settings_get_string(profile->settings, "host");
	profile->router_info->user = g_settings_get_string(profile->settings, "login-user");
	profile->router_info->password = rm_password_get(profile, "login-password");

	g_free(settings_path);

	rm_profile_list = g_slist_prepend(rm_profile_list, profile);
}

/**
 * rm_profile_set_active:
 * @profile: a #RmProfile
 *
 * Set active profile in detail:
 *  * Set internal active profile
 *  * Connect user plugin bindings
 *  * Initialize audio
 *  * Load and initialize action
 *  * Load journal
 */
void rm_profile_set_active(RmProfile *profile)
{
	/* Shut profile actions down */
	rm_action_shutdown(rm_profile_active);

	rm_profile_active = profile;

	/* If we have no active profile, exit */
	if (!rm_profile_active) {
		/* Clear journal list */
		rm_object_emit_journal_loaded(NULL);
		return;
	}

	router_present(profile->router_info);

	rm_plugins_bind_loaded_plugins();

	/* Init audio */
	rm_audio_init(profile);

	/* Load and initialize action */
	rm_action_init(profile);

	/* Load journal list */
	router_load_journal(rm_profile_active);
}

/**
 * rm_profile_detect_active:
 *
 * Detect active profile (scan available routers and select correct profile).
 *
 * Returns: TRUE on success, otherwise FALSE
 */
gboolean rm_profile_detect_active(void)
{
	GSList *list = rm_profile_get_list();
	RmProfile *profile;
	struct router_info *router_info;
	gchar *requested_profile = rm_get_requested_profile();

	/* Compare our profile list against available routers */
	while (list) {
		profile = list->data;

		/* Check whether user specified a profile */
		if (requested_profile && strcmp(profile->name, requested_profile)) {
			list = list->next;
			continue;
		}

		router_info = g_slice_new0(struct router_info);
		router_info->host = g_settings_get_string(profile->settings, "host");

		/* Check if router is present */
		if (router_present(router_info) == TRUE && !strcmp(router_info->serial, g_settings_get_string(profile->settings, "serial"))) {
			g_debug("Configured router found: %s", router_info->name);
			g_debug("Current router firmware: %d.%d.%d", router_info->box_id, router_info->maj_ver_id, router_info->min_ver_id);

			router_info_free(router_info);

			rm_profile_set_active(profile);
			return TRUE;
		}

		router_info_free(router_info);

		list = list->next;
	}

	g_debug("No already configured router found!");

	return FALSE;
}

/**
 * rm_profile_init:
 *
 * Initialize profiles (load profiles).
 * Returns: TRUE
 */
gboolean rm_profile_init(void)
{
	rm_settings = rm_settings_new(RM_SCHEME);

	gchar **profiles = g_settings_get_strv(rm_settings, "profiles");
	gint count;

	/* Load all available profiles */
	for (count = 0; count < g_strv_length(profiles); count++) {
		rm_profile_load(profiles[count]);
	}

	g_strfreev(profiles);

	return TRUE;
}

/**
 * rm_profile_free_entry:
 * @data: pointer to a #RmProfile
 *
 * Free profile entry.
 */
static void rm_profile_free_entry(gpointer data)
{
	RmProfile *profile = data;

	/* Free entries */
	g_free(profile->name);

	/* Free router information structure */
	router_info_free(profile->router_info);

	/* Free actions */
	rm_action_shutdown(profile);

	/* Free profiles settings */
	g_clear_object(&profile->settings);

	/* Free structure */
	g_slice_free(struct profile, profile);
}

/**
 * rm_profile_shutdown:
 *
 * Shutdown profile (free profile list, free settings).
 */
void rm_profile_shutdown(void)
{
	/* Set active profile to NULL */
	rm_profile_active = NULL;

	if (rm_profile_list) {
		/* Free profile list */
		g_slist_free_full(rm_profile_list, rm_profile_free_entry);
	}

	/* Clear settings object */
	if (rm_settings) {
		g_clear_object(&rm_settings);
	}
}

/**
 * rm_profile_set_host:
 * @profile: a #RmProfile
 * @host: new host name for @profile
 *
 * Set host name used in profile.
 */
void rm_profile_set_host(RmProfile *profile, const gchar *host)
{
	if (profile->router_info->host) {
		g_free(profile->router_info->host);
	}

	profile->router_info->host = g_strdup(host);
	g_settings_set_string(profile->settings, "host", host);
}

/**
 * rm_profile_set_login_user:
 * @profile: a #RmProfile
 * @user: new user name for @profile
*
 * Set login user used in profile.
 */
void rm_profile_set_login_user(RmProfile *profile, const gchar *user)
{
	g_settings_set_string(profile->settings, "login-user", user);
}

/**
 * rm_profile_set_login_password:
 * @profile: a #RmProfile
 * @password: new password for @profile
*
 * Set login password used in profile.
 */
void rm_profile_set_login_password(RmProfile *profile, const gchar *password)
{
	rm_password_set(profile, "login-password", password);
}
