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

/**
 * SECTION:rmfilter
 * @title: RmFilter
 * @short_description: Filter handling functions
 *
 * Filtering of calls based on logical and connections
 */

/**
 * TODO:
 *  - add or- combination
 */

#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include <rm/rmfile.h>
#include <rm/rmcallentry.h>
#include <rm/rmmain.h>
#include <rm/rmstring.h>
#include <rm/rmfilter.h>

/**
 * rm_filter_compare:
 * @rule: a #RmFilterRule
 * @compare: compare string
 *
 * Compares two filter
 *
 * Returns: see #strcmp
 */
static inline gboolean rm_filter_compare(RmFilterRule *rule, const gchar *compare)
{
	gboolean valid = FALSE;

	if (!rule->entry || !compare) {
		return valid;
	}

	switch (rule->sub_type) {
	case RM_FILTER_IS:
		if (!strcmp(compare, rule->entry)) {
			valid = TRUE;
		}
		break;
	case RM_FILTER_IS_NOT:
		if (strcmp(compare, rule->entry)) {
			valid = TRUE;
		}
		break;
	case RM_FILTER_STARTS_WITH:
		if (!strncmp(compare, rule->entry, strlen(rule->entry))) {
			valid = TRUE;
		}
		break;
	case RM_FILTER_CONTAINS:
		if (rm_strcasestr(compare, rule->entry)) {
			valid = TRUE;
		}
		break;
	default:
		break;
	}

	return valid;
}

/**
 * rm_filter_rule_match:
 * @filter: a #RmFilter
 * @call: a #RmCallEntry
 *
 * Check if call structure matches filter rules.
 *
 * Returns: %TRUE on match, otherwise %FALSE
 */
