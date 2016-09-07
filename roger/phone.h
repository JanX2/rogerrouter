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

#include <libroutermanager/rmcontact.h>
#include <libroutermanager/rmconnection.h>
#include <libroutermanager/rmcall.h>
#include <libroutermanager/rmobject.h>

G_BEGIN_DECLS

enum phone_type {
	PHONE_TYPE_VOICE,
	PHONE_TYPE_FAX,
};

struct phone_state {
	GtkWidget *window;
	GtkWidget *grid;

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

	GtkWidget *menu_button;

	GtkWidget *contact_menu;
	GtkWidget *scrolled_win;
	GtkWidget *box;
	const gchar *filter;
	gboolean discard;

	gint status_timer_id;

	const gchar *number;
	RmConnection *connection;

	gpointer priv;

	enum phone_type type;
};

struct phone_device {
	gchar *(*get_title)(void);
	gboolean (*init)(RmContact *contact, RmConnection *connection);
	void (*terminated)(struct phone_state *state, RmConnection *connection);
	GtkWidget *(*create_menu)(struct profile *profile, struct phone_state *state);
	GtkWidget *(*create_child)(struct phone_state *state, GtkWidget *grid);
	void (*delete)(struct phone_state *state);
	void (*status)(struct phone_state *state, RmConnection *connection);
};

void app_show_phone_window(RmContact *contact, RmConnection *connection);
GtkWidget *phone_search_entry_new(GtkWidget *window, RmContact *contact, struct phone_state *state);
void phone_call_notify_cb(RmObject *object, RmCall *call, gint connection, gchar *medium, gpointer user_data);
void phone_setup_timer();
void phone_remove_timer();
GtkWidget *phone_dial_buttons_new(GtkWidget *window, struct phone_state *state);
void phone_dial_buttons_set_dial(gboolean allow);
GtkWidget *phone_window_new(enum phone_type type, RmContact *contact, RmConnection *connection, gpointer priv);

G_END_DECLS

#endif
