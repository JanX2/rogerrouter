/**
 * The libroutermanager project
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#ifndef LIBROUTERMANAGER_RMROUTER_H
#define LIBROUTERMANAGER_RMROUTER_H

#include <gio/gio.h>

#include <libroutermanager/rmprofile.h>

G_BEGIN_DECLS

#define ROUTER_ENABLE_TELNET	"#96*5*"
#define ROUTER_ENABLE_CAPI		"#96*3*"

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

enum rm_router_dial_port {
	ROUTER_DIAL_PORT_AUTO = -1,
};

typedef struct {
	gchar *name;
	gint type;
} RmPhone;

typedef struct {
	enum phone_number_type type;
	gchar *number;
} RmPhoneNumber;

typedef struct {
	gchar *name;
	gint type;
	gint number;
} RmPhonePort;

#if 0
typedef struct {
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
} RmRouterInfo;
#endif

typedef struct {
	const gchar *name;
	gboolean (*present)(RmRouterInfo *router_info);
	gboolean (*login)(RmProfile *profile);
	gboolean (*logout)(RmProfile *profile, gboolean force);
	gboolean (*get_settings)(RmProfile *profile);
	gboolean (*load_journal)(RmProfile *profile, gchar **data);
	gboolean (*clear_journal)(RmProfile *profile);
	gboolean (*dial_number)(RmProfile *profile, gint port, const gchar *number);
	gboolean (*hangup)(RmProfile *profile, gint port, const gchar *number);
	gchar *(*load_fax)(RmProfile *profile, const gchar *filename, gsize *len);
	gchar *(*load_voice)(RmProfile *profile, const gchar *filename, gsize *len);
	gchar *(*get_ip)(RmProfile *profile);
	gboolean (*reconnect)(RmProfile *profile);
	gboolean (*delete_fax)(RmProfile *profile, const gchar *filename);
	gboolean (*delete_voice)(RmProfile *profile, const gchar *filename);
} RmRouter;

extern RmPhonePort rm_router_phone_ports[PORT_MAX];

gboolean rm_router_present(RmRouterInfo *router_info);
gboolean rm_router_login(RmProfile *profile);
gboolean rm_router_logout(RmProfile *profile);
gboolean rm_router_get_settings(RmProfile *profile);
const gchar *rm_router_get_name(RmProfile *profile);
const gchar *rm_router_get_version(RmProfile *profile);
gchar *rm_router_get_host(RmProfile *profile);
gchar *rm_router_get_login_password(RmProfile *profile);
gchar *rm_router_get_login_user(RmProfile *profile);
gchar *rm_router_get_ftp_password(RmProfile *profile);
gchar *rm_router_get_ftp_user(RmProfile *profile);
gboolean rm_router_load_journal(RmProfile *profile);
gboolean rm_router_clear_journal(RmProfile *profile);
gboolean rm_router_dial_number(RmProfile *profile, gint port, const gchar *number);
gboolean rm_router_hangup(RmProfile *profile, gint port, const gchar *number);
gchar *rm_router_get_ip(RmProfile *profile);
gboolean rm_router_reconnect(RmProfile *profile);
gboolean rm_router_delete_fax(RmProfile *profile, const gchar *filename);
gboolean rm_router_delete_voice(RmProfile *profile, const gchar *filename);

gchar *rm_router_get_area_code(RmProfile *profile);
gchar *rm_router_get_country_code(RmProfile *profile);
gchar *rm_router_get_international_prefix(RmProfile *profile);
gchar *rm_router_get_national_prefix(RmProfile *profile);

gboolean rm_router_init(void);
void rm_router_shutdown(void);

GSList *rm_router_get_phone_list(RmProfile *profile);
gchar **rm_router_get_numbers(RmProfile *profile);

void rm_router_process_journal(GSList *journal);

gboolean rm_router_register(RmRouter *router);

gchar *rm_router_load_fax(RmProfile *profile, const gchar *filename, gsize *len);
gchar *rm_router_load_voice(RmProfile *profile, const gchar *filename, gsize *len);

gboolean rm_router_info_free(RmRouterInfo *info);
gboolean rm_router_is_cable(RmProfile *profile);

GSList *rm_router_load_fax_reports(RmProfile *profile, GSList *journal);
GSList *rm_router_load_voice_records(RmProfile *profile, GSList *journal);

void rm_router_free_phone_list(GSList *phone_list);

gint rm_router_get_phone_port(RmProfile *profile);
void rm_router_set_phone_port(RmProfile *profile, gint port);

gboolean rm_router_get_suppress_state(RmProfile *profile);

void rm_router_release_lock(void);
gboolean rm_router_is_locked(void);


G_END_DECLS

#endif
