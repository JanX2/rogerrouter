/*
 * The rm project
 * Copyright (c) 2012-2017 Jan-Michael Brummer
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

#ifndef __RM_FILTER_H
#define __RM_FILTER_H

#include <rm/rmcallentry.h>

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

typedef struct {
	gint type;
	gint sub_type;
	gchar *entry;
} RmFilterRule;

typedef struct {
	gchar *name;
	gchar *file;
	gboolean compare_or;
	GSList *rules;
} RmFilter;

void rm_filter_init(RmProfile *profile);
void rm_filter_shutdown(RmProfile *profile);
GSList *rm_filter_get_list(RmProfile *profile);

RmFilter *rm_filter_new(RmProfile *profile, const gchar *name);
void rm_filter_remove(RmProfile *profile, RmFilter *filter);

gboolean rm_filter_rule_match(RmFilter *filter, RmCallEntry *call);
void rm_filter_rule_add(RmFilter *filter, gint type, gint sub_type, gchar *entry);

G_END_DECLS

#endif
