/**
 * Roger Router
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

#include <gtk/gtk.h>

#include <libsecret/secret.h>

#include <libcallibre/call.h>
#include <libcallibre/plugins.h>
#include <libcallibre/password.h>

#include <secret.h>

#define CALLIBRE_TYPE_SECRET_PLUGIN        (callibre_secret_plugin_get_type ())
#define CALLIBRE_SECRET_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), CALLIBRE_TYPE_SECRET_PLUGIN, CallibreSecretPlugin))

typedef struct {
        guint signal_id;
} CallibreSecretPluginPrivate;

CALLIBRE_PLUGIN_REGISTER(CALLIBRE_TYPE_SECRET_PLUGIN, CallibreSecretPlugin, callibre_secret_plugin)

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

void impl_activate(PeasActivatable *plugin)
{
	CallibreSecretPlugin *secret_plugin = CALLIBRE_SECRET_PLUGIN(plugin);

	if (secret_service_get_sync(1 << 1, NULL, NULL)) {
		g_debug("Register libsecret password manager plugin");
		password_manager_register(&secret);
	}

#ifdef SECRET_TEST
	g_debug("Starting secret\n");

	secret_store_password(profile_get_active(), "Test", "testpassword");

	g_debug("Got test password: '%s'", secret_get_password(profile_get_active(), "Test"));
	secret_remove_password(profile_get_active(), "Test");
#endif
}

void impl_deactivate(PeasActivatable *plugin)
{
	CallibreSecretPlugin *secret_plugin = CALLIBRE_SECRET_PLUGIN(plugin);
}
