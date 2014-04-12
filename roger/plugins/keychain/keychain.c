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

static gchar *keychain_get_password(struct profile *profile, const gchar *type) {
	OSStatus status;
	UInt32 pwd_length;
	void *password = NULL;
	gchar *pwd_name;
	gchar *secret_password = NULL;

	if (profile == NULL || profile->name == NULL || type == NULL) {
		return NULL;
	}

	/* Find password */
	pwd_name = g_strdup_printf("%s/%s", profile->name, type);

	status = SecKeychainFindGenericPassword(
		NULL,
		strlen(SERVICE_NAME),
		SERVICE_NAME,
		strlen(pwd_name),
		pwd_name,
		&pwd_length,
		&password,
		NULL);
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

static void keychain_store_password(struct profile *profile, const gchar *type, const gchar *password) {
	OSStatus status;
	SecKeychainItemRef item_ref = NULL;
	gchar *pwd_name;

	if (profile == NULL || profile->name == NULL || type == NULL) {
		return;
	}

	/* store password */
	pwd_name = g_strdup_printf("%s/%s", profile->name, type);
	status = SecKeychainFindGenericPassword(
		NULL,
		strlen(SERVICE_NAME),
		SERVICE_NAME,
		strlen(pwd_name),
		pwd_name,
		NULL,
		NULL,
		&item_ref);

	if (status == noErr) {
		/* remove existing keychain entry. This is not a clean solution as we remove any access entries as well */
		/* macos however does not have a simple solution to just update a password */

		keychain_remove_password(profile, type);
	}

	/* insert new keychain entry */
	status = SecKeychainAddGenericPassword(
		NULL,
		strlen(SERVICE_NAME),
		SERVICE_NAME,
		strlen(pwd_name),
		pwd_name,
		strlen(password),
		password,
		NULL);

	g_free(pwd_name);
	/* Check return value, if error occurred: show reason */
	if (status != noErr) {
		g_warning("Couldn't store password: %d", status);
	}
}

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

struct password_manager keychain = {
	"OS X Keychain",
	keychain_store_password,
	keychain_get_password,
	keychain_remove_password,
};

void impl_activate(PeasActivatable *plugin)
{
	g_debug("Register keychain password manager plugin");
	password_manager_register(&keychain);
}

void impl_deactivate(PeasActivatable *plugin)
{
}
