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

#ifndef LIBROUTERMANAGER_RMCALL_H
#define LIBROUTERMANAGER_RMCALL_H

#include <libroutermanager/rmcontact.h>
#include <libroutermanager/rmprofile.h>

G_BEGIN_DECLS

/** Supported call types */
enum rm_call_types {
	RM_CALL_TYPE_ALL,
	RM_CALL_TYPE_INCOMING,
	RM_CALL_TYPE_MISSED,
	RM_CALL_TYPE_OUTGOING,
	RM_CALL_TYPE_VOICE,
	RM_CALL_TYPE_FAX,
	RM_CALL_TYPE_FAX_REPORT,
	RM_CALL_TYPE_RECORD,
	RM_CALL_TYPE_BLOCKED
};
typedef struct {
	enum rm_call_types type;
	gchar *date_time;
	gchar *duration;

	RmContact *remote;
	RmContact *local;

	/* Private (e.g. original filename) */
	gchar *priv;
} RmCall;

RmCall *rm_call_new(enum rm_call_types type, const gchar *date_time, const gchar *remote_name, const gchar *remote_number, const gchar *local_name, const gchar *local_number, const gchar *duration, gpointer priv);
void rm_call_free(gpointer data);

G_END_DECLS

#endif
