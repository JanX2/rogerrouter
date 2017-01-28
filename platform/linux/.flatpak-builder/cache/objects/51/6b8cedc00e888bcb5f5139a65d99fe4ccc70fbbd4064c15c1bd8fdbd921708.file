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

#include <libsoup/soup.h>

#include <string.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/network.h>
#include <libroutermanager/appobject-emit.h>

/** Soup session */
SoupSession *soup_session = NULL;

static void free_auth_data(struct auth_data *auth_data)
{
	g_object_unref(auth_data->msg);
	g_free(auth_data->username);
	g_free(auth_data->password);

	g_slice_free(struct auth_data, auth_data);
}

static void save_password_callback(SoupMessage* msg, struct auth_data *auth_data)
{
	if (msg->status_code != 401 && msg->status_code < 500) {
		struct profile *profile = profile_get_active();

		g_settings_set_string(profile->settings, "auth-user", auth_data->username);
		g_settings_set_string(profile->settings, "auth-password", auth_data->password);

		g_debug("%s(): Storing data for later processing", __FUNCTION__);
	}

	g_signal_handlers_disconnect_by_func(msg, save_password_callback, auth_data);

	free_auth_data(auth_data);
}

void network_authenticate(gboolean auth_set, struct auth_data *auth_data)
{
	g_debug("%s(): calling authenticate", __FUNCTION__);

	if (auth_set) {
		soup_auth_authenticate(auth_data->auth, auth_data->username, auth_data->password);
		g_signal_connect(auth_data->msg, "got-headers", G_CALLBACK(save_password_callback), auth_data);
	}

	soup_session_unpause_message(auth_data->session, auth_data->msg);
	if (!auth_set) {
		free_auth_data(auth_data);
	}
}

static void network_authenticate_cb(SoupSession *session, SoupMessage *msg, SoupAuth *auth, gboolean retrying, gpointer user_data)
{
	struct auth_data *auth_data;
	struct profile *profile = profile_get_active();
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

	if (!retrying && !EMPTY_STRING(user) && !EMPTY_STRING(password)) {
		g_debug("%s(): Already configured...", __FUNCTION__);
		soup_auth_authenticate(auth, user, password);

		soup_session_unpause_message(session, msg);
	} else {
		auth_data = g_slice_new0(struct auth_data);

		auth_data->msg = msg;
		auth_data->auth = auth;
		auth_data->session = session;
		auth_data->retry = retrying;
		auth_data->username = g_strdup(user);
		auth_data->password = g_strdup(password);

		emit_authenticate(auth_data);
	}
}

/**
 * \brief Initialize network functions
 * \return TRUE on success, otherwise FALSE
 */
gboolean net_init(void)
{
	soup_session = soup_session_new_with_options(SOUP_SESSION_TIMEOUT, 5, NULL);

	g_signal_connect(soup_session, "authenticate", G_CALLBACK(network_authenticate_cb), soup_session);

	return soup_session != NULL;
}

/**
 * \brief Deinitialize network infrastructure
 */
void net_shutdown(void)
{
	g_clear_object(&soup_session);
}
