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

#ifndef LIBROUTERMANAGER_FILTER_H
#define LIBROUTERMANAGER_FILTER_H

#include <libroutermanager/call.h>

G_BEGIN_DECLS

enum {
	FILTER_IS = 0,
	FILTER_IS_NOT,
	FILTER_STARTS_WITH,
	FILTER_CONTAINS
};

enum {
	FILTER_CALL_TYPE = 0,
	FILTER_DATE_TIME,
	FILTER_REMOTE_NAME,
	FILTER_REMOTE_NUMBER,
	FILTER_LOCAL_NAME,
	FILTER_LOCAL_NUMBER
};

enum {
	FILTER_CALL_ALL = 0,
	FILTER_CALL_INCOMING,
	FILTER_CALL_MISSED,
	FILTER_CALL_OUTGOING,
	FILTER_CALL_VOICE,
	FILTER_CALL_FAX,
};

struct filter_rule {
	gint type;
	gint sub_type;
	gchar *entry;
};

struct filter {
	gchar *name;
	gchar *file;
	gboolean compare_or;
	GSList *rules;
};

gboolean filter_rule_match(struct filter *filter, struct call *call);
void filter_rule_add(struct filter *filter, gint type, gint sub_type, gchar *entry);
struct filter *filter_new(const gchar *name);
void filter_free(gpointer data);
GSList *filter_get_list(void);
void filter_init(void);
void filter_shutdown(void);
void filter_add(struct filter *filter);
void filter_remove(struct filter *filter);
void filter_save(void);

G_END_DECLS

#endif