gboolean rm_filter_rule_match(RmFilter *filter, RmCallEntry *call)
{
	GSList *list;
	gint result = 0;
	gint dates = 0;
	gint dates_valid = 0;
	gboolean call_type = FALSE;
	gboolean call_type_valid = FALSE;
	gboolean remote_name = FALSE;
	gboolean remote_name_valid = FALSE;
	gboolean remote_number = FALSE;
	gboolean remote_number_valid = FALSE;
	gboolean local_number = FALSE;
	gboolean local_number_valid = FALSE;
	gboolean local_name = FALSE;
	gboolean local_name_valid = FALSE;

	if (!filter) {
		/* We have no filter, everything matches */
		return TRUE;
	}

	/* Test each filter rule with the call structure */
	for (list = filter->rules; list != NULL; list = list->next) {
		RmFilterRule *rule = list->data;

		switch (rule->type) {
		case RM_FILTER_CALL_TYPE:
			/* Call type */
			call_type = TRUE;

			if (rule->sub_type == RM_CALL_ENTRY_TYPE_ALL || rule->sub_type == call->type) {
				call_type_valid = TRUE;
			}

			if (filter->compare_or && call_type_valid) {
				return TRUE;
			}

			break;
		case RM_FILTER_DATE_TIME: {
			/* Date/Time */
			gchar *number_a = rule->entry;
			gchar *number_b = call->date_time;
			gint ret = 0;

			if (!rule->entry) {
				break;
			}

			dates++;

			switch (rule->sub_type) {
			case RM_FILTER_IS:
				/* Compare year */
				ret = strncmp(number_a + 6, number_b + 6, 2);
				if (ret == 0) {
					/* Compare month */
					ret = strncmp(number_a + 3, number_b + 3, 2);
					if (ret == 0) {
						/* Compare day */
						ret = strncmp(number_a, number_b, 2);
						if (ret == 0) {
							dates_valid++;
						}
					}
				}
				break;
			case RM_FILTER_IS_NOT:
				/* Compare year */
				ret = strncmp(number_a + 6, number_b + 6, 2);
				if (ret == 0) {
					/* Compare month */
					ret = strncmp(number_a + 3, number_b + 3, 2);
					if (ret == 0) {
						/* Compare day */
						ret = strncmp(number_a, number_b, 2);
						if (ret != 0) {
							dates_valid++;
						}
					} else {
						dates_valid++;
					}
				} else {
					dates_valid++;
				}
			case RM_FILTER_STARTS_WITH:
				/* Compare year */
				ret = strncmp(number_a + 6, number_b + 6, 2);
				if (ret >= 0) {
					/* Compare month */
					ret = strncmp(number_a + 3, number_b + 3, 2);
					if (ret >= 0) {
						/* Compare day */
						ret = strncmp(number_a, number_b, 2);
						if (ret < 0) {
							dates_valid++;
						}
					} else {
						dates_valid++;
					}
				} else {
					dates_valid++;
				}
			case RM_FILTER_CONTAINS:
				/* Compare year */
				ret = strncmp(number_a + 6, number_b + 6, 2);
				if (ret <= 0) {
					/* Compare month */
					ret = strncmp(number_a + 3, number_b + 3, 2);
					if (ret <= 0) {
						/* Compare day */
						ret = strncmp(number_a, number_b, 2);
						if (ret < 0) {
							dates_valid++;
						}
					} else {
						dates_valid++;
					}
				} else {
					dates_valid++;
				}
				break;
			}

			break;
		}
		case RM_FILTER_REMOTE_NAME:
			/* Remote name */
			remote_name = TRUE;
			remote_name_valid = rm_filter_compare(rule, call->remote->name);
			break;
		case RM_FILTER_REMOTE_NUMBER:
			/* Remote number */
			remote_number = TRUE;
			remote_number_valid = rm_filter_compare(rule, call->remote->number);
			break;
		case RM_FILTER_LOCAL_NAME:
			/* Local name */
			local_name = TRUE;
			local_name_valid = rm_filter_compare(rule, call->local->name);
			break;
		case RM_FILTER_LOCAL_NUMBER:
			/* Local number */
			local_number = TRUE;
			local_number_valid = rm_filter_compare(rule, call->local->number);
			break;
		default:
			break;
		}
	}

	/* Setup result value */
	if (call_type_valid || !call_type) {
		result |= 0x01;
	}

	if ((dates && dates == dates_valid) || dates == 0) {
		result |= 0x02;
	}

	if (remote_name_valid || !remote_name) {
		result |= 0x04;
	}
	if (remote_number_valid || !remote_number) {
		result |= 0x08;
	}
	if (local_number_valid || !local_number) {
		result |= 0x10;
	}
	if (local_name_valid || !local_name) {
		result |= 0x20;
	}

	/* Result check */
	return (result == 0x3F);
}

/**
 * rm_filter_sort_by_name:
 * @a: a #RmFilter
 * @b: a #RmFilter
 *
 * Sort filter by name.
 *
 * Returns: result of #strcmp
 */
gint rm_filter_sort_by_name(gconstpointer a, gconstpointer b)
{
	RmFilter *filter_a = (RmFilter *)a;
	RmFilter *filter_b = (RmFilter *)b;

	return strcmp(filter_a->name, filter_b->name);
}

/**
 * rm_filter_rule_add:
 * @filter: a #RmFilter
 * @type: type of filter
 * @sub_type: sub type of filter
 * @entry: ruleal entry
 *
 * Add new filter rule.
 */
void rm_filter_rule_add(RmFilter *filter, gint type, gint sub_type, gchar *entry)
{
	RmFilterRule *rule = g_slice_new(RmFilterRule);

	rule->type = type;
	rule->sub_type = sub_type;
	rule->entry = g_strdup(entry);

	filter->rules = g_slist_append(filter->rules, rule);
}

/**
 * rm_filter_rules_free:
 * @data: a #RmFilterRule
 *
 * Free filter rules.
 */
static void rm_filter_rules_free(gpointer data)
{
	RmFilterRule *rule = data;

	g_return_if_fail(rule != NULL);

	/* Free entry */
	g_free(rule->entry);

	/* Free rule structure */
	g_slice_free(RmFilterRule, rule);
}

