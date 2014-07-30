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

#ifndef LIBROUTERMANAGER_VOXPLAY_H
#define LIBROUTERMANAGER_VOXPLAY_H

G_BEGIN_DECLS

gpointer vox_init(gchar *data, gsize len, GError **error);
gboolean vox_play(gpointer vox_data);
gboolean vox_stop(gpointer vox_data);
gboolean vox_playpause(gpointer vox_data);
gboolean vox_seek(gpointer vox_data, gdouble pos);
gint vox_get_fraction(gpointer vox_data);

G_END_DECLS

#endif
