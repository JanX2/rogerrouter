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

#ifndef LIBROUTERMANAGER_ROUTER_H
#define LIBROUTERMANAGER_ROUTER_H

#include <gio/gio.h>

#include <libroutermanager/profile.h>

G_BEGIN_DECLS

#define NUM_PHONE_PORTS 27

#define PORT_ANALOG1 0x0001
#define PORT_ANALOG2 0x0002
#define PORT_ANALOG3 0x0004
#define PORT_ISDNALL 0x0008
#define PORT_ISDN1 0x0010
#define PORT_ISDN2 0x0020
#define PORT_ISDN3 0x0040
#define PORT_ISDN4 0x0080
#define PORT_ISDN5 0x0100
#define PORT_ISDN6 0x0200
#define PORT_ISDN7 0x0400
#define PORT_ISDN8 0x0800
#define PORT_DECT1 0x1000
#define PORT_DECT2 0x2000
#define PORT_DECT3 0x4000
#define PORT_DECT4 0x8000
#define PORT_DECT5 0x10000
#define PORT_DECT6 0x20000

#define PORT_IP1 0x40000
#define PORT_IP2 0x80000
#define PORT_IP3 0x100000
#define PORT_IP4 0x200000
#define PORT_IP5 0x400000
#define PORT_IP6 0x800000
#define PORT_IP7 0x1000000
#define PORT_IP8 0x2000000
#define PORT_IP9 0x4000000
#define PORT_IP10 0x8000000

#define PORT_SOFTPHONE 0x10000000

struct phone_port {
	gchar *name;
	gint type;
	gint number;
};

struct router_info {
	gchar *host;
	gchar *user;
	gchar *password;
	gchar *name;
	gchar *version;
	gchar *serial;
	gchar *session_id;
	gchar *lang;

	/* Extend */
	gint box_id;
	gint maj_ver_id;
	gint min_ver_id;
	GTimer *session_timer;
};

struct router {
	gboolean (*present)(struct router_info *router_info);
	gboolean (*login)(struct profile *profile);
	gboolean (*logout)(struct profile *profile, gboolean force);
	gboolean (*get_settings)(struct profile *profile);
	gboolean (*load_journal)(struct profile *profile, gchar **data);
	gboolean (*clear_journal)(struct profile *profile);
	gboolean (*dial_number)(struct profile *profile, gint port, const gchar *number);
	gboolean (*hangup)(struct profile *profile, gint port, const gchar *number);
	gchar *(*load_fax)(struct profile *profile, const gchar *filename, gsize *len);
	gchar *(*load_voice)(struct profile *profile, const gchar *filename, gsize *len);
	gchar *(*get_ip)(struct profile *profile);
	gboolean (*reconnect)(struct profile *profile);
	gboolean (*delete_fax)(struct profile *profile, const gchar *filename);
	gboolean (*delete_voice)(struct profile *profile, const gchar *filename);
};

extern struct phone_port router_phone_ports[NUM_PHONE_PORTS];

gboolean router_present(struct router_info *router_info);
gboolean router_login(struct profile *profile);
gboolean router_logout(struct profile *profile);
gboolean router_get_settings(struct profile *profile);
const gchar *router_get_name(struct profile *profile);
const gchar *router_get_version(struct profile *profile);
gchar *router_get_host(struct profile *profile);
gchar *router_get_login_password(struct profile *profile);
gchar *router_get_login_user(struct profile *profile);
gchar *router_get_ftp_password(struct profile *profile);
gchar *router_get_ftp_user(struct profile *profile);
gboolean router_load_journal(struct profile *profile);
gboolean router_clear_journal(struct profile *profile);
gboolean router_dial_number(struct profile *profile, gint port, const gchar *number);
gboolean router_hangup(struct profile *profile, gint port, const gchar *number);
gchar *router_get_ip(struct profile *profile);
gboolean router_reconnect(struct profile *profile);
gboolean router_delete_fax(struct profile *profile, const gchar *filename);
gboolean router_delete_voice(struct profile *profile, const gchar *filename);

gchar *router_get_area_code(struct profile *profile);
gchar *router_get_country_code(struct profile *profile);
gchar *router_get_international_prefix(struct profile *profile);
gchar *router_get_national_prefix(struct profile *profile);

gboolean router_init(void);
void router_shutdown(void);

GSList *router_get_phone_list(struct profile *profile);
gchar **router_get_numbers(struct profile *profile);

void router_process_journal(GSList *journal);

gboolean routermanager_router_register(struct router *router_new);

gchar *router_load_fax(struct profile *profile, const gchar *filename, gsize *len);
gchar *router_load_voice(struct profile *profile, const gchar *filename, gsize *len);

gboolean router_info_free(struct router_info *info);

G_END_DECLS

#endif
