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
	GtkWidget *window;

	GtkWidget *child_frame;
	GtkWidget *control_frame;
	GtkWidget *call_frame;
	GtkWidget *header_bar;
	GtkWidget *search_entry;
	GtkWidget *mute_button;
	GtkWidget *hold_button;
	GtkWidget *record_button;
	GtkWidget *pickup_button;
	GtkWidget *hangup_button;

	GtkWidget *contact_menu;
	GtkWidget *scrolled_win;
	GtkWidget *box;
	const gchar *filter;
	gboolean discard;

	GTimer *phone_session_timer;
	gint phone_session_timer_id;

	const gchar *number;
	gpointer connection;

	gpointer priv;

	enum phone_type type;
};

struct phone_device {
	gchar *(*get_title)(void);
	gboolean (*init)(struct contact *contact, struct connection *connection);
	void (*terminated)(struct phone_state *state, struct capi_connection *connection);
	GtkWidget *(*create_menu)(struct profile *profile, struct phone_state *state);
	GtkWidget *(*create_child)(struct phone_state *state, GtkWidget *grid);
	void (*delete)(struct phone_state *state);
	void (*status)(struct phone_state *state, struct capi_connection *connection);
};

void app_show_phone_window(struct contact *contact, struct connection *connection);
GtkWidget *phone_search_entry_new(GtkWidget *window, struct contact *contact, struct phone_state *state);
void phone_call_notify_cb(AppObject *object, struct call *call, gint connection, gchar *medium, gpointer user_data);
void phone_setup_timer(struct phone_state *state);
void phone_remove_timer(struct phone_state *state);
GtkWidget *phone_dial_buttons_new(GtkWidget *window, struct phone_state *state);
void phone_dial_buttons_set_dial(struct phone_state *state, gboolean allow);
GtkWidget *phone_window_new(enum phone_type type, struct contact *contact, struct connection *connection, gpointer priv);

G_END_DECLS

#endif
