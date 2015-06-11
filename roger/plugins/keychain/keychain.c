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

/**
 * TODO
 * Instead of removing password during update, use SecKeychainItemModifyContent() instead
 */

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <CoreServices/CoreServices.h>

#include <libroutermanager/call.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/password.h>

#define ROUTERMANAGER_TYPE_KEYCHAIN_PLUGIN        (routermanager_keychain_plugin_get_type ())
#define ROUTERMANAGER_KEYCHAIN_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_KEYCHAIN_PLUGIN, RouterManagerKeyChainPlugin))

typedef struct {
	guint signal_id;
} RouterManagerKeyChainPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_KEYCHAIN_PLUGIN, RouterManagerKeyChainPlugin, routermanager_keychain_plugin)

#define SERVICE_NAME "roger"

static gboolean keychain_remove_password(struct profile *profile, const gchar *type);

OSStatus keychain_get(gchar *pwd_name, void **password, UInt32 *pwd_len, SecKeychainItemRef *item_ref)
{
	OSStatus status;

	status = SecKeychainFindGenericPassword(
		NULL,
		strlen(SERVICE_NAME),
		SERVICE_NAME,
		strlen(pwd_name),
		pwd_name,
		pwd_len,
		password,
		item_ref);

	return status;
}

/**
 * \brief Get password of secure chain
 * \param profile profile structure pointer
 * \param type password type
 * \return password or NULL on error
 */
static gchar *keychain_get_password(struct profile *profile, const gchar *type)
{
	OSStatus status;
	UInt32 pwd_length;
	void *password = NULL;
	gchar *pwd_name;
	gchar *secret_password = NULL;
	SecKeychainItemRef ref;

	if (profile == NULL || profile->name == NULL || type == NULL) {
		return NULL;
	}

	/* Find password */
	pwd_name = g_strdup_printf("%s/%s", profile->name, type);
	status = keychain_get(pwd_name, &password, &pwd_length, &ref);
	g_free(pwd_name);

	/* Check return value, if no error occurred: return password */
	if (status == noErr) {
		secret_password = g_strndup((char *) password, pwd_length);
		SecKeychainItemFreeContent(NULL, password);
		return secret_password;
	}

	g_warning("Couldn't find password: %d", status);

	return NULL;
}

/**
 * \brief Store a new password in secure chain
 * \param profile profile structure pointer
 * \param type password type
 * \param password password text
 */
static void keychain_store_password(struct profile *profile, const gchar *type, const gchar *password) {
	OSStatus status;
	SecKeychainItemRef item_ref = NULL;
	gchar *pwd_name;
	UInt32 pwd_len = 0;
	void *pwd_data = NULL;

	if (profile == NULL || profile->name == NULL || type == NULL) {
		return;
	}

	/* store password */
	pwd_name = g_strdup_printf("%s/%s", profile->name, type);

	status = keychain_get(pwd_name, &pwd_data, &pwd_len, &item_ref);
	g_free(pwd_name);

	if (status == errSecItemNotFound) {
		status = SecKeychainAddGenericPassword(NULL, strlen(SERVICE_NAME), SERVICE_NAME, strlen(pwd_name), pwd_name, strlen(password), password, NULL);
		return;
	}

	if (status == noErr) {
		status = SecKeychainItemFreeContent(NULL, pwd_data);
	}

	if (status) {
		g_warning("Couldn't gete password: %d", status);
		CFRelease(item_ref);
		return;
	}
	
	/* insert new keychain entry */
	status = SecKeychainItemModifyAttributesAndData(item_ref, NULL, (UInt32) strlen(password), password);

	/* Check return value, if error occurred: show reason */
	if (status != noErr) {
		g_warning("Couldn't modify password: %d", status);
	}

	if (item_ref) {
		CFRelease(item_ref);
	}
}

/**
 * \brief Remove password of secure chain
 * \param profile profile structure pointer
 * \param type type indicating which password to remove
 * \return TRUE on success, otherwise FALSE on error
 */
static gboolean keychain_remove_password(struct profile *profile, const gchar *type) {
	OSStatus status;
	SecKeychainItemRef item_ref = NULL;
	gchar *pwd_name;
	gboolean ret = TRUE;

	if (profile == NULL || profile->name == NULL) {
		return FALSE;
	}

	pwd_name = g_strdup_printf("%s/%s", profile->name, type);
	status = SecKeychainFindGenericPassword(
		NULL,
		strlen(SERVICE_NAME),
		SERVICE_NAME,
		strlen(profile->name),
		profile->name,
		NULL,
		NULL,
		&item_ref);

	if (status == noErr) {
		g_assert(item_ref != NULL);
		status = SecKeychainItemDelete(item_ref);
	}

	/* Check return value, if error occurred: show reason */
	if (status != noErr) {
		g_warning("Couldn't remove password: %d", status);
		ret = FALSE;
	}

	g_free(pwd_name);

	return ret;
}

/** Keychain password manager structure */
struct password_manager keychain = {
	"OS X Keychain",
	keychain_store_password,
	keychain_get_password,
	keychain_remove_password,
};

/**
 * \brief Activate plugin
 * \param plugin peas plugin structure
 */
void impl_activate(PeasActivatable *plugin)
{
	g_debug("Register keychain password manager plugin");
	password_manager_register(&keychain);
}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin structure
 */
void impl_deactivate(PeasActivatable *plugin)
{
}
