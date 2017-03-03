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

#ifndef __RM_PLUGINS_H
#define __RM_PLUGINS_H

G_BEGIN_DECLS

#define PLUGIN(NAME)\
	G_MODULE_EXPORT void __rm_init_plugin(RmPlugin *plugin) {\
		plugin->init = NAME##_plugin_init;\
		plugin->shutdown = NAME##_plugin_shutdown;\
	}

#define PLUGIN_CONFIG(NAME)\
	G_MODULE_EXPORT void __rm_init_plugin(RmPlugin *plugin) {\
		plugin->init = NAME##_plugin_init;\
		plugin->shutdown = NAME##_plugin_shutdown;\
		plugin->configure = NAME##_plugin_configure;\
	}

typedef struct _RmPlugin RmPlugin;
typedef gboolean (*initPlugin)(RmPlugin *plugin);
typedef gboolean (*shutdownPlugin)(RmPlugin *plugin);
typedef gpointer (*configurePlugin)(RmPlugin *plugin);

struct _RmPlugin {
	gchar *name;
	gchar *description;
	gchar *copyright;
	initPlugin init;
	shutdownPlugin shutdown;
	configurePlugin configure;

	gchar *module_name;
	gchar *help;
	gchar *homepage;
	gboolean builtin;
	gboolean enabled;
	gpointer priv;
	GModule *module;
};

void rm_plugins_init(void);
void rm_plugins_shutdown(void);
void rm_plugins_bind_loaded_plugins(void);

void rm_plugins_add_search_path(gchar *path);

GSList *rm_plugins_get(void);
void rm_plugins_disable(RmPlugin *plugin);
void rm_plugins_enable(RmPlugin *plugin);

G_END_DECLS

#endif
