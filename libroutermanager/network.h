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

#ifndef LIBROUTERMANAGER_NETWORK_H
#define LIBROUTERMANAGER_NETWORK_H

G_BEGIN_DECLS

#include <libsoup/soup.h>

struct auth_data {
	SoupMessage *msg;
	SoupAuth *auth;
	SoupSession *session;
	gboolean retry;
	gchar *username;
	gchar *password;
};

extern SoupSession *soup_session;

gboolean net_init(void);
void net_shutdown(void);
void network_authenticate(gboolean auth, struct auth_data *auth_data);

G_END_DECLS

#endif
