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

#include <rm/rmobject.h>

/*
 * TODO List
 * - combine connection-XXX in connection-notify
 */

/** main internal rm_object containing signals and private data */
GObject *rm_object = NULL;

/** rm_object signals array */
guint rm_object_signals[RM_ACB_MAX] = { 0 };

/** Private rm_object data */
typedef struct _RmObjectPrivate RmObjectPrivate;

struct _RmObjectPrivate {
	gchar dummy;
};

G_DEFINE_TYPE(RmObject, rm_object, G_TYPE_OBJECT);

#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer

static void marshal_VOID__POINTER_POINTER(GClosure *closure, GValue *return_value, guint n_param_values, const GValue *param_values, gpointer invocation_hint, gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID_POINTER_POINTER)(gpointer data1, gpointer arg_1, gpointer arg_2, gpointer data2);
	register GMarshalFunc_VOID_POINTER_POINTER callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail(n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA(closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer(param_values + 0);
	} else {
		data1 = g_value_peek_pointer(param_values + 0);
		data2 = closure->data;
	}

	callback = (GMarshalFunc_VOID_POINTER_POINTER)(marshal_data ? marshal_data : cc->callback);

	callback(data1, g_marshal_value_peek_pointer(param_values + 1), g_marshal_value_peek_pointer(param_values + 2), data2);
}

/**
 * rm_object_create_signals:
 * g_object_class: a #GObjectClass
 *
 * Create internal rm_object signals.
 */
static void rm_object_create_signals(GObjectClass *g_object_class)
{
	rm_object_signals[RM_ACB_JOURNAL_LOADED] = g_signal_new(
	            "journal-loaded",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(RmObjectClass, journal_loaded),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__POINTER,
	            G_TYPE_NONE,
	            1,
	            G_TYPE_POINTER);

	rm_object_signals[RM_ACB_CONNECTION_CHANGED] = g_signal_new(
	            "connection-changed",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(RmObjectClass, connection_changed),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__UINT_POINTER,
	            G_TYPE_NONE,
	            2,
	            G_TYPE_UINT,
	            G_TYPE_POINTER);

	rm_object_signals[RM_ACB_CONTACT_PROCESS] = g_signal_new(
	            "contact-process",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(RmObjectClass, contact_process),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__POINTER,
	            G_TYPE_NONE,
	            1,
	            G_TYPE_POINTER);

	rm_object_signals[RM_ACB_FAX_PROCESS] = g_signal_new(
	            "fax-process",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(RmObjectClass, fax_process),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__POINTER,
	            G_TYPE_NONE,
	            1,
	            G_TYPE_POINTER);

	rm_object_signals[RM_ACB_CONNECTION_STATUS] = g_signal_new(
	            "connection-status",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(RmObjectClass, connection_status),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__UINT_POINTER,
	            G_TYPE_NONE,
	            2,
	            G_TYPE_UINT,
	            G_TYPE_POINTER);

	rm_object_signals[RM_ACB_MESSAGE] = g_signal_new(
	            "message",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(RmObjectClass, message),
	            NULL,
	            NULL,
	            marshal_VOID__POINTER_POINTER,
	            G_TYPE_NONE,
	            2,
	            G_TYPE_POINTER,
	            G_TYPE_POINTER);

	rm_object_signals[RM_ACB_CONTACTS_CHANGED] = g_signal_new(
	            "contacts-changed",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(RmObjectClass, contacts_changed),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__VOID,
	            G_TYPE_NONE,
	            0,
	            G_TYPE_NONE);

	rm_object_signals[RM_ACB_AUTHENTICATE] = g_signal_new(
	            "authenticate",
	            G_OBJECT_CLASS_TYPE(g_object_class),
	            G_SIGNAL_RUN_FIRST,
	            G_STRUCT_OFFSET(RmObjectClass, authenticate),
	            NULL,
	            NULL,
	            g_cclosure_marshal_VOID__POINTER,
	            G_TYPE_NONE,
	            1,
	            G_TYPE_POINTER);
}

/**
 * rm_object_class_init:
 * @klass: a #RmObjectClass
 *
 * Initialize rm_object class.
 */
static void rm_object_class_init(RmObjectClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);

	//g_type_class_add_private(klass, sizeof(RmObjectPrivate));
	rm_object_create_signals(g_object_class);
}

/**
 * rm_object_init:
 * @self: a #RmObject
 *
 * Initialize rm_object (does nothing ATM).
 *
 */
static void rm_object_init(RmObject *self)
{
}

/**
 * rm_object_new:
 *
 * Create new rm_object.
 *
 * Returns: new rm_object.
 */
GObject *rm_object_new(void)
{
	return g_object_new(RM_OBJECT_TYPE, NULL);
}
