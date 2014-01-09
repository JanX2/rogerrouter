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

/**
 * TODO:
 *  - add or- combination
 */

#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <libintl.h>

#include <libroutermanager/file.h>
#include <libroutermanager/call.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/filter.h>

/** Internal filter list in memory */
static GSList *filter_list = NULL;

static inline gboolean filter_compare(struct filter_rule *rule, const gchar *compare)
{
	gboolean valid = FALSE;

	if (!rule->entry || !compare) {
		return valid;
	}

	switch (rule->sub_type) {
	case FILTER_IS:
		if (!strcmp(compare, rule->entry)) {
			valid = TRUE;
		}
		break;
	case FILTER_IS_NOT:
		if (strcmp(compare, rule->entry)) {
			valid = TRUE;
		}
		break;
	case FILTER_STARTS_WITH:
		if (!strncmp(compare, rule->entry, strlen(rule->entry))) {
			valid = TRUE;
		}
		break;
	case FILTER_CONTAINS:
		if (g_strcasestr(compare, rule->entry)) {
			valid = TRUE;
		}
		break;
	default:
		break;
	}

	return valid;
}

/**
 * \brief Check if call structure matches filter rules
 * \param filter compare filter
 * \param call call structure
 * \return TRUE on match, otherwise FALSE
 */
gboolean filter_rule_match(struct filter *filter, struct call *call)
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

	if (!filter) {
		/* We have no filter, everything matches */
		return TRUE;
	}

	/* Test each filter rule with the call structure */
	for (list = filter->rules; list != NULL; list = list->next) {
		struct filter_rule *rule = list->data;

		switch (rule->type) {
		case FILTER_CALL_TYPE:
			/* Call type */
			call_type = TRUE;

			if (rule->sub_type == CALL_TYPE_ALL || rule->sub_type == call->type) {
				call_type_valid = TRUE;
			}

			if (filter->compare_or && call_type_valid) {
				return TRUE;
			}

			break;
		case FILTER_DATE_TIME: {
			/* Date/Time */
			gchar *number_a = rule->entry;
			gchar *number_b = call->date_time;
			gint ret = 0;

			if (!rule->entry) {
				break;
			}

			dates++;

			switch (rule->sub_type) {
			case FILTER_IS:
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
			case FILTER_IS_NOT:
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
			case FILTER_STARTS_WITH:
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
			case FILTER_CONTAINS:
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
		case FILTER_REMOTE_NAME:
			/* Remote name */
			remote_name = TRUE;
			remote_name_valid = filter_compare(rule, call->remote->name);
			break;
		case FILTER_REMOTE_NUMBER:
			/* Remote number */
			remote_number = TRUE;
			remote_number_valid = filter_compare(rule, call->remote->number);
			break;
		case FILTER_LOCAL_NUMBER:
			/* Local number */
			local_number = TRUE;
			local_number_valid = filter_compare(rule, call->local->number);
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

	/* Result check */
	return (result == 0x1F);
}

/**
 * \brief Sort filter by name
 * \param a pointer to first filter
 * \param b pointer to second filter
 * \return result of strcmp
 */
gint filter_sort_by_name(gconstpointer a, gconstpointer b)
{
	struct filter *filter_a = (struct filter *)a;
	struct filter *filter_b = (struct filter *)b;

	return strcmp(filter_a->name, filter_b->name);
}

/**
 * \brief Add new filter rule
 * \param filter filter structure
 * \param type type of filter
 * \param sub_type sub type of filter
 * \param entry ruleal entry
 */
void filter_rule_add(struct filter *filter, gint type, gint sub_type, gchar *entry)
{
	struct filter_rule *rule = g_slice_new(struct filter_rule);

	rule->type = type;
	rule->sub_type = sub_type;
	rule->entry = g_strdup(entry);

	filter->rules = g_slist_append(filter->rules, rule);
}

/**
 * \brief Free filter rules
 * \param data filter rule pointer
 */
static void filter_rules_free(gpointer data)
{
	struct filter_rule *rule = data;

	g_return_if_fail(rule != NULL);

	/* Free entry */
	g_free(rule->entry);

	/* Free rule structure */
	g_slice_free(struct filter_rule, rule);
}

/**
 * \brief Free filter structure (including rules)
 * \param data filter structrue to free
 */
void filter_free(gpointer data)
{
	struct filter *filter = data;

	/* Free filter rules */
	g_slist_free_full(filter->rules, filter_rules_free);

	/* Free filter name */
	g_free(filter->name);

	/* Free filter structure */
	g_slice_free(struct filter, filter);
}

/**
 * \brief Add filter structure to list
 * \param filter filter structure
 */
void filter_add(struct filter *filter)
{
	filter_list = g_slist_insert_sorted(filter_list, filter, filter_sort_by_name);
}

/**
 * \brief Create new filter structure with given name
 * \param name filter name
 * \return filter structure pointer
 */
struct filter *filter_new(const gchar *name)
{
	struct filter *filter = g_slice_new0(struct filter);

	filter->name = g_strdup(name);
	filter->compare_or = FALSE;
	filter->rules = NULL;

	return filter;
}

/**
 * \brief Get filter list
 * \return filter list
 */
GSList *filter_get_list(void)
{
	return filter_list;
}

/**
 * \brief Load filter from storage - INTERNAL -
 */
static void filter_load(void)
{
	GFile *dir;
	GFileEnumerator *enumerator;
	GFileInfo *info;
	GError *error = NULL;
	gchar *path = g_build_filename(g_get_user_config_dir(), "routermanager/filters", G_DIR_SEPARATOR_S, NULL);

	dir = g_file_new_for_path(path);

	enumerator = g_file_enumerate_children(dir, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, &error);
	if (!enumerator) {
		return;
	}

	while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL))) {
		const gchar *name = g_file_info_get_name(info);
		gchar *tmp = g_strconcat(path, name, NULL);
		GKeyFile *keyfile = g_key_file_new();
		struct filter *filter;
		gsize cnt;
		gchar **groups;
		gint idx;

		g_key_file_load_from_file(keyfile, tmp, G_KEY_FILE_NONE, NULL);
		groups = g_key_file_get_groups(keyfile, &cnt);
		filter = filter_new(name);
		for (idx = 0; idx < cnt; idx++) {
			gint type;
			gint subtype;
			gchar *entry;

			type = g_key_file_get_integer(keyfile, groups[idx], "type", NULL);
			subtype = g_key_file_get_integer(keyfile, groups[idx], "subtype", NULL);
			entry =  g_key_file_get_string(keyfile, groups[idx], "entry", NULL);
			filter_rule_add(filter, type, subtype, entry);

			filter->file = g_strdup(tmp);
			g_free(entry);
		}
		filter_add(filter);

		g_key_file_unref(keyfile);

		g_free(tmp);
	}

	g_object_unref(enumerator);
}

