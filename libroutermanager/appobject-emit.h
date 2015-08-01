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

#ifndef LIBROUTERMANAGER_APPOBJECT_EMIT_H
#define LIBROUTERMANAGER_APPOBJECT_EMIT_H

#include <libroutermanager/connection.h>
#include <libroutermanager/libfaxophone/faxophone.h>

G_BEGIN_DECLS

void emit_connection_notify(struct connection *connection);
void emit_contact_process(struct contact *contact);
void emit_journal_loaded(GSList *journal);
void emit_fax_process(const gchar *filename);

void emit_connection_established(struct capi_connection *connection);
void emit_connection_terminated(struct capi_connection *connection);
void emit_connection_status(gint status, struct capi_connection *connection);

void emit_message(gint status, gchar *message);

void emit_contacts_changed(void);

G_END_DECLS

#endif
