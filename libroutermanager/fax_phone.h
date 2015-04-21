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

#ifndef LIBROUTERMANAGER_FAX_PHONE_H
#define LIBROUTERMANAGER_FAX_PHONE_H

G_BEGIN_DECLS

extern struct capi_connection *active_capi_connection;

void faxophone_setup(void);
struct capi_connection *fax_dial(gchar *tiff, const gchar *number, gboolean suppress);
struct capi_connection *phone_dial(const gchar *trg_no, gboolean suppress);

G_END_DECLS

#endif
