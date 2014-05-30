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

#ifndef LIBROUTERMANAGER_NUMBER_H
#define LIBROUTERMANAGER_NUMBER_H

G_BEGIN_DECLS

enum phone_number_type {
	PHONE_NUMBER_HOME,
	PHONE_NUMBER_WORK,
	PHONE_NUMBER_MOBILE,
	PHONE_NUMBER_FAX_HOME,
	PHONE_NUMBER_FAX_WORK,
	PHONE_NUMBER_PAGER,
};

struct phone_number {
	enum phone_number_type type;
	gchar *number;
};

G_END_DECLS

#endif
