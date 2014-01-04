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

#ifndef FIRMWARE_04_74_H
#define FIRMWARE_04_74_H

G_BEGIN_DECLS

gboolean fritzbox_login_04_74(struct profile *profile);
gboolean fritzbox_get_settings_04_74(struct profile *profile);
gboolean fritzbox_load_journal_04_74(struct profile *profile, gchar **data_ptr);
gboolean fritzbox_clear_journal_04_74(struct profile *profile);

G_END_DECLS

#endif
