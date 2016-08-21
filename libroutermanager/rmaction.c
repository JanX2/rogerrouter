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

#include <string.h>

#include <glib.h>

#include <libroutermanager/rmaction.h>
#include <libroutermanager/rmconnection.h>
#include <libroutermanager/rmobject.h>
#include <libroutermanager/rmobjectemit.h>
#include <libroutermanager/rmmain.h>
#include <libroutermanager/rmstring.h>
#include <libroutermanager/rmsettings.h>

/**
 * SECTION:rmaction
 * @title: RmAction
 * @short_description: Action handling functions
 * @stability: Stable
 *
 * Actions are user defined reactions to specific call event types. E.g. stop music on incoming calls.
 */

/**
 * rm_action_eval_cb:
 * @info: @ a #GMatchInfo
 * @res: result string
 * @data: user data pointing to action hash table
 *
 * Replaces match string with corresponding entry of hash table
 *
 * Returns: FALSE
 */
static gboolean rm_action_eval_cb(const GMatchInfo *info, GString *res, gpointer data)
{
	gchar *match;
	gchar *r;

	match = g_match_info_fetch(info, 0);
	r = g_hash_table_lookup(data, match);
	g_string_append(res, r);
	g_free(match);

	return FALSE;
}

/**
 * rm_action_regex:
 * @str input string
 * @connection connection structure
 *
 * Parse the input string and replaces templates with connection information
 *
 * Returns: new allocated regex modified string
 */
gchar *rm_action_regex(gchar *str, RmConnection *connection)
{
	GHashTable *h;
	GRegex *reg;
	gchar *res;
	RmContact *contact;

	h = g_hash_table_new(g_str_hash, g_str_equal);

	g_hash_table_insert(h, "%LINE%", connection->local_number);
	g_hash_table_insert(h, "%NUMBER%", connection->remote_number);

	/* Based on connection ask for contact information */
	contact = rm_contact_find_by_number(connection->remote_number);

	if (contact) {
		g_hash_table_insert(h, "%NAME%", contact->name ? contact->name : "");
		g_hash_table_insert(h, "%COMPANY%", contact->company ? contact->company : "");
	} else {
		g_hash_table_insert(h, "%NAME%", "");
		g_hash_table_insert(h, "%COMPANY%", "");
	}

	reg = g_regex_new("%LINE%|%NUMBER%|%NAME%|%COMPANY%", 0, 0, NULL);
	res = g_regex_replace_eval(reg, str, -1, 0, 0, rm_action_eval_cb, h, NULL);
	g_hash_table_destroy(h);

	return res;
}

/**
 * rm_action_connection_notify_cb:
 * @object: an #RmObject
 * @connection: a #RmConnection
 * @user_data: user data pointer to #RmProfile
 *
 * \brief connection-notify callback - based on connection type execute actions
 */
static void rm_action_connection_notify_cb(RmObject *object, RmConnection *connection, gpointer user_data)
{
	GSList *list;
	RmProfile *profile = user_data;

	/* Sanity check #1 - profile must be != NULL */
	g_return_if_fail(profile != NULL);
	/* Sanity check #2 - if local number is NULL there is no reason to execute specified actions */
	g_return_if_fail(!RM_EMPTY_STRING(connection->local_number));

	/* Loop through all actions within the profile */
	for (list = profile->action_list; list != NULL; list = list->next) {
		struct rm_action *action = list->data;

		/* If connection number is not within the action or no flags are set, continue with next entry */
		if (!action->flags || !rm_strv_contains(action->numbers, connection->local_number)) {
			continue;
		}

		if (/* Incoming connection */
		    ((connection->type == RM_CONNECTION_TYPE_INCOMING) && (action->flags & RM_ACTION_INCOMING_RING)) ||
		    /* Outgoing connection */
		    ((connection->type == RM_CONNECTION_TYPE_OUTGOING) && (action->flags & RM_ACTION_OUTGOING_DIAL)) ||
		    /* Incoming connection missed */
		    ((connection->type == RM_CONNECTION_TYPE_MISSED) && (action->flags & RM_ACTION_INCOMING_MISSED)) ||
		    /* Incoming connection established */
		    ((connection->type == (RM_CONNECTION_TYPE_INCOMING | RM_CONNECTION_TYPE_CONNECT)) && (action->flags & RM_ACTION_INCOMING_BEGIN)) ||
		    /* Outgoing connection established */
		    ((connection->type == (RM_CONNECTION_TYPE_OUTGOING | RM_CONNECTION_TYPE_CONNECT)) && (action->flags & RM_ACTION_OUTGOING_BEGIN)) ||
		    /* Incoming connection terminated */
		    ((connection->type == (RM_CONNECTION_TYPE_INCOMING | RM_CONNECTION_TYPE_CONNECT | RM_CONNECTION_TYPE_DISCONNECT)) && (action->flags & RM_ACTION_INCOMING_END)) ||
		    /* Outgoing connection terminated */
		    ((connection->type == (RM_CONNECTION_TYPE_OUTGOING | RM_CONNECTION_TYPE_CONNECT | RM_CONNECTION_TYPE_DISCONNECT)) && (action->flags & RM_ACTION_OUTGOING_END))) {
			gchar *tmp = rm_action_regex(action->exec, connection);

			g_debug("Action requested: '%s', executing '%s'", action->exec, tmp);
			g_spawn_command_line_async(tmp, NULL);
			g_free(tmp);
		}
	}
}

