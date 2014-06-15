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

#ifndef PHONE_H
#define PHONE_H

#include <libroutermanager/contact.h>
#include <libroutermanager/connection.h>
#include <libroutermanager/appobject.h>

G_BEGIN_DECLS

enum phone_type {
	PHONE_TYPE_VOICE,
	PHONE_TYPE_FAX,
};

struct phone_state {
	GtkWidget *dialpad_frame;
	GtkWidget *control_frame;
	GtkWidget *call_frame;
	GtkWidget *phone_status_label;
	GtkWidget *name_entry;
	GtkWidget *photo_image;
	GtkWidget *llevel_in;
	GtkWidget *llevel_out;
	GtkWidget *call_label;
	GtkWidget *port_combobox;
	GtkWidget *number_combobox;
	gchar phone_status_text[255];
	GTimer *phone_session_timer;
	gint phone_session_timer_id;

	const gchar *number;
	struct connection *connection;

	gpointer priv;

	enum phone_type type;
};

void app_show_phone_window(struct contact *contact);
GtkWidget *phone_dial_frame(GtkWidget *window, struct contact *contact, struct phone_state *state);
void phone_call_notify_cb(AppObject *object, struct call *call, gint connection, gchar *medium, gpointer user_data);
gboolean phone_window_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data);
void phone_setup_timer(struct phone_state *state);
void phone_remove_timer(struct phone_state *state);
void phone_add_connection(gpointer connection);
void phone_remove_connection(gpointer connection);

G_END_DECLS

#endif
