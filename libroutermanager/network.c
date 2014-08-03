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

#include <libsoup/soup.h>

#include <libroutermanager/network.h>

/** Soup sync. session */
SoupSession *soup_session_sync = NULL;
/** Soup async. session */
SoupSession *soup_session_async = NULL;

/**
 * \brief Initialize network functions
 * \return TRUE on success, otherwise FALSE
 */
gboolean net_init(void)
{
	soup_session_sync = soup_session_sync_new_with_options(SOUP_SESSION_TIMEOUT, 5, NULL);
	soup_session_async = soup_session_async_new();

	return soup_session_sync && soup_session_async;
}

/**
 * \brief Deinitialize network infrastructure
 */
void net_shutdown(void)
{
	g_clear_object(&soup_session_sync);
	g_clear_object(&soup_session_async);
}
