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

#ifndef PRINT_H
#define PRINT_H

#include <libroutermanager/router.h>
#include <libroutermanager/libfaxophone/faxophone.h>
#include <libroutermanager/libfaxophone/fax.h>

G_BEGIN_DECLS

void journal_print(GtkWidget *view_widget);
void create_fax_report(struct fax_status *status, const char *report_dir);

G_END_DECLS

#endif