/**
 * \brief Initialize filter and add default filters
 */
void filter_init(void)
{
	struct filter *filter;

	filter_load();

	if (filter_list) {
		return;
	}

	/* No self-made filters available, create standard filters */

	/* All calls */
	filter = filter_new(_("All calls"));
	filter_rule_add(filter, 0, CALL_TYPE_ALL, NULL);
	filter_add(filter);

	/* Incoming calls */
	filter = filter_new(_("Incoming calls"));
	filter_rule_add(filter, 0, CALL_TYPE_INCOMING, NULL);
	filter_add(filter);

	/* Missed calls */
	filter = filter_new(_("Missed calls"));
	filter_rule_add(filter, 0, CALL_TYPE_MISSED, NULL);
	filter_add(filter);

	/* Outgoing calls */
	filter = filter_new(_("Outgoing calls"));
	filter_rule_add(filter, 0, CALL_TYPE_OUTGOING, NULL);
	filter_add(filter);

	/* Fax */
	filter = filter_new(_("Fax"));
	filter_rule_add(filter, 0, CALL_TYPE_FAX, NULL);
	filter_add(filter);

	/* Answering machine */
	filter = filter_new(_("Answering machine"));
	filter_rule_add(filter, 0, CALL_TYPE_VOICE, NULL);
	filter_add(filter);
}

/**
 * \brief Convert filter structure to keyfile data
 * \param filter filter structure
 * \return keyfile data
 */
static inline gchar *filter_to_data(struct filter *filter)
{
	GSList *rules;
	gchar *data = g_strdup("# Filter file\n\n");
	gint counter;

	/* We have always rules available */
	for (rules = filter->rules, counter = 0; rules != NULL; rules = rules->next, counter++) {
		struct filter_rule *rule = rules->data;
		gchar *rule_str = g_strdup_printf("[rule%d]\ntype=%d\nsubtype=%d\nentry=%s\n\n", counter, rule->type, rule->sub_type, rule->entry ? rule->entry : "");
		gchar *tmp;

		tmp = g_strconcat(data, rule_str, NULL);
		g_free(data);
		data = tmp;
	}

	return data;
}

/**
 * \brief Save filters to keyfiles
 */
void filter_save(void)
{
	GSList *list = filter_list;
	gchar *path = g_build_filename(g_get_user_config_dir(), "routermanager/filters", G_DIR_SEPARATOR_S, NULL);

	g_mkdir_with_parents(path, 0700);

	while (list) {
		struct filter *filter = list->data;
		gchar *file = g_build_filename(path, filter->name, NULL);
		gchar *data = filter_to_data(filter);

		file_save(file, data, -1);
		if (!filter->file) {
			filter->file = g_strdup(file);
		}
		g_free(data);
		g_free(file);

		list = list->next;
	}
}

/**
 * \brief Shutdown filter (free list and entries)
 */
void filter_shutdown(void)
{
	g_slist_free_full(filter_list, filter_free);
}

/**
 * \brief Remove filter from list
 * \param filter filter to remove
 */
void filter_remove(struct filter *filter)
{
	filter_list = g_slist_remove(filter_list, filter);

	if (filter->file) {
		g_remove(filter->file);
	}
}
