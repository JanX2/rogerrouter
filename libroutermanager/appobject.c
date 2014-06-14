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

#include <appobject.h>

/**
 * \TODO List
 * - combine connection-XXX in connection-notify
 */

/** main internal app_object containing signals and private data */
GObject *app_object = NULL;

/** app_object signals array */
guint app_object_signals[ACB_MAX] = { 0 };

/** Private app_object data */
typedef struct _AppObjectPrivate AppObjectPrivate;

struct _AppObjectPrivate {
	gchar dummy;
};

G_DEFINE_TYPE(AppObject, app_object, G_TYPE_OBJECT);

/**
 * \brief Create internal app_object signals
 * \param g_object_class main object class
 */
static void app_object_create_signals(GObjectClass *g_object_class)
{
	app_object_signals[ACB_JOURNAL_LOADED] = g_signal_new(
	            "journal-loaded",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(AppObjectClass, journal_loaded),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__POINTER,
	            G_TYPE_NONE,
	            1,
	            G_TYPE_POINTER);

	app_object_signals[ACB_CONNECTION_NOTIFY] = g_signal_new(
	            "connection-notify",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(AppObjectClass, connection_notify),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__POINTER,
	            G_TYPE_NONE,
	            1,
	            G_TYPE_POINTER);

	app_object_signals[ACB_CONTACT_PROCESS] = g_signal_new(
	            "contact-process",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(AppObjectClass, contact_process),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__POINTER,
	            G_TYPE_NONE,
	            1,
	            G_TYPE_POINTER);

	app_object_signals[ACB_FAX_PROCESS] = g_signal_new(
	        "fax-process",
	        G_OBJECT_CLASS_TYPE(g_object_class),
	        G_SIGNAL_RUN_FIRST,
	        G_STRUCT_OFFSET(AppObjectClass, fax_process),
	        NULL,
	        NULL,
	        g_cclosure_marshal_VOID__POINTER,
	        G_TYPE_NONE,
	        1,
	        G_TYPE_POINTER);

	app_object_signals[ACB_CONNECTION_ESTABLISHED] = g_signal_new(
	            "connection-established",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(AppObjectClass, connection_established),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__POINTER,
	            G_TYPE_NONE,
	            1,
	            G_TYPE_POINTER);

	app_object_signals[ACB_CONNECTION_TERMINATED] = g_signal_new(
	            "connection-terminated",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(AppObjectClass, connection_terminated),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__POINTER,
	            G_TYPE_NONE,
	            1,
	            G_TYPE_POINTER);

	app_object_signals[ACB_CONNECTION_STATUS] = g_signal_new(
	            "connection-status",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(AppObjectClass, connection_status),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__UINT_POINTER,
	            G_TYPE_NONE,
	            2,
	            G_TYPE_UINT,
	            G_TYPE_POINTER);

	app_object_signals[ACB_MESSAGE] = g_signal_new(
	            "message",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(AppObjectClass, connection_status),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__UINT_POINTER,
	            G_TYPE_NONE,
	            2,
	            G_TYPE_UINT,
	            G_TYPE_POINTER);
}

/**
 * \brief Initialize app_object class
 * \param klass main class object
 */
static void app_object_class_init(AppObjectClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(AppObjectPrivate));
	app_object_create_signals(g_object_class);
}

/**
 * \brief Initialize app_object (does nothing ATM)
 * \param self pointer to app_object itself
 */
static void app_object_init(AppObject *self)
{
}

/**
 * \brief Create new app_object
 * \return new app_object
 */
GObject *app_object_new(void)
{
	return g_object_new(APP_OBJECT_TYPE, NULL);
}
