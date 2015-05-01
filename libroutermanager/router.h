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

enum phone_ports {
	PORT_SOFTPHONE,
	PORT_ANALOG1,
	PORT_ANALOG2,
	PORT_ANALOG3,
	PORT_ISDNALL,
	PORT_ISDN1,
	PORT_ISDN2,
	PORT_ISDN3,
	PORT_ISDN4,
	PORT_ISDN5,
	PORT_ISDN6,
	PORT_ISDN7,
	PORT_ISDN8,
	PORT_DECT1,
	PORT_DECT2,
	PORT_DECT3,
	PORT_DECT4,
	PORT_DECT5,
	PORT_DECT6,
	PORT_IP1,
	PORT_IP2,
	PORT_IP3,
	PORT_IP4,
	PORT_IP5,
	PORT_IP6,
	PORT_IP7,
	PORT_IP8,
	PORT_IP9,
	PORT_IP10,
	PORT_MAX
};

enum phone_number_type {
	PHONE_NUMBER_HOME,
	PHONE_NUMBER_WORK,
	PHONE_NUMBER_MOBILE,
	PHONE_NUMBER_FAX_HOME,
	PHONE_NUMBER_FAX_WORK,
	PHONE_NUMBER_PAGER,
};

struct phone {
	gchar *name;
	gint type;
};

struct phone_number {
	enum phone_number_type type;
	gchar *number;
};

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
	gchar *annex;

	/* Extend */
	gint box_id;
	gint maj_ver_id;
	gint min_ver_id;
	GTimer *session_timer;
};

struct router {
	const gchar *name;
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

extern struct phone_port router_phone_ports[PORT_MAX];

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
gboolean router_is_cable(struct profile *profile);

GSList *router_load_fax_reports(struct profile *profile, GSList *journal);
GSList *router_load_voice_records(struct profile *profile, GSList *journal);

void router_free_phone_list(GSList *phone_list);

gint router_get_phone_port(struct profile *profile);
void router_set_phone_port(struct profile *profile, gint port);

gboolean router_get_suppress_state(struct profile *profile);


G_END_DECLS

#endif