/**
 * rm_filter_free:
 * @data: a #RmFilter
 *
 * Free filter structure (including rules)
 */
void rm_filter_free(gpointer data)
{
	RmFilter *filter = data;

	/* Free filter rules */
	g_slist_free_full(filter->rules, rm_filter_rules_free);

	/* Free filter name */
	g_free(filter->name);

	/* Free filter structure */
	g_slice_free(RmFilter, filter);
}

/**
 * rm_filter_new:
 * @profile: a #RmProfile
 * @name: filter name
 *
 * Create new filter structure with given name.
 *
 * Returns: filter structure pointer
 */
RmFilter *rm_filter_new(RmProfile *profile, const gchar *name)
{
	RmFilter *filter = g_slice_new0(RmFilter);

	filter->name = g_strdup(name);
	filter->compare_or = FALSE;
	filter->rules = NULL;

	profile->filter_list = g_slist_insert_sorted(profile->filter_list, filter, rm_filter_sort_by_name);

	return filter;
}

/**
 * rm_filter_remove:
 * @profile: a #RmProfile
 * @filter: a #RmFilter
 *
 * Remove filter from list.
 */
void rm_filter_remove(RmProfile *profile, RmFilter *filter)
{
	profile->filter_list = g_slist_remove(profile->filter_list, filter);

	if (filter->file) {
		g_remove(filter->file);
	}

	g_free(filter->name);

	g_slice_free(RmFilter, filter);
}

/**
 * rm_filter_get_list:
 * @profile: a #RmProfile
 *
 * Get filter list.
 *
 * Returns: filter list or %NULL if profile is not set
 */
GSList *rm_filter_get_list(RmProfile *profile)
{
	if (!profile) {
		return NULL;
	}

	return profile->filter_list;
}

/**
 * rm_filter_load:
 * @profile: a #RmProfile
 *
 * Load filter from storage - INTERNAL -
 */
static void rm_filter_load(RmProfile *profile)
{
	GFile *dir;
	GFileEnumerator *enumerator;
	GFileInfo *info;
	GError *error = NULL;
	gchar *path = g_build_filename(rm_get_user_config_dir(), G_DIR_SEPARATOR_S, "profile", G_DIR_SEPARATOR_S, profile->name, G_DIR_SEPARATOR_S, "filters", G_DIR_SEPARATOR_S, NULL);

	dir = g_file_new_for_path(path);

	enumerator = g_file_enumerate_children(dir, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, &error);
	if (!enumerator) {
		return;
	}

	while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL))) {
		const gchar *name = g_file_info_get_name(info);
		gchar *tmp = g_strconcat(path, name, NULL);
		GKeyFile *keyfile = g_key_file_new();
		RmFilter *filter;
		gsize cnt;
		gchar **groups;
		gint idx;

		g_key_file_load_from_file(keyfile, tmp, G_KEY_FILE_NONE, NULL);
		groups = g_key_file_get_groups(keyfile, &cnt);

		filter = rm_filter_new(profile, name);
		for (idx = 0; idx < cnt; idx++) {
			gint type;
			gint subtype;
			gchar *entry;

			type = g_key_file_get_integer(keyfile, groups[idx], "type", NULL);
			subtype = g_key_file_get_integer(keyfile, groups[idx], "subtype", NULL);
			entry =  g_key_file_get_string(keyfile, groups[idx], "entry", NULL);
			rm_filter_rule_add(filter, type, subtype, entry);

			filter->file = g_strdup(tmp);
			g_free(entry);
		}

		g_key_file_unref(keyfile);

		g_free(tmp);
	}

	g_object_unref(enumerator);
}

/**
 * rm_filter_to_data:
 * @filter: a #RmFilter
 *
 * Convert filter structure to keyfile data.
 *
 * Returns: keyfile data
 */
