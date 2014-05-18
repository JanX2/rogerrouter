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

#include <string.h>

#include <libroutermanager/password.h>

/** Internal password manager list */
static GSList *pm_plugins = NULL;

struct password_manager *password_manager_find(struct profile *profile)
{
	gchar *name = g_settings_get_string(profile->settings, "password-manager");
	GSList *list;

	for (list = pm_plugins; list != NULL; list = list->next) {
		struct password_manager *pm = list->data;

		if (pm && pm->name && name && !strcmp(pm->name, name)) {
			return pm;
		}
	}

	return pm_plugins ? pm_plugins->data : NULL;
}

/**
 * \brief Set password in manager
 * \param profile profile structure
 * \param name description of password
 * \param password password string
 */
void password_manager_set_password(struct profile *profile, const gchar *name, const gchar *password)
{
	struct password_manager *pm = password_manager_find(profile);

	pm->set_password(profile, name, password);
}

/**
 * \brief Get password from manager
 * \param profile profile structure
 * \param name description of password
 * \return password on success, NULL on error
 */
gchar *password_manager_get_password(struct profile *profile, const gchar *name)
{
	struct password_manager *pm = password_manager_find(profile);

	return pm->get_password(profile, name);
}

/**
 * \brief Remove password from manager
 * \param profile profile structure
 * \param name description of password
 * \return TRUE on success, FALSE on error
 */
gboolean password_manager_remove_password(struct profile *profile, const gchar *name)
{
	struct password_manager *pm = password_manager_find(profile);

	return pm->remove_password(profile, name);
}

/**
 * \brief Register password manager plugin
 * \param manager password manager plugin
 */
void password_manager_register(struct password_manager *manager)
{
	pm_plugins = g_slist_prepend(pm_plugins, manager);
}

GSList *password_manager_get_plugins(void)
{
	return pm_plugins;
}

void simple_store_password(struct profile *profile, const gchar *name, const gchar *password)
{
	g_settings_set_string(profile->settings, name, password);
}

gchar *simple_get_password(struct profile *profile, const gchar *name)
{
	return g_settings_get_string(profile->settings, name);
}

gboolean simple_remove_password(struct profile *profile, const gchar *name)
{
	g_settings_set_string(profile->settings, name, "");

	return TRUE;
}

struct password_manager simple = {
	"Simple",
	simple_store_password,
	simple_get_password,
	simple_remove_password,
};

void password_manager_init(void)
{
	password_manager_register(&simple);
}
