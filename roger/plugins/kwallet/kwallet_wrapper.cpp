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

extern "C" {
extern int kwallet4_get_password( const char *, char ** );
extern int kwallet4_store_password( const char *, const char * );
extern int kwallet4_init( void );

#include <libroutermanager/call.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/password.h>

#define ROUTERMANAGER_TYPE_KWALLET_PLUGIN        (routermanager_kwallet_plugin_get_type ())
#define ROUTERMANAGER_KWALLET_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_KWALLET_PLUGIN, RouterManagerKWalletPlugin))

typedef struct {
	guint signal_id;
} RouterManagerKWalletPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_KWALLET_PLUGIN, RouterManagerKWalletPlugin, routermanager_kwallet_plugin)

/**
 * \brief Get password out of kwallet
 * \param profile profile structure
 * \param name name of password
 * \return password or NULL on error
 */
static gchar *kwallet_get_password(struct profile *profile, const gchar *name)
{
	gint error = -1;
	gchar *pwd_name;
	gchar *secret_password = NULL;

	pwd_name = g_strdup_printf("%s/%s", profile->name, name);
	error = kwallet4_get_password(pwd_name, &secret_password);
	g_free(pwd_name);

	if (error != 0) {
		return NULL;
	}

	return g_strdup(secret_password);
}

/**
 * \brief Store password in kwallet
 * \param profile profile structure
 * \param name name of password
 * \param password password text
 */
static void kwallet_store_password(struct profile *profile, const gchar *name, const gchar *password)
{
	gchar *pwd_name;

	pwd_name = g_strdup_printf("%s/%s", profile->name, name);
	kwallet4_store_password(pwd_name, password);
	g_free(pwd_name);
}

/**
 * \brief Remove password in kwallet (NOT IMPLEMENTED!)
 * \param profile profile structure
 * \param name name of password
 * \return FALSE
 */
static gboolean kwallet_remove_password(struct profile *profile, const gchar *name)
{
	return FALSE;
}

/** KWallet password manager structure */
struct password_manager kwallet = {
	"KWallet",
	kwallet_store_password,
	kwallet_get_password,
	kwallet_remove_password,
};

/**
 * \brief Active plugin
 * \param plugin peas plugin structure
 */
void impl_activate(PeasActivatable *plugin)
{
	if (kwallet4_init()) {
		g_debug("Register kwallet password manager plugin");
		password_manager_register(&kwallet);
	}
}

/**
 * \brief Deactive plugin
 * \param plugin peas plugin structure
 */
void impl_deactivate(PeasActivatable *plugin)
{
}
}
