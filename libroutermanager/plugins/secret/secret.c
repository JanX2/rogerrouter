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

#include <libsecret/secret.h>

#include <libroutermanager/call.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/password.h>

#include <secret.h>

#define ROUTERMANAGER_TYPE_SECRET_PLUGIN        (routermanager_secret_plugin_get_type ())
#define ROUTERMANAGER_SECRET_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_SECRET_PLUGIN, RouterManagerSecretPlugin))

typedef struct {
	guint signal_id;
} RouterManagerSecretPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_SECRET_PLUGIN, RouterManagerSecretPlugin, routermanager_secret_plugin)

/**
 * \brief Get secret schema for this plugin
 * \return secret schema layout
 */
const SecretSchema *secret_get_schema(void)
{
	static const SecretSchema the_schema = {
		"org.tabos.roger.Password", SECRET_SCHEMA_NONE,
		{
			{"profile", SECRET_SCHEMA_ATTRIBUTE_STRING},
			{"name", SECRET_SCHEMA_ATTRIBUTE_STRING},
			{"NULL", 0},
		}
	};

	return &the_schema;
}

/**
 * \brief Store password
 * \param profile profile pointer
 * \param name password name
 * \param password password
 */
static void secret_store_password(struct profile *profile, const gchar *name, const gchar *password)
{
	GError *error = NULL;

	secret_password_store_sync(SECRET_SCHEMA, SECRET_COLLECTION_DEFAULT,
	                           "Roger Router password", password, NULL, &error,
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
 * \brief Get password
 * \param profile profile pointer
 * \param name password name
 * \return password
 */
static gchar *secret_get_password(struct profile *profile, const gchar *name)
{
	GError *error = NULL;
	gchar *password = secret_password_lookup_sync(SECRET_SCHEMA, NULL, &error,
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

	return password;
}

/**
 * \brief Remove password
 * \param profile profile pointer
 * \param name password name
 * \return TRUE on success, otherwise FALSE
 */
static gboolean secret_remove_password(struct profile *profile, const gchar *name)
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

struct password_manager secret = {
	"libsecret",
	secret_store_password,
	secret_get_password,
	secret_remove_password,
};

/**
 * \brief Activate peas plugin - register libsecret password manager if present
 * \param plugin peas plugin
 */
void impl_activate(PeasActivatable *plugin)
{
	g_debug("Register libsecret password manager plugin");
	password_manager_register(&secret);

#ifdef SECRET_TEST
	g_debug("Starting secret\n");

	secret_store_password(profile_get_active(), "Test", "testpassword");

	g_debug("Got test password: '%s'", secret_get_password(profile_get_active(), "Test"));
	secret_remove_password(profile_get_active(), "Test");
#endif
}

/**
 * \brief Deactivate peas plugin
 * \param plugin peas plugin
 */
void impl_deactivate(PeasActivatable *plugin)
{
}