static inline gchar *rm_filter_to_data(RmFilter *filter)
{
	GSList *rules;
	gchar *data = g_strdup("# Filter file\n\n");
	gint counter;

	/* We have always rules available */
	for (rules = filter->rules, counter = 0; rules != NULL; rules = rules->next, counter++) {
		RmFilterRule *rule = rules->data;
		gchar *rule_str = g_strdup_printf("[rule%d]\ntype=%d\nsubtype=%d\nentry=%s\n\n", counter, rule->type, rule->sub_type, rule->entry ? rule->entry : "");
		gchar *tmp;

		tmp = g_strconcat(data, rule_str, NULL);
		g_free(data);
		data = tmp;
	}

	return data;
}

/**
 * rm_filter_save:
 * @profile: a #RmProfile
 *
 * Save filters to keyfiles.
 */
static void rm_filter_save(RmProfile *profile)
{
	GSList *list;
	gchar *path;

	list = profile->filter_list;
	path = g_build_filename(rm_get_user_config_dir(), G_DIR_SEPARATOR_S, "profile", G_DIR_SEPARATOR_S, profile->name, G_DIR_SEPARATOR_S, "filters", G_DIR_SEPARATOR_S, NULL);

	g_mkdir_with_parents(path, 0700);

	while (list) {
		RmFilter *filter = list->data;
		gchar *file = g_build_filename(path, filter->name, NULL);
		gchar *data = rm_filter_to_data(filter);

		rm_file_save(file, data, -1);
		if (!filter->file) {
			filter->file = g_strdup(file);
		}
		g_free(data);
		g_free(file);

		list = list->next;
	}
}

/**
 * rm_filter_init:
 * @profile: a #RmProfile
 *
 * Initialize filter and add default filters if needed.
 */
void rm_filter_init(RmProfile *profile)
{
	RmFilter *filter;

	rm_filter_load(profile);

	if (rm_filter_get_list(profile)) {
		return;
	}

	/* No self-made filters available, create standard filters */

	/* All calls */
	filter = rm_filter_new(profile, R_("All calls"));
	rm_filter_rule_add(filter, 0, RM_CALL_ENTRY_TYPE_ALL, NULL);

	/* Incoming calls */
	filter = rm_filter_new(profile, R_("Incoming calls"));
	rm_filter_rule_add(filter, 0, RM_CALL_ENTRY_TYPE_INCOMING, NULL);

	/* Missed calls */
	filter = rm_filter_new(profile, R_("Missed calls"));
	rm_filter_rule_add(filter, 0, RM_CALL_ENTRY_TYPE_MISSED, NULL);

	/* Outgoing calls */
	filter = rm_filter_new(profile, R_("Outgoing calls"));
	rm_filter_rule_add(filter, 0, RM_CALL_ENTRY_TYPE_OUTGOING, NULL);

	/* Fax */
	filter = rm_filter_new(profile, R_("Fax"));
	rm_filter_rule_add(filter, 0, RM_CALL_ENTRY_TYPE_FAX, NULL);

	/* Answering machine */
	filter = rm_filter_new(profile, R_("Answering machine"));
	rm_filter_rule_add(filter, 0, RM_CALL_ENTRY_TYPE_VOICE, NULL);

	/* Fax Report */
	filter = rm_filter_new(profile, R_("Fax Report"));
	rm_filter_rule_add(filter, 0, RM_CALL_ENTRY_TYPE_FAX_REPORT, NULL);

	/* Voice Record */
	filter = rm_filter_new(profile, R_("Record"));
	rm_filter_rule_add(filter, 0, RM_CALL_ENTRY_TYPE_RECORD, NULL);

	/* Blocked calls */
	filter = rm_filter_new(profile, R_("Blocked"));
	rm_filter_rule_add(filter, 0, RM_CALL_ENTRY_TYPE_BLOCKED, NULL);
}

/**
 * rm_filter_shutdown:
 * @profile: a #RmProfile
 *
 * Shutdown filter (free list and entries).
 */
void rm_filter_shutdown(RmProfile *profile)
{
	if (!profile) {
		return;
	}

	rm_filter_save(profile);
	g_slist_free_full(profile->filter_list, rm_filter_free);
	profile->filter_list = NULL;
}
