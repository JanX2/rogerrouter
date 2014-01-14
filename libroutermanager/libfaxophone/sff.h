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

/**
 * \file sff.h
 * \brief SFF structures
 */

#ifndef SFF_H
#define SFF_H

//#define SFF_CIP 0x04
#define SFF_CIP 0x11

struct capi_connection *sff_send(gchar *tiff_file, gint modem, gint ecm, gint controller, const gchar *src_no, const gchar *trg_no, const gchar *lsi, const gchar *local_header_info, gint call_anonymous);
void sff_init_data(struct capi_connection *connection);
void sff_clean(struct capi_connection *connection);

#endif
