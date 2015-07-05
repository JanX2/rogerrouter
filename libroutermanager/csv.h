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

#ifndef LIBROUTERMANAGER_CSV_H
#define LIBROUTERMANAGER_CSV_H

G_BEGIN_DECLS

typedef gpointer (*csv_parse_line_func)(gpointer ptr, gchar **split);

gpointer csv_parse_data(const gchar *data, const gchar *header, csv_parse_line_func csv_parse_line, gpointer ptr);
gboolean csv_save_journal_as(GSList *journal, gchar *file_name);
gboolean csv_save_journal(GSList *journal);
GSList *csv_load_journal(GSList *journal);

G_END_DECLS

#endif
