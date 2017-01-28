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

#ifndef __RM_FAX_H_
#define __RM_FAX_H_

#include <rm/rmconnection.h>
#include <rm/rmdevice.h>

G_BEGIN_DECLS

typedef enum {
	FAX_PHASE_CALL,
	FAX_PHASE_IDENTIFY,
	FAX_PHASE_SIGNALLING,
	FAX_PHASE_RELEASE,
} RmFaxPhase;

typedef struct _RmFaxStatus {
	RmFaxPhase phase;
	gdouble percentage;
	gchar *remote_ident;
	gchar *local_ident;
	gchar *remote_number;
	gchar *local_number;
	gint bitrate;
	gint page_current;
	gint page_total;
	gint error_code;
} RmFaxStatus;

typedef struct _RmFax {
	RmDevice *device;
	gchar *name;
	RmConnection *(*send)(gchar *tiff, const gchar *target, gboolean anonymous);
	gboolean (*get_status)(RmConnection *connection, RmFaxStatus *fax_status);
	gint (*pickup)(RmConnection *connection);
	void (*hangup)(RmConnection *connection);

	gboolean (*number_is_handled)(gchar *number);
} RmFax;

void rm_fax_register(RmFax *fax);
GSList *rm_fax_get_plugins(void);
gchar *rm_fax_get_name(RmFax *fax);
gboolean rm_fax_get_status(RmFax *fax, RmConnection *connection, RmFaxStatus *status);
RmConnection *rm_fax_send(RmFax *fax, gchar *tiff, const gchar *target, gboolean anonymous);
RmFax *rm_fax_get(gchar *name);
void rm_fax_hangup(RmFax *fax, RmConnection *connection);

G_END_DECLS

#endif
