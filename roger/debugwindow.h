/**
 * Roger Router
 * Copyright (c) 2012-2013 Jan-Michael Brummer
 *
 * This file is part of Roger Router.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

void debug_window_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);
void debug_window(void);

G_END_DECLS

#endif
