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

#ifndef FRITZBOX_CSV_H
#define FRITZBOX_CSV_H

G_BEGIN_DECLS

#define CSV_FRITZBOX_JOURNAL_DE "Typ;Datum;Name;Rufnummer;Nebenstelle;Eigene Rufnummer;Dauer"
#define CSV_FRITZBOX_JOURNAL_EN "Type;Date;Name;Number;Extension;Outgoing Caller ID;Duration"
#define CSV_FRITZBOX_JOURNAL_EN2 "Type;Date;Name;Number;Extension;Telephone Number;Duration"
#define CSV_FRITZBOX_JOURNAL_EN3 "Type;Date;Name;Telephone number;Extension;Telephone number;Duration"

GSList *csv_parse_fritzbox_journal_data(GSList *list, const gchar *data);

G_END_DECLS

#endif
