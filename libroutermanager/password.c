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

/** Internal password manager pointer */
static struct password_manager *internal_password_manager = NULL;

/**
 * \brief Set password in manager
 * \param profile profile structure
 * \param name description of password
 * \param password password string
 */
void password_manager_set_password(struct profile *profile, const gchar *name, const gchar *password)
{
	if (internal_password_manager) {
		/* Store it in external password manager */
		internal_password_manager->set_password(profile, name, password);
		return;
	}

	/* Fallback: Use gsettings */
	g_settings_set_string(profile->settings, name, password);
}

/**
 * \brief Get password from manager
 * \param profile profile structure
 * \param name description of password
 * \return password on success, NULL on error
 */
gchar *password_manager_get_password(struct profile *profile, const gchar *name)
{
	if (internal_password_manager) {
		return internal_password_manager->get_password(profile, name);
	}

	/* Fallback: Use gsettings */
	return g_settings_get_string(profile->settings, name);
}

/**
 * \brief Remove password from manager
 * \param profile profile structure
 * \param name description of password
 * \return TRUE on success, FALSE on error
 */
gboolean password_manager_remove_password(struct profile *profile, const gchar *name)
{
	if (internal_password_manager) {
		return internal_password_manager->remove_password(profile, name);
	}

	/* Fallback: Use gsettings */
	g_settings_set_string(profile->settings, name, "");

	return TRUE;
}

/**
 * \brief Register password manager plugin
 * \param manager password manager plugin
 */
void password_manager_register(struct password_manager *manager)
{
	internal_password_manager = manager;
}
