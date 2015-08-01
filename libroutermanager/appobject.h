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

#ifndef LIBROUTERMANAGER_APPOBJECT_H
#define LIBROUTERMANAGER_APPOBJECT_H

#include <glib.h>
#include <glib-object.h>

#include <libroutermanager/connection.h>
#include <libroutermanager/libfaxophone/faxophone.h>

G_BEGIN_DECLS

typedef enum {
	ACB_JOURNAL_LOADED,
	ACB_CONNECTION_NOTIFY,
	ACB_CONTACT_PROCESS,
	ACB_FAX_PROCESS,
	ACB_CONNECTION_ESTABLISHED,
	ACB_CONNECTION_TERMINATED,
	ACB_CONNECTION_STATUS,
	ACB_MESSAGE,
	ACB_CONTACTS_CHANGED,
	ACB_MAX
} AppCallbackId;

#define APP_OBJECT_TYPE (app_object_get_type())
#define APP_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GEANY_OBJECT_TYPE, AppObject))
#define APP_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), APP_OBJECT_TYPE, AppObjectClass))
#define IS_APP_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), APP_OBJECT_TYPE))
#define IS_APP_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), APP_OBJECT_TYPE))

typedef struct _AppObject AppObject;
typedef struct _AppObjectClass AppObjectClass;

struct _AppObject {
	GObject parent;
};

struct _AppObjectClass {
	GObjectClass parent_class;
	void (*journal_loaded)(GSList *journal);
	void (*connection_notify)(struct connection *connection);
	void (*contact_process)(struct contact *contact);
	void (*fax_process)(const gchar *filename);
	void (*connection_established)(struct capi_connection *connection);
	void (*connection_terminated)(struct capi_connection *connection);
	void (*connection_status)(gint status, struct capi_connection *connection);
	void (*message)(gint type, gchar *message);
	void (*contacts_changed)(void);
};

GType app_object_get_type(void);
GObject *app_object_new(void);

extern GObject *app_object;
extern guint app_object_signals[ACB_MAX];

G_END_DECLS

#endif
