/**
 * The libroutermanager project
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#ifndef LIBROUTERMANAGER_RMPLUGINS_H
#define LIBROUTERMANAGER_RMPLUGINS_H

#include <libpeas/peas.h>

#ifdef GTK_MAJOR_VERSION
#include <libpeas-gtk/peas-gtk-configurable.h>
#endif

G_BEGIN_DECLS

/**
 * _RM_PLUGIN_REGISTER:
 * \param TYPE_NAME: the name of the plugin type, in UPPER_CASE
 * \param TypeName: the name of the plugin type, in CamelCase
 * \param type_name: the name of the plugin type, in lower_case
 * \param TYPE_CODE: code to go in the fifth parameter to %G_DEFINE_DYNAMIC_TYPE_EXTENDED
 * \param REGISTER_CODE: code to go in the peas_register_types() exported function
 *
 * Registers a plugin with the RouterManager plugin system, including registering the type specified in the parameters and declaring its activate and
 * deactivate functions.
 **/
#define _RM_PLUGIN_REGISTER(TYPE_NAME, TypeName, type_name, TYPE_CODE, REGISTER_CODE)	\
	typedef struct {							\
		PeasExtensionBaseClass parent_class;				\
	} TypeName##Class;							\
	typedef struct {							\
		PeasExtensionBase parent;					\
		TypeName##Private *priv;					\
	} TypeName;								\
	GType type_name##_get_type (void) G_GNUC_CONST;				\
	static void impl_activate (PeasActivatable *plugin);			\
	static void impl_deactivate (PeasActivatable *plugin);			\
	G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);	\
	static void peas_activatable_iface_init (PeasActivatableInterface *iface); \
	enum {									\
		PROP_0,								\
		PROP_OBJECT							\
	};									\
	G_DEFINE_DYNAMIC_TYPE_EXTENDED (TypeName,				\
					type_name,				\
					PEAS_TYPE_EXTENSION_BASE,		\
					0,					\
					G_IMPLEMENT_INTERFACE_DYNAMIC (PEAS_TYPE_ACTIVATABLE, \
								       peas_activatable_iface_init) \
					TYPE_CODE)				\
	static void								\
	peas_activatable_iface_init (PeasActivatableInterface *iface)		\
	{									\
		iface->activate = impl_activate;				\
		iface->deactivate = impl_deactivate;				\
	}									\
	static void								\
	set_property (GObject      *object,					\
		      guint         prop_id,					\
		      const GValue *value,					\
		      GParamSpec   *pspec)					\
	{									\
		switch (prop_id) {						\
		case PROP_OBJECT:						\
			g_object_set_data_full (object, "object",		\
						g_value_dup_object (value),	\
						g_object_unref);		\
			break;							\
		default:							\
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec); \
			break;							\
		}								\
	}									\
	static void								\
	get_property (GObject    *object,					\
		      guint       prop_id,					\
		      GValue     *value,					\
		      GParamSpec *pspec)					\
	{									\
		switch (prop_id) {						\
		case PROP_OBJECT:						\
			g_value_set_object (value, g_object_get_data (object, "object")); \
			break;							\
		default:							\
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec); \
			break;							\
		}								\
	}									\
	static void								\
	type_name##_class_init (TypeName##Class *klass)				\
	{									\
		GObjectClass *object_class = G_OBJECT_CLASS (klass);		\
										\
		object_class->set_property = set_property;			\
		object_class->get_property = get_property;			\
										\
		g_object_class_override_property (object_class, PROP_OBJECT, "object"); \
		g_type_class_add_private (klass, sizeof (TypeName##Private));	\
	}									\
	static void								\
	type_name##_class_finalize (TypeName##Class *klass)			\
	{									\
	}									\
	static void								\
	type_name##_init (TypeName *plugin)					\
	{									\
		plugin->priv = G_TYPE_INSTANCE_GET_PRIVATE (plugin, TYPE_NAME, TypeName##Private); \
	}									\
	G_MODULE_EXPORT void							\
	peas_register_types (PeasObjectModule *module)				\
	{									\
		type_name##_register_type (G_TYPE_MODULE (module));		\
		peas_object_module_register_extension_type (module,		\
							    PEAS_TYPE_ACTIVATABLE, \
							    TYPE_NAME);		\
		REGISTER_CODE							\
	}

/**
 * RM_PLUGIN_REGISTER:
 * \param TYPE_NAME: the name of the plugin type, in UPPER_CASE
 * \param TypeName: the name of the plugin type, in CamelCase
 * \param type_name: the name of the plugin type, in lower_case
 *
 * Registers a plugin with the RouterManager plugin system, including registering the type specified in the parameters and declaring its activate and
 * deactivate functions.
 **/
#define RM_PLUGIN_REGISTER(TYPE_NAME, TypeName, type_name)			\
	_RM_PLUGIN_REGISTER(TYPE_NAME, TypeName, type_name,,)

/**
 * RM_PLUGIN_REGISTER_CONFIGURABLE:
 * \param TYPE_NAME: the name of the plugin type, in UPPER_CASE
 * \param TypeName: the name of the plugin type, in CamelCase
 * \param type_name: the name of the plugin type, in lower_case
 *
 * Registers a configurable plugin with the RouterManager plugin system, including registering the type specified in the parameters and declaring its activate
 * and deactivate and widget creation functions.
 **/
#define RM_PLUGIN_REGISTER_CONFIGURABLE(TYPE_NAME, TypeName, type_name)	\
	static GtkWidget *impl_create_configure_widget (PeasGtkConfigurable *configurable); \
	static void peas_gtk_configurable_iface_init (PeasGtkConfigurableInterface *iface); \
	_RM_PLUGIN_REGISTER(TYPE_NAME, TypeName, type_name,			\
		(G_IMPLEMENT_INTERFACE_DYNAMIC (PEAS_GTK_TYPE_CONFIGURABLE, peas_gtk_configurable_iface_init)), \
		peas_object_module_register_extension_type (module, PEAS_GTK_TYPE_CONFIGURABLE, TYPE_NAME);) \
	static void								\
	peas_gtk_configurable_iface_init (PeasGtkConfigurableInterface *iface)	\
	{									\
		iface->create_configure_widget = impl_create_configure_widget;	\
	}

extern PeasEngine *rm_engine;

void rm_plugins_init(void);
void rm_plugins_shutdown(void);
void rm_plugins_bind_loaded_plugins(void);

void rm_plugins_add_search_path(gchar *path);

G_END_DECLS

#endif