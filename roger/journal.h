/**
 * Roger Router
 * Copyright (c) 2012-2014 Jan-Michael Brummer
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

#ifndef JOURNAL_H
#define JOURNAL_H

#include <libroutermanager/filter.h>
#include <libroutermanager/call.h>

G_BEGIN_DECLS

enum {
	JOURNAL_COL_TYPE = 0,
	JOURNAL_COL_DATETIME,
	JOURNAL_COL_NAME,
	JOURNAL_COL_COMPANY,
	JOURNAL_COL_NUMBER,
	JOURNAL_COL_CITY,
	JOURNAL_COL_EXTENSION,
	JOURNAL_COL_LINE,
	JOURNAL_COL_DURATION,
	JOURNAL_COL_CALL_PTR,
};

GtkWidget *journal_window(GApplication *app, GFile *file);
void journal_set_visible(gboolean state);
GdkPixbuf *journal_get_call_icon(gint type);

void journal_add_call(struct call *call);
void journal_update_filter(void);
void journal_quit(void);

void journal_set_hide_on_quit(gboolean hide);
void journal_set_hide_on_start(gboolean hide);

GtkWidget *journal_get_window(void);

G_END_DECLS

#endif