/**
 * rm_action_set_name:
 * @action: a #RmAction
 * @name: new name of @action
 *
 * Set action name.
 */
void rm_action_set_name(RmAction *action, const gchar *name)
{
	RmProfile *profile = rm_profile_get_active();
	gchar *settings_path;
	gchar *filename;

	action->name = g_strdup(name);

	/* Setup action settings */
	settings_path = g_strconcat("/org/tabos/routermanager/profile/", profile->name, "/", name, "/", NULL);
	filename = g_strconcat("actions/", profile->name, "-", name, NULL);
	action->settings = rm_settings_new_with_path(RM_SCHEME_PROFILE_ACTION, settings_path, filename);
	g_free(filename);

	/* Set internal data to settings */
	g_settings_set_string(action->settings, "name", action->name);
	g_settings_set_string(action->settings, "description", action->description);
	g_settings_set_string(action->settings, "exec", action->exec);
	g_settings_set_int(action->settings, "flags", action->flags);
	g_settings_set_strv(action->settings, "numbers", (const gchar * const *) action->numbers);

	g_free(settings_path);
}

/**
 * rm_action_get_name:
 * @action: a #RmAction
 *
 * Returns: (transfer full) action name.
 */
gchar *rm_action_get_name(RmAction *action)
{
	return g_strdup(action->name);
}

/**
 * rm_action_set_description:
 * @action: a #RmAction
 * @description: new description of @action
 *
 * Set action description.
 */
void rm_action_set_description(RmAction *action, const gchar *description)
{
	action->description = g_strdup(description);
	g_settings_set_string(action->settings, "description", action->description);
}

/**
 * rm_action_get_description:
 * @action: a #RmAction
 *
 * Returns: (transfer full) action description.
 */
gchar *rm_action_get_description(RmAction *action)
{
	return g_strdup(action->description);
}

/**
 * rm_action_set_exec:
 * @action: a #RmAction
 * @exec: new exec of @action
 *
 * Set action exec.
 */
void rm_action_set_exec(RmAction *action, const gchar *exec)
{
	action->exec = g_strdup(exec);
	g_settings_set_string(action->settings, "exec", action->exec);
}

/**
 * rm_action_get_exec:
 * @action: a #RmAction
 *
 * Returns: (transfer full) action exec.
 */
gchar *rm_action_get_exec(RmAction *action)
{
	return g_strdup(action->exec);
}

/**
 * rm_action_set_numbers:
 * @action: a #RmAction
 * @numbers: new numbers of @action
 *
 * Set action numbers.
 */
void rm_action_set_numbers(RmAction *action, const gchar **numbers)
{
	action->numbers = g_strdupv((gchar**)numbers);
	g_settings_set_string(action->settings, "numbers", action->numbers);
}

/**
 * rm_action_get_numbers:
 * @action: a #RmAction
 *
 * Returns: (transfer full) action numbers.
 */
gchar **rm_action_get_numbers(RmAction *action)
{
	return g_strdupv(action->numbers);
}

/**
 * rm_action_get_list:
 * @profile; a #RmProfile
 *
 * Retrieve action list of given @profile.
 *
 * Returns: profiles action list
 */
GSList *rm_action_get_list(RmProfile *profile)
{
	return profile->action_list;
}

/**
 * rm_action_save:
 * @profile: a #RmProfile
 *
 * Writes action list to profile settings
 */
static void rm_action_save(RmProfile *profile)
{
	GSList *list;
	gchar **actions = g_new0(gchar *, g_slist_length(profile->action_list) + 1);
	gsize counter = 0;

	/* Traverse through profile action list and add it to string array actions */
	for (list = profile->action_list; list; list = list->next) {
		RmAction *current_action = list->data;

		actions[counter++] = g_strdup(current_action->name);
	}
	actions[counter] = NULL;

	/* Write actions to profile settings */
	g_settings_set_strv(profile->settings, "actions", (const gchar * const *)actions);

	/* Clean up */
	g_strfreev(actions);
}

