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

#include <string.h>

#include <glib.h>

#include <rmconfig.h>

#if HAVE_UUID
#include <uuid.h>
#endif

#if HAVE_DCONF
#include <dconf.h>
#endif

#include <rm/rmaction.h>
#include <rm/rmstring.h>
#include <rm/rmobject.h>
#include <rm/rmsettings.h>

/**
 * SECTION:rmaction
 * @title: RmAction
 * @short_description: User action (execute an application for a set of phone numbers)
 * @stability: Stable
 *
 * Actions are user defined reactions to specific call event types, e.g. stop music on incoming calls.
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
 * @event: a #RmConnectionType
 * @connection: a #RmConnection
 * @user_data: user data pointer to #RmProfile
 *
 * Connection-notify callback - based on connection type execute actions
 */
static void rm_action_connection_changed_cb(RmObject *object, gint event, RmConnection *connection, gpointer user_data)
{
	GSList *list;
	RmProfile *profile = user_data;

	/* Sanity check #1 - profile must be != NULL */
	g_return_if_fail(profile != NULL);
	/* Sanity check #2 - if local number is NULL there is no reason to execute specified actions */
	g_return_if_fail(!RM_EMPTY_STRING(connection->local_number));

	/* Loop through all actions within the profile */
	for (list = profile->action_list; list != NULL; list = list->next) {
		RmAction *action = list->data;
		gchar **numbers = rm_action_get_numbers(action);
		gchar flags = rm_action_get_flags(action);

		/* If connection number is not within the action or no flags are set, continue with next entry */
		if (!rm_action_get_flags(action) || !rm_strv_contains((const gchar * const *)numbers, connection->local_number)) {
			g_strfreev(numbers);
			continue;
		}
		g_strfreev(numbers);

		if (/* Incoming connection */
		    ((connection->type == RM_CONNECTION_TYPE_INCOMING) && (flags & RM_ACTION_INCOMING_RING)) ||
		    /* Outgoing connection */
		    ((connection->type == RM_CONNECTION_TYPE_OUTGOING) && (flags & RM_ACTION_OUTGOING_DIAL)) ||
		    /* Incoming connection missed */
		    ((connection->type == RM_CONNECTION_TYPE_MISSED) && (flags & RM_ACTION_INCOMING_MISSED)) ||
		    /* Incoming connection established */
		    ((connection->type == (RM_CONNECTION_TYPE_INCOMING | RM_CONNECTION_TYPE_CONNECT)) && (flags & RM_ACTION_INCOMING_BEGIN)) ||
		    /* Outgoing connection established */
		    ((connection->type == (RM_CONNECTION_TYPE_OUTGOING | RM_CONNECTION_TYPE_CONNECT)) && (flags & RM_ACTION_OUTGOING_BEGIN)) ||
		    /* Incoming connection terminated */
		    ((connection->type == (RM_CONNECTION_TYPE_INCOMING | RM_CONNECTION_TYPE_CONNECT | RM_CONNECTION_TYPE_DISCONNECT)) && (flags & RM_ACTION_INCOMING_END)) ||
		    /* Outgoing connection terminated */
		    ((connection->type == (RM_CONNECTION_TYPE_OUTGOING | RM_CONNECTION_TYPE_CONNECT | RM_CONNECTION_TYPE_DISCONNECT)) && (flags & RM_ACTION_OUTGOING_END))) {
			gchar *tmp = rm_action_regex(rm_action_get_exec(action), connection);

			g_debug("%s(): Action requested = '%s', executing = '%s'", __FUNCTION__, rm_action_get_exec(action), tmp);
			g_spawn_command_line_async(tmp, NULL);
			g_free(tmp);
		}
	}
}

/**
 * rm_action_set_path:
 * @action: a #RmAction
 * @path: new path of @action
 *
 * Set action path.
 */
static void rm_action_set_path(RmAction *action, const gchar *path)
{
	g_object_set_data(G_OBJECT(action), "path", g_strdup(path));
}

/**
 * rm_action_get_path:
 * @action: a #RmAction
 *
 * Returns: (transfer full) action path.
 */
static gchar *rm_action_get_path(RmAction *action)
{
	return g_object_get_data(G_OBJECT(action), "path");
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
	g_settings_set_string(action, "name", name);
}

/**
 * rm_action_get_name:
 * @action: a #RmAction
 *
 * Returns: (transfer full) action name.
 */
gchar *rm_action_get_name(RmAction *action)
{
	return g_settings_get_string(action, "name");
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
	g_settings_set_string(action, "description", description);
}

