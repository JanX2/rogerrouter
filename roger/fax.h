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

#ifndef ROGER_FAX_H
#define ROGER_FAX_H

G_BEGIN_DECLS

#include <libroutermanager/libfaxophone/fax.h>

struct fax_ui {
	gchar *file;

	GtkWidget *progress_bar;
	GtkWidget *remote_label;
	GtkWidget *page_current_label;
	GtkWidget *status_current_label;

	struct fax_status *status;
	struct capi_connection *fax_connection;
};

void fax_process_init(void);
void app_show_fax_window(gchar *tiff_file);
void fax_window_clear(gpointer priv);

G_END_DECLS

#endif
