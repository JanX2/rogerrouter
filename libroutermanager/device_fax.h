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

#ifndef LIBROUTERMANAGER_DEVICE_FAX_H
#define LIBROUTERMANAGER_DEVICE_FAX_H

G_BEGIN_DECLS

struct device_fax {
	gchar *name;
	struct connection *(*send)(gchar *tiff, const gchar *target, gboolean anonymous);
	gboolean (*get_status)(struct fax_status *status);
	gint (*pickup)(struct connection *connection);
	void (*hangup)(struct connection *connection);

	gboolean (*number_is_handled)(gchar *number);
};

void fax_register(struct device_fax *fax);
GSList *fax_get_plugins(void);

G_END_DECLS

#endif