/**
 * rm_action_get_description:
 * @action: a #RmAction
 *
 * Returns: (transfer full) action description.
 */
gchar *rm_action_get_description(RmAction *action)
{
	return g_settings_get_string(action, "description");
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
	g_settings_set_string(action, "exec", exec);
}

/**
 * rm_action_get_exec:
 * @action: a #RmAction
 *
 * Returns: (transfer full) action exec.
 */
gchar *rm_action_get_exec(RmAction *action)
{
	return g_settings_get_string(action, "exec");
}

/**
 * rm_action_get_flags:
 * @action: a #RmAction
 *
 * Returns: action flags.
 */
guchar rm_action_get_flags(RmAction *action)
{
	return g_settings_get_int(action, "flags");
}

/**
 * rm_action_set_flags:
 * @action: a #RmAction
 * @flags: action flags
 *
 * Set action flags.
 */
void rm_action_set_flags(RmAction *action, guchar flags)
{
	g_settings_set_int(action, "flags", flags);
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
	g_settings_set_strv(action, "numbers", (const gchar * const *) numbers);
}

/**
 * rm_action_get_numbers:
 * @action: a #RmAction
 *
 * Returns: (transfer full) action numbers.
 */
gchar **rm_action_get_numbers(RmAction *action)
{
	return g_settings_get_strv(action, "numbers");
}

/**
 * rm_action_get_list:
 * @profile: a #RmProfile
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

		actions[counter++] = rm_action_get_path(current_action);
	}
	actions[counter] = NULL;

	/* Write actions to profile settings */
	g_settings_set_strv(profile->settings, "actions", (const gchar * const *)actions);

	/* Clean up */
	g_strfreev(actions);
}

/**
 * rm_action_load:
 * @profile: a #RmProfile
 * @name: name of action to load from @profile
 *
 * Load action @name of selected @profile.
 *
 * Returns: loaded #RmAction
 */
static RmAction *rm_action_load(RmProfile *profile, const gchar *name)
{
	RmAction *action;
	gchar *settings_path;

	/* concat the settings path */
	settings_path = g_strconcat("/org/tabos/rm/profile/", profile->name, "/", name, "/", NULL);

	/* Create/Read settings from path */
	action = rm_settings_new_with_path(RM_SCHEME_PROFILE_ACTION, settings_path);
	rm_action_set_path(action, name);

	/* Free settings path */
	g_free(settings_path);

	/* Add action to list */
	profile->action_list = g_slist_prepend(profile->action_list, action);

	return action;
}

static char *rand_string(gchar *str, size_t size)
{
	const gchar charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	if (size) {
		size_t n;

		--size;
		for (n = 0; n < size; n++) {
			int key = g_random_int() % (gint)(sizeof(charset) - 1);
			str[n] = charset[key];
		}

		str[size] = '\0';
	}

	return str;
}

char *rand_string_alloc(size_t size)
{
	char *s = g_malloc0(size + 1);

	rand_string(s, size);

	return s;
}

/**
 * rm_action_new:
 * @profile: a #RmProfile
 *
 * Creates and adds new action to @profile's action list.
 *
 * Returns: the new #RmAction
 */
RmAction *rm_action_new(RmProfile *profile)
{
	RmAction *action;
	gchar uuidstr[37];
#if HAVE_UUID
	uuid_t u;

	uuid_generate(u);
	uuid_unparse(u, uuidstr);

	action = rm_action_load(profile, uuidstr);
#else
	rand_string(uuidstr, sizeof(uuidstr));
	action = rm_action_load(profile, uuidstr);
#endif

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

#if HAVE_DCONF
	if (rm_settings_backend_is_dconf()) {
		gchar *path;
		DConfClient *client;

		path = g_strconcat("/org/tabos/rm/profile/", profile->name, "/", rm_action_get_path(action), "/", NULL);
		client = dconf_client_new ();
		dconf_client_write_sync (client, path, NULL, NULL, NULL, NULL);
		g_free(path);
		g_object_unref(client);
	}
#endif

	/* Free internal action data */
	g_object_unref(action);
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

	/* Connect to ::connection-changed signal */
	g_signal_connect(G_OBJECT(rm_object), "connection-changed", G_CALLBACK(rm_action_connection_changed_cb), profile);
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
	g_signal_handlers_disconnect_by_func(G_OBJECT(rm_object), G_CALLBACK(rm_action_connection_changed_cb), profile);

	/* Clear action list */
	g_slist_free_full(profile->action_list, g_object_unref);
	profile->action_list = NULL;
}
