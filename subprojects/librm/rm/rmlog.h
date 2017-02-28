/*
 * The rm project
 * Copyright (c) 2012-2017 Jan-Michael Brummer
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

#ifndef __RM_LOG_H
#define __RM_LOG_H

G_BEGIN_DECLS

typedef void (*rm_log_func)(GLogLevelFlags level, const gchar *message);
void rm_log_save_data(gchar *name, const gchar *data, gsize len);
void rm_log_init(void);
void rm_log_shutdown(void);
void rm_log_set_debug(gboolean state);
void rm_log_set_level(GLogLevelFlags level);
void rm_log_set_app_handler(rm_log_func app_log);

G_END_DECLS

#endif
