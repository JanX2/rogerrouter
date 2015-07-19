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

#ifndef FRITZBOX_H
#define FRITZBOX_H

#include <libroutermanager/router.h>

G_BEGIN_DECLS

gboolean fritzbox_login(struct profile *profile);
gboolean fritzbox_get_settings(struct profile *profile);
gboolean fritzbox_load_journal(struct profile *profile, gchar **data_ptr);
gboolean fritzbox_dial_number(struct profile *profile, gint port, const gchar *number);
gboolean fritzbox_hangup(struct profile *profile, gint port, const gchar *number);

G_END_DECLS

#endif
