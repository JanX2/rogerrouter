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


#include <libsecret/secret.h>

#include <rm/rmcallentry.h>
#include <rm/rmplugins.h>
#include <rm/rmpassword.h>
#include <rm/rmstring.h>

#include <secret.h>

/**
 * secret_get_schema:
 *
 * Get secret schema for this plugin
 *
 * Returns: a #SecretSchema
 */
const SecretSchema *secret_get_schema(void)
{
	static const SecretSchema the_schema = {
		"org.tabos.rm.Password", SECRET_SCHEMA_NONE,
		{
			{"profile", SECRET_SCHEMA_ATTRIBUTE_STRING},
			{"name", SECRET_SCHEMA_ATTRIBUTE_STRING},
			{"NULL", 0},
		}
	};

	return &the_schema;
}

/**
 * secret_store_password:
 * @profile: a #RmProfile
 * @name: password name
 * @password: password
 *
 * Store password
 */
static void secret_store_password(RmProfile *profile, const gchar *name, const gchar *password)
{
	GError *error = NULL;

	secret_password_store_sync(SECRET_SCHEMA, SECRET_COLLECTION_DEFAULT,
	                           "Router Manager password", password, NULL, &error,
	                           "profile", profile ? profile->name : "fallback",
	                           "name", name,
	                           NULL);

	if (error != NULL) {
		/* ... handle the failure here */
		g_debug("could not store password: %s", error->message);
		g_error_free(error);
	} else {
		/* ... do something now that the password has been stored */
	}
}

/**
 * secret_get_password:
 * @profile: a #RmProfile
 * @name: password name
 *
 * Get password
 *
 * Returns: password
 */
static gchar *secret_get_password(RmProfile *profile, const gchar *name)
{
	GError *error = NULL;
	gchar *password = secret_password_lookup_sync(SECRET_SCHEMA, NULL, &error,
	                  "profile", profile ? profile->name : "fallback",
	                  "name", name,
	                  NULL);

	if (error != NULL) {
		/* ... handle the failure here */
		g_debug("could not get password: %s", error->message);
		g_error_free(error);
	} else {
		/* ... do something now that the password has been stored */
	}

	return password;
}

/**
 * secret_remove_password:
 * @profile: a #RmProfile
 * @name: password name
 *
 * Remove password
 *
 * Returns: %TRUE on success, otherwise %FALSE
 */
static gboolean secret_remove_password(RmProfile *profile, const gchar *name)
{
	GError *error = NULL;
	gboolean removed = secret_password_clear_sync(SECRET_SCHEMA, NULL, &error,
	                   "profile", profile ? profile->name : "fallback",
	                   "name", name,
	                   NULL);

	if (error != NULL) {
		/* ... handle the failure here */
		g_debug("could not store password: %s", error->message);
		g_error_free(error);
	} else {
		/* ... do something now that the password has been stored */
	}

	return removed;
}

RmPasswordManager secret = {
	"GNOME Keyring (libsecret)",
	secret_store_password,
	secret_get_password,
	secret_remove_password,
};

/**
 * secret_available:
 *
 * Checks for secret based on desktop environment.
 *
 * Returns: %TRUE if it should be used, otherwise %FALSE
 */
gboolean secret_available(void)
{
	const gchar *xdg_desktop = g_environ_getenv(g_get_environ(), "XDG_CURRENT_DESKTOP");
	const gchar *desktop_session = g_environ_getenv(g_get_environ(), "DESKTOP_SESSION");
	gboolean ret = FALSE;

	if (!g_strcmp0(xdg_desktop, "GNOME")) {
		ret = TRUE;
	} else if (!g_strcmp0(xdg_desktop, "Unity")) {
		ret = TRUE;
	}

	if (!g_strcmp0(desktop_session, "gnome")) {
		ret = TRUE;
	} else if (!rm_strcasestr(desktop_session, "xfce")) {
		ret = TRUE;
	} else if (!rm_strcasestr(desktop_session, "xubuntu")) {
		ret = TRUE;
	}

	if (!ret) {
		g_debug("%s(): No supported desktop environment (%s/%s), secret will be disabled", __FUNCTION__, xdg_desktop, desktop_session);
	}

	return ret;
}

/**
 * impl_activate:
 * @plugin: a #PeasActivatable
 *
 * Activate peas plugin - register libsecret password manager if present
 */
gboolean secret_plugin_init(RmPlugin *plugin)
{
	if (!secret_available()) {
		return FALSE;
	}

	rm_password_register(&secret);

	return TRUE;
}

/**
 * impl_deactivate:
 * @plugin: a #PeasActivatable
 *
 * Deactivate peas plugin
 */
gboolean secret_plugin_shutdown(RmPlugin *plugin)
{
	return TRUE;
}

PLUGIN(secret);