/**
 * rm_action_free:
 * @data: pointer to a #RmAction

 * \brief Free action data
 */
static void rm_action_free(gpointer data)
{
	RmAction *action = data;

	/* Free action data */
	if (action->name) {
		g_free(action->name);
		action->name = NULL;
	}
	if (action->description) {
		g_free(action->description);
		action->description = NULL;
	}
	if (action->exec) {
		g_free(action->exec);
		action->exec = NULL;
	}

	if (action->numbers) {
		g_strfreev(action->numbers);
		action->numbers = NULL;
	}

	g_slice_free(RmAction, action);
}

/**
 * rm_action_add:
 * @profile: a #RmProfile
 * @action_name: a name for new #RmAction
 *
 * Creates and adds action with given @action_name to @profile's internal action list, afterwards writes actions to @profile settings.
 *
 * Returns: the new #RmAction
 */
RmAction *rm_action_add(RmProfile *profile, const gchar *action_name)
{
	RmAction *action;

	/* Allocate fixed struct */
	action = g_slice_new0(RmAction);

	/* Set name */
	action->name = g_strdup(action_name);

	/* Add action to profile action list */
	profile->action_list = g_slist_prepend(profile->action_list, action);

	/* Save all actions to profile */
	rm_action_save(profile);

	return action;
}

/**
 * rm_action_remove:
 * @profile: a #RmProfile
 * @action: a #RmAction to remove
 *
 * Remove @action of @profile action list.
 */
void rm_action_remove(RmProfile *profile, RmAction *action)
{
	/* Remove profile from action list */
	profile->action_list = g_slist_remove(profile->action_list, action);

	/* Save all actions to profile */
	rm_action_save(profile);

	/* Free internal action data */
	rm_action_free(action);
}

/**
 * rm_action_load:
 * @profile: a #RmProfile
 * @name: name of action to load from @profile
 *
 * Load action @name of selected @profile.
 */
static void rm_action_load(RmProfile *profile, const gchar *name)
{
	RmAction *action;
	gchar *settings_path;
	gchar *filename;

	/* Allocate fixed struct */
	action = g_slice_new0(RmAction);

	/* concat the settings path */
	settings_path = g_strconcat("/org/tabos/routermanager/profile/", profile->name, "/", name, "/", NULL);

	/* Create/Read settings from path */
	filename = g_strconcat("actions/", profile->name, "-", name, NULL);
	action->settings = rm_settings_new_with_path(RM_SCHEME_PROFILE_ACTION, settings_path, filename);
	g_free(filename);

	/* Read internal data */
	action->name = g_settings_get_string(action->settings, "name");
	action->description = g_settings_get_string(action->settings, "description");
	action->exec = g_settings_get_string(action->settings, "exec");
	action->flags = g_settings_get_int(action->settings, "flags");
	action->numbers = g_settings_get_strv(action->settings, "numbers");

	/* Free settings path */
	g_free(settings_path);

	/* Add action to list */
	profile->action_list = g_slist_prepend(profile->action_list, action);
}

/**
 * rm_action_init:
 * @profile: a #RmProfile
 *
 * Initialize actions - load actions selected by profile and connect to ::connection-notify signal
 */
void rm_action_init(RmProfile *profile)
{
	gchar **actions;
	guint count;
	guint index;

	/* Based upon the profile settings load the action list */
	actions = g_settings_get_strv(profile->settings, "actions");

	/* Load all available actions */
	count = g_strv_length(actions);
	for (index = 0; index < count; index++) {
		rm_action_load(profile, actions[index]);
	}

	/* Free actions */
	g_strfreev(actions);

	/* Connect to ::connection-notify signal */
	g_signal_connect(G_OBJECT(rm_object), "connection-notify", G_CALLBACK(rm_action_connection_notify_cb), profile);
}

/**
 * rm_action_shutdown:
 * @profile: a #RmProfile
 *
 * Shutdown actions within given @profile.
 */
void rm_action_shutdown(RmProfile *profile)
{
	if (!profile) {
		return;
	}

	/* Disconnect connection-notify signal */
	g_signal_handlers_disconnect_by_func(G_OBJECT(rm_object), G_CALLBACK(rm_action_connection_notify_cb), profile);

	/* Clear action list */
	g_slist_free_full(profile->action_list, rm_action_free);
	profile->action_list = NULL;
}
