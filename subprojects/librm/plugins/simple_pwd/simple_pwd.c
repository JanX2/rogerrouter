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

#include <rm/rmcallentry.h>
#include <rm/rmplugins.h>
#include <rm/rmpassword.h>
#include <rm/rmmain.h>

#define RM_TYPE_SIMPLE_PWD_PLUGIN        (rm_simple_pwd_plugin_get_type ())
#define RM_SIMPLE_PWD_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), RM_TYPE_SIMPLE_PWD_PLUGIN, RmSimplePwdPlugin))

typedef struct {
	guint dummy;
} RmSimplePwdPluginPrivate;

static GKeyFile *simple_pwd_keyfile = NULL;
static gchar *simple_pwd_file = NULL;
static gchar *simple_pwd_group = NULL;

RM_PLUGIN_REGISTER(RM_TYPE_SIMPLE_PWD_PLUGIN, RmSimplePwdPlugin, rm_simple_pwd_plugin)

/**
 * simple_pwd_store_password:
 * @profile: a #RmProfile
 * @name: password name
 * @password: password
 *
 * Store password
 */
static void simple_pwd_store_password(RmProfile *profile, const gchar *name, const gchar *password)
{
	GError *error = NULL;
	gchar *enc_password = rm_password_encode(password);

	if (!simple_pwd_keyfile) {
		return;
	}

	g_key_file_set_string(simple_pwd_keyfile, simple_pwd_group, name, enc_password);
	g_free(enc_password);

	if (!g_key_file_save_to_file(simple_pwd_keyfile, simple_pwd_file, &error)) {
		g_warning("%s(): Failed to store password: %s", __FUNCTION__, error ? error->message : "");
	}
}

/**
 * simple_pwd_get_password:
 * @profile: a #RmProfile
 * @name: password name
 *
 * Get password
 *
 * Returns: password
 */
static gchar *simple_pwd_get_password(RmProfile *profile, const gchar *name)
{
	GError *error = NULL;
	gchar *enc_password;
	gchar *password = NULL;

	enc_password = g_key_file_get_string(simple_pwd_keyfile, simple_pwd_group, name, &error);

	if (enc_password) {
		password = (gchar*) rm_password_decode(enc_password);
		g_free(enc_password);
	} else {
		g_warning("%s(): Failed to get password: %s", __FUNCTION__, error ? error->message : "");
		password = g_strdup("");
	}

	return password;
}

/**
 * simple_pwd_remove_password:
 * @profile: a #RmProfile
 * @name: password name
 *
 * Remove password
 *
 * Returns: %TRUE on success, otherwise %FALSE
 */
static gboolean simple_pwd_remove_password(RmProfile *profile, const gchar *name)
{
	g_key_file_remove_key(simple_pwd_keyfile, simple_pwd_group, name, NULL);

	g_key_file_save_to_file(simple_pwd_keyfile, simple_pwd_file, NULL);
	return FALSE;
}

RmPasswordManager simple_pwd = {
	"Simple Password",
	simple_pwd_store_password,
	simple_pwd_get_password,
	simple_pwd_remove_password,
};

/**
 * impl_activate:
 * @plugin: a #PeasActivatable
 *
 * Activate peas plugin - register libsimple_pwd password manager if present
 */
void impl_activate(PeasActivatable *plugin)
{
	GError *error = NULL;

	simple_pwd_keyfile = g_key_file_new();

	simple_pwd_file = g_build_filename(rm_get_user_config_dir(), G_DIR_SEPARATOR_S, "simple_pwd.keys", NULL);
	simple_pwd_group = g_strdup("passwords");

	if (!g_key_file_load_from_file(simple_pwd_keyfile, simple_pwd_file, G_KEY_FILE_NONE, &error)) {
		g_warning("%s(): could not load key file -> %s", __FUNCTION__, error ? error->message : "");
	}

	rm_password_register(&simple_pwd);
}

/**
 * impl_deactivate:
 * @plugin: a #PeasActivatable
 *
 * Deactivate peas plugin
 */
void impl_deactivate(PeasActivatable *plugin)
{
	//g_key_file_free(simple_pwd_keyfile);
}

