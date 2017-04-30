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

#include <libsoup/soup.h>

#include <string.h>
#include <rm/rmstring.h>
#include <rm/rmnetwork.h>
#include <rm/rmobjectemit.h>
#include <rm/rmprofile.h>
#include <rm/rmrouter.h>
#include <rm/rmmain.h>

/**
 * SECTION:rmnetwork
 * @Title: RmNetwork
 * @Short_description: Network - Handle network access
 *
 * A network wrapper with authentication support
 */

/** Soup session */
SoupSession *rm_soup_session = NULL;

/**
 * rm_network_free_auth_data:
 * @auth_data: a #RmAuthData
 *
 * Releases and frees authentication data.
 */
static void rm_network_free_auth_data(RmAuthData *auth_data)
{
	g_object_unref(auth_data->msg);

	g_free(auth_data->username);
	g_free(auth_data->password);

	g_slice_free(struct auth_data, auth_data);
}

/**
 * rm_network_save_password_cb:
 * @msg: a #SoupMessage
 * @auth_data: a #RmAuthData
 *
 * Saves authentication credentials if network transfer was successful.
 */
static void rm_network_save_password_cb(SoupMessage* msg, RmAuthData *auth_data)
{
	if (msg->status_code != 401 && msg->status_code < 500) {
		RmProfile *profile = rm_profile_get_active();

		g_settings_set_string(profile->settings, "auth-user", auth_data->username);
		g_settings_set_string(profile->settings, "auth-password", auth_data->password);

		g_debug("%s(): Storing data for later processing", __FUNCTION__);
	}

	g_debug("%s(): MOEP", __FUNCTION__);
	g_signal_handlers_disconnect_by_func(msg, rm_network_save_password_cb, auth_data);

	rm_network_free_auth_data(auth_data);
}

/**
 * rm_network_authenticate:
 * @auth_set: indicated whether authtentification data has been set
 * @auth_data: a #RmAuthData
 */
void rm_network_authenticate(gboolean auth_set, RmAuthData *auth_data)
{
	g_debug("%s(): calling authenticate", __FUNCTION__);

	if (auth_set) {
		soup_auth_authenticate(auth_data->auth, auth_data->username, auth_data->password);
		g_signal_connect(auth_data->msg, "got-headers", G_CALLBACK(rm_network_save_password_cb), auth_data);
	}

	soup_session_unpause_message(auth_data->session, auth_data->msg);
	if (!auth_set) {
		rm_network_free_auth_data(auth_data);
	}
}

/**
 * rm_network_authenticate_cb:
 * @sesion: a #SoupSession
 * @msg: a #SoupMessage
 * @auth: a #SoupAuth
 * @retrying: flag indicating if method is called again
 * @user_data: user data
 *
 * A network authentication is required. Retrieve information from settings or ask user.
 */
static void rm_network_authenticate_cb(SoupSession *session, SoupMessage *msg, SoupAuth *auth, gboolean retrying, gpointer user_data)
{
	RmAuthData *auth_data;
	RmProfile *profile = rm_profile_get_active();
	const gchar *user;
	const gchar *password;

	g_debug("%s(): retrying: %d, status code: %d == %d", __FUNCTION__, retrying, msg->status_code, SOUP_STATUS_UNAUTHORIZED);
	if (msg->status_code != SOUP_STATUS_UNAUTHORIZED) {
		return;
	}

	g_debug("%s(): called with profile %p", __FUNCTION__, profile);
	if (!profile) {
		return;
	}

	soup_session_pause_message(session, msg);
	/* We need to make sure the message sticks around when pausing it */
	g_object_ref(msg);

	user = g_settings_get_string(profile->settings, "auth-user");
	password = g_settings_get_string(profile->settings, "auth-password");

	if (RM_EMPTY_STRING(user) && RM_EMPTY_STRING(password)) {
		user = rm_router_get_login_user(profile);
		g_settings_set_string(profile->settings, "auth-user", user);

		password = rm_router_get_login_password(profile);
		g_settings_set_string(profile->settings, "auth-password", password);
	}

	if (!retrying && !RM_EMPTY_STRING(user) && !RM_EMPTY_STRING(password)) {
		g_debug("%s(): Already configured...", __FUNCTION__);
		soup_auth_authenticate(auth, user, password);

		soup_session_unpause_message(session, msg);
	} else {
		auth_data = g_slice_new0(RmAuthData);

		auth_data->msg = msg;
		auth_data->auth = auth;
		auth_data->session = session;
		auth_data->retry = retrying;
		auth_data->username = g_strdup(user);
		auth_data->password = g_strdup(password);

		rm_object_emit_authenticate(auth_data);
	}
}

/**
 * rm_network_init:
 *
 * Initialize network functions.
 *
 * Returns: TRUE on success, otherwise FALSE
 */
gboolean rm_network_init(void)
{
	SoupCache *cache;

	/* NULL for directory is not sufficient on Windows platform, therefore set user cache dir */
	cache = soup_cache_new(rm_get_user_cache_dir(), SOUP_CACHE_SINGLE_USER);
	rm_soup_session = soup_session_new_with_options(SOUP_SESSION_TIMEOUT, 5, SOUP_SESSION_USE_THREAD_CONTEXT, TRUE, SOUP_SESSION_ADD_FEATURE, cache, SOUP_SESSION_SSL_STRICT, FALSE, NULL);
	soup_cache_load(cache);

	g_signal_connect(rm_soup_session, "authenticate", G_CALLBACK(rm_network_authenticate_cb), rm_soup_session);

	return rm_soup_session != NULL;
}

/**
 * rm_network_shutdown:
 *
 * Shutdown network infrastructure.
 */
void rm_network_shutdown(void)
{
	g_clear_object(&rm_soup_session);
}
