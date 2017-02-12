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

#include <windows.h>
#include <wincred.h>

#include <wchar.h>

#include <glib.h>
#include <gio/gio.h>

#include <rm/rmplugins.h>
#include <rm/rmpassword.h>

/**
 * \brief Store password
 * \param profile profile pointer
 * \param name password name
 * \param password password
 */
static void wincred_store_password(RmProfile *profile, const gchar *name, const gchar *password)
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

/**
 * \brief Get password
 * \param profile profile pointer
 * \param name password name
 * \return password
 */
static gchar *wincred_get_password(RmProfile *profile, const gchar *name)
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

/**
 * \brief Remove password
 * \param profile profile pointer
 * \param name password name
 * \return TRUE on success, otherwise FALSE
 */
static gboolean wincred_remove_password(RmProfile *profile, const gchar *name)
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

RmPasswordManager wincred = {
	"Windows Credentials",
	wincred_store_password,
	wincred_get_password,
	wincred_remove_password,
};

/**
 * \brief Activate plugin - register windows credential password manager
 * \pram plugin peas plugin
 */
gboolean wincred_plugin_init(RmPlugin *plugin)
{
	rm_password_register(&wincred);

	return TRUE;
}

/**
 * \brief Deactivate plugin
 * \pram plugin peas plugin
 */
gboolean wincred_plugin_shutdown(RmPlugin *plugin)
{
	return TRUE;
}

PLUGIN(wincred);
