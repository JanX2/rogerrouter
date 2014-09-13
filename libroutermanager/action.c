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

#include <string.h>

#include <glib.h>

#include <libroutermanager/action.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/settings.h>

/**
 * \brief Parse the input string and replaces templates with connection information
 * \param str input string
 * \param connection connection structure
 * \return regex modified string
 */
static gchar *action_regex(gchar *str, struct connection *connection)
{
	GRegex *line = g_regex_new("%LINE%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	GRegex *number = g_regex_new("%NUMBER%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	GRegex *name = g_regex_new("%NAME%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	GRegex *company = g_regex_new("%COMPANY%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	struct contact *contact;
	gchar *tmp0;
	gchar *tmp1;
	gchar *tmp2;
	gchar *out;

	/* Replace number template */
	tmp0 = g_regex_replace_literal(number, str, -1, 0, connection->remote_number, 0, NULL);
	/* Replace line template */
	tmp1 = g_regex_replace_literal(line, tmp0, -1, 0, connection->local_number, 0, NULL);

	/** Based on connection ask for contact information */
	contact = contact_find_by_number(connection->remote_number);

	/* Replace company template */
	tmp2 = g_regex_replace_literal(company, tmp1, -1, 0, contact->company != NULL ? contact->company : "", 0, NULL);
	/* Replace name template */
	out = g_regex_replace_literal(name, tmp2, -1, 0, contact->name != NULL ? contact->name : "", 0, NULL);

	/* Free temporary data fields */
	g_free(tmp2);
	g_free(tmp1);
	g_free(tmp0);

	/* Unref regex */
	g_regex_unref(line);
	g_regex_unref(number);
	g_regex_unref(name);
	g_regex_unref(company);

	return out;
}

/**
 * \brief Check if local_number is within numbers string vector
 * \param local_number local number to check
 * \param numbers string vector
 * \return TRUE if number is found, otherwise FALSE
 */
static inline gboolean action_check_number(const gchar *local_number, gchar **numbers)
{
	guint i;

	for (i = 0; i < g_strv_length(numbers); i++) {
		if (!strcmp(local_number, numbers[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * \brief connection-notify callback - based on connection type execute actions
 * \param object app object
 * \param connection connection structure
 * \param user_data profile pointer
 */
void action_connection_notify_cb(AppObject *object, struct connection *connection, gpointer user_data)
{
	GSList *list;
	struct profile *profile = user_data;

	/* Sanity check #1 - profile must be != NULL */
	g_return_if_fail(profile != NULL);
	/* Sanity check #2 - if local number is NULL there is no reason to execute MSN specified actions */
	g_return_if_fail(!EMPTY_STRING(connection->local_number));

	/* Loop through all actions within the profile */
	for (list = profile->action_list; list; list = list->next) {
		struct action *action = list->data;

		/* If connection number is not within the action lis or no flags are sett, continue with next entry */
		if (!action_check_number(connection->local_number, action->numbers) || !action->flags) {
			continue;
		}

#if ACTION_DEBUG
		g_debug("Possible action: %s, %s, %s, %d", action->name, action->description, action->exec, action->flags);
		g_debug("Connection type: %d", connection->call_type);
#endif

		if (/* Incoming connection */
		    ((connection->type == CONNECTION_TYPE_INCOMING) && (action->flags & ACTION_INCOMING_RING)) ||
		    /* Outgoing connection */
		    ((connection->type == CONNECTION_TYPE_OUTGOING) && (action->flags & ACTION_OUTGOING_DIAL)) ||
		    /* Incoming connection missed */
		    ((connection->type == CONNECTION_TYPE_MISS) && (action->flags & ACTION_INCOMING_MISSED)) ||
		    /* Incoming connection established */
		    ((connection->type == (CONNECTION_TYPE_INCOMING | CONNECTION_TYPE_CONNECT)) && (action->flags & ACTION_INCOMING_BEGIN)) ||
		    /* Outgoing connection established */
		    ((connection->type == (CONNECTION_TYPE_OUTGOING | CONNECTION_TYPE_CONNECT)) && (action->flags & ACTION_OUTGOING_BEGIN)) ||
		    /* Incoming connection terminated */
		    ((connection->type == (CONNECTION_TYPE_INCOMING | CONNECTION_TYPE_CONNECT | CONNECTION_TYPE_DISCONNECT)) && (action->flags & ACTION_INCOMING_END)) ||
		    /* Outgoing connection terminated */
		    ((connection->type == (CONNECTION_TYPE_OUTGOING | CONNECTION_TYPE_CONNECT | CONNECTION_TYPE_DISCONNECT)) && (action->flags & ACTION_OUTGOING_END))) {
			gchar *tmp = action_regex(action->exec, connection);

			g_debug("Action requested: '%s', executing '%s'", action->exec, tmp);
			g_spawn_command_line_async(tmp, NULL);
			g_free(tmp);
		} else {
			//g_debug("Case within action connection-notify callback:");
			//g_debug("Action: %s, %s, %s, %d", action->name, action->description, action->exec, action->flags);
			//g_debug("Connection type: %d", connection->type);
		}
	}
}

/**
 * \brief Create new action structure
 * \return new action structure pointer
 */
struct action *action_create(void)
{
	struct action *action = g_slice_new(struct action);

	memset(action, 0, sizeof(struct action));

	return action;
}

/**
 * \brief Modify action structure
 * \param name name of the new action
 * \param description action description
 * \param exec application execution line
 * \param numbers numbers the apply to this action
 * \return new action structure pointer
 */
struct action *action_modify(struct action *action, const gchar *name, const gchar *description, const gchar *exec, gchar **numbers)
{
	gchar *settings_path;
	gchar *filename;

	action->name = g_strdup(name);
	action->description = g_strdup(description);
	action->exec = g_strdup(exec);
	action->numbers = numbers;

	/* Setup action settings */
	settings_path = g_strconcat("/org/tabos/routermanager/profile/", profile_get_active()->name, "/", name, "/", NULL);
	filename = g_strconcat("actions/", profile_get_active()->name, "-", name, NULL);
	action->settings = rm_settings_new_with_path(ROUTERMANAGER_SCHEME_PROFILE_ACTION, settings_path, filename);
	g_free(filename);

	g_settings_set_string(action->settings, "name", action->name);
	g_settings_set_string(action->settings, "description", action->description);
	g_settings_set_string(action->settings, "exec", action->exec);
	g_settings_set_int(action->settings, "flags", action->flags);
	g_settings_set_strv(action->settings, "numbers", (const gchar * const *) action->numbers);

	g_free(settings_path);

	return action;
}

/**
 * \brief Retrieve action list of profile
 * \param profile profile structure
 * \return action list
 */
GSList *action_get_list(struct profile *profile)
{
	return profile->action_list;
}

/**
 * \brief Commit action - write action names to profile settings
 * \param profile profile pointer
 */
void action_commit(struct profile *profile)
{
	GSList *list;
	gchar **actions = g_new0(gchar *, g_slist_length(profile->action_list) + 1);
	gsize counter = 0;

	for (list = profile->action_list; list; list = list->next) {
		struct action *current_action = list->data;

		actions[counter++] = current_action->name;
	}
	actions[counter] = NULL;

	g_settings_set_strv(profile->settings, "actions", (const gchar * const *)actions);

	//g_strfreev(actions);
}

/**
 * \brief Add action to action list of given profile
 * \param profile profile pointer
 * \param action action to add to profile action list
 */
void action_add(struct profile *profile, struct action *action)
{
	/* Add action to profile action list */
	profile->action_list = g_slist_prepend(profile->action_list, action);
}

/**
 * \brief Remove action of profile action list
 * \param profile profile pointer
 * \param action action structure to remove
 */
void action_remove(struct profile *profile, struct action *action)
{
	/* Remove profile from action list */
	profile->action_list = g_slist_remove(profile->action_list, action);
}

/**
 * \brief Free action data
 * \param data action structure pointer
 */
void action_free(gpointer data)
{
	struct action *action = data;

	/* Free action data */
	g_free(action->name);
	g_free(action->description);
	g_free(action->exec);
	g_strfreev(action->numbers);

	g_slice_free(struct action, action);
}

/**
 * \brief Load action 'name' of selected profile
 * \param profile profile pointer
 * \param name action name
 */
static void action_load(struct profile *profile, const gchar *name)
{
	struct action *action;
	gchar *settings_path;
	gchar *filename;

	/* Allocate fixed struct */
	action = g_slice_new0(struct action);

	/* concat the settings path */
	settings_path = g_strconcat("/org/tabos/routermanager/profile/", profile->name, "/", name, "/", NULL);

	/* Create/Read settings from path */
	filename = g_strconcat("actions/", profile->name, "-", name, NULL);
	action->settings = rm_settings_new_with_path(ROUTERMANAGER_SCHEME_PROFILE_ACTION, settings_path, filename);
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
	action_add(profile, action);
}

/**
 * \brief Initialize action structure - load actions selected by profile and connect to connection-notify signal
 * \param profile profile pointer
 */
void action_init(struct profile *profile)
{
	gchar **actions;
	guint count;
	guint index;

	/* Based upon the profile settings load the action list */
	actions = g_settings_get_strv(profile->settings, "actions");

	/* Load all available actions */
	count = g_strv_length(actions);
	for (index = 0; index < count; index++) {
		action_load(profile, actions[index]);
	}

	g_strfreev(actions);

	/* Connect to connection-notify signal */
	g_signal_connect(G_OBJECT(app_object), "connection-notify", G_CALLBACK(action_connection_notify_cb), profile);
}

/**
 * \brief Shutdown action list within given profile
 * \param profile profile structure
 */
void action_shutdown(struct profile *profile)
{
	/* Disconnect connection-notify signal */
	g_signal_handlers_disconnect_by_func(G_OBJECT(app_object), G_CALLBACK(action_connection_notify_cb), profile);

	/* Clear action list */
	g_slist_free_full(profile->action_list, action_free);
}
