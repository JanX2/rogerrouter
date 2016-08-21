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

#ifndef LIBROUTERMANAGER_RMFILTER_H
#define LIBROUTERMANAGER_RMFILTER_H

#include <libroutermanager/rmcall.h>

G_BEGIN_DECLS

enum {
	RM_FILTER_IS = 0,
	RM_FILTER_IS_NOT,
	RM_FILTER_STARTS_WITH,
	RM_FILTER_CONTAINS
};

enum {
	RM_FILTER_CALL_TYPE = 0,
	RM_FILTER_DATE_TIME,
	RM_FILTER_REMOTE_NAME,
	RM_FILTER_REMOTE_NUMBER,
	RM_FILTER_LOCAL_NAME,
	RM_FILTER_LOCAL_NUMBER
};

enum {
	RM_FILTER_CALL_ALL = 0,
	RM_FILTER_CALL_INCOMING,
	RM_FILTER_CALL_MISSED,
	RM_FILTER_CALL_OUTGOING,
	RM_FILTER_CALL_VOICE,
	RM_FILTER_CALL_FAX,
};

typedef struct rm_filter_rule {
	gint type;
	gint sub_type;
	gchar *entry;
} RmFilterRule;

typedef struct rm_filter {
	gchar *name;
	gchar *file;
	gboolean compare_or;
	GSList *rules;
} RmFilter;

gboolean rm_filter_rule_match(RmFilter *filter, RmCall *call);
void rm_filter_rule_add(RmFilter *filter, gint type, gint sub_type, gchar *entry);

struct rm_filter *filter_new(const gchar *name);
void rm_filter_free(gpointer data);
GSList *rm_filter_get_list(void);
void rm_filter_init(void);
void rm_filter_shutdown(void);
void rm_filter_add(RmFilter *filter);
void rm_filter_remove(RmFilter *filter);
void rm_filter_save(void);

G_END_DECLS

#endif
