/**
 * Roger Router - Router manager
 * Copyright (c) 2012-2013 Jan-Michael Brummer
 *
 * This file is part of Roger Router.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <wincred.h>

#include <wchar.h>

#include <libcallibre/plugins.h>
#include <libcallibre/password.h>

#define CALLIBRE_TYPE_WINCRED_PLUGIN        (callibre_wincred_plugin_get_type ())
#define CALLIBRE_WINCRED_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), CALLIBRE_TYPE_WINCRED_PLUGIN, CallibreWinCredPlugin))

typedef struct {
        guint signal_id;
} CallibreWinCredPluginPrivate;

CALLIBRE_PLUGIN_REGISTER(CALLIBRE_TYPE_WINCRED_PLUGIN, CallibreWinCredPlugin, callibre_wincred_plugin)

static void wincred_store_password(struct profile *profile, const gchar *name, const gchar *password)
{
	CREDENTIALA cred = { 0 };
	WCHAR *wide_password = NULL;
	gchar *cred_name;
	int length;

	if (!name || !profile || !profile->name || !password) {
		return;
	}

	length = strlen(password);

	wide_password = malloc(length * sizeof(WCHAR));
	swprintf(wide_password, length, L"%S", password);

	cred_name = g_strdup_printf("%s/%s", profile->name, name);

	cred.Type = CRED_TYPE_GENERIC;
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	cred.TargetName = cred_name;
	cred.UserName = cred_name;
	cred.CredentialBlob = (BYTE *) wide_password;
	cred.CredentialBlobSize = sizeof(BYTE) * sizeof(WCHAR) * length;

	CredWrite(&cred, 0);

	g_free(cred_name);
	g_free(wide_password);
}

static gchar *wincred_get_password(struct profile *profile, const gchar *name)
{
	PCREDENTIALA cred;
	BOOL result;
	gchar *cred_name;
	gchar *secret_password = NULL;

	if (!profile || !profile->name || !name) {
		return NULL;
	}

	cred_name = g_strdup_printf("%s/%s", profile->name, name);
	result = CredReadA(cred_name, CRED_TYPE_GENERIC, 0, &cred);
	g_free(cred_name);

	if (result == TRUE) {
		secret_password = g_strdup_printf("%S", (WCHAR *) cred->CredentialBlob);

		if (cred->CredentialBlobSize > 1) {
			secret_password[ cred->CredentialBlobSize / 2 ] = '\0';
		}
	}

	CredFree(cred);

	return g_strdup(secret_password);
}

static gboolean wincred_remove_password(struct profile *profile, const gchar *name)
{
	PCREDENTIALA cred;
	BOOL result;
	gchar *cred_name;

	if (!profile || !profile->name || !name) {
		return FALSE;
	}

	cred_name = g_strdup_printf("%s/%s", profile->name, name);
	result = CredRead(cred_name, CRED_TYPE_GENERIC, 0, &cred);
	g_free(cred_name);
	if (result == TRUE) {
		CredDelete(cred->TargetName, cred->Type, 0);
	}

	return result;
}

struct password_manager wincred = {
	"Windows Credentials",
	wincred_store_password,
	wincred_get_password,
	wincred_remove_password,
};

void impl_activate(PeasActivatable *plugin)
{
	g_debug("Register wincred password manager plugin");
	password_manager_register(&wincred);
}

void impl_deactivate(PeasActivatable *plugin)
{
}
