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

#include <config.h>

#ifdef HAVE_DBUS
#include <string.h>

#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libroutermanager/call.h>
#include <libroutermanager/appobject-emit.h>

#include <roger/phone.h>

static GDBusNodeInfo *introspection_data = NULL;
static guint owner_id;

static const gchar introspection_xml[] =
	"<node>"
	"	<interface name='org.tabos.rogerInterface'>"
	"		<method name='Dial'>"
	"			<arg type='s' name='number' direction='in'/>"
	"		</method>"
	"	</interface>"
	"</node>";

static void handle_method_call(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
	if (g_strcmp0(method_name, "Dial") == 0) {
		struct contact contact_s;
		struct contact *contact = &contact_s;
		const gchar *number;
		gchar *full_number;

		g_variant_get(parameters, "(&s)", &number);

		full_number = call_full_number(number, FALSE);

		/** Ask for contact information */
		memset(contact, 0, sizeof(struct contact));
		contact_s.number = full_number;
		emit_contact_process(contact);

		app_show_phone_window(contact);
		g_free(full_number);
	}

	g_dbus_method_invocation_return_value(invocation, NULL);
}

static const GDBusInterfaceVTable interface_table =
{
	handle_method_call,
	NULL,
	NULL
};

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	g_dbus_connection_register_object(connection, "/org/tabos/rogerObject", introspection_data->interfaces[0], &interface_table, NULL, NULL, NULL);
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

gboolean dbus_init(void)
{
	introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	owner_id = g_bus_own_name(G_BUS_TYPE_SESSION, "org.tabos.roger", G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);

	return TRUE;
}

void dbus_shutdown(void)
{
	g_bus_unown_name(owner_id);
	g_dbus_node_info_unref (introspection_data);
}

#endif
