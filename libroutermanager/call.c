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
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include <libroutermanager/call.h>
#include <libroutermanager/gstring.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/router.h>

/** Call-by-call number table */
struct call_by_call_entry call_by_call_table[] = {
	{"49", "0100", 6},
	{"49", "010", 5},
	{"31", "16", 4},
	{ "", "", 0},
};

/**
 * \brief Sort calls (compares two calls based on date/time)
 * \param a call a
 * \param b call b
 * \return see strncmp
 */
gint call_sort_by_date(gconstpointer a, gconstpointer b)
{
	struct call *call_a = (struct call *) a;
	struct call *call_b = (struct call *) b;
	gchar *number_a = NULL;
	gchar *number_b = NULL;
	gchar part_time_a[7];
	gchar part_time_b[7];
	gint ret = 0;

	if (!call_a || !call_b) {
		return 0;
	}

	if (call_a) {
		number_a = call_a->date_time;
	}

	if (call_b) {
		number_b = call_b->date_time;
	}

	/* Compare year */
	ret = strncmp(number_a + 6, number_b + 6, 2);
	if (ret == 0) {
		/* Compare month */
		ret = strncmp(number_a + 3, number_b + 3, 2);
		if (ret == 0) {
			/* Compare day */
			ret = strncmp(number_a, number_b, 2);
			if (ret == 0) {
				/* Extract time */
				memset(part_time_a, 0, sizeof(part_time_a));
				g_strlcpy(part_time_a, number_a + 9, 6);

				/* Extract time */
				memset(part_time_b, 0, sizeof(part_time_b));
				g_strlcpy(part_time_b, number_b + 9, 6);

				ret = g_utf8_collate(part_time_a, part_time_b);
			}
		}
	}

	return -ret;
}

/**
 * \brief Add call to journal
 * \param journal call list
 * \param type call type
 * \param date_time date and time of call
 * \param remote_name remote caller name
 * \param remote_number remote caller number
 * \param local_name local caller name
 * \param local_number local caller number
 * \param duration call duration
 * \param priv private data
 * \return new call list with appended call structure
 */
GSList *call_add(GSList *journal, gint type, const gchar *date_time, const gchar *remote_name, const gchar *remote_number, const gchar *local_name, const gchar *local_number, const gchar *duration, gpointer priv)
{
	GSList *list = journal;
	struct call *call = NULL;

	/* Search through list and find duplicates */
	for (list = journal; list != NULL; list = list->next) {
		call = list->data;

		/* Easier compare method, we are just interested in the complete date_time, remote_number and type field */
		if (!strcmp(call->date_time, date_time) && !strcmp(call->remote->number, remote_number)) {
			if (call->type == type) {
				/* Call with the same type already exists, return unchanged journal */
				return journal;
			}

			/* Found same call with different type (voice/fax): merge them */
			if (type == CALL_TYPE_VOICE || type == CALL_TYPE_FAX) {
				call->type = type;
				call->priv = priv;

				return journal;
			}
		}
	}

	/* Create new call structure */
	call = g_slice_new0(struct call);

	/* Set entries */
	call->type = type;
	call->date_time = date_time ? g_strdup(date_time) : g_strdup("");
	call->remote = g_slice_new0(struct contact);
	call->remote->name = remote_name ? g_convert_utf8(remote_name, -1) : g_strdup("");
	call->remote->number = remote_number ? g_strdup(remote_number) : g_strdup("");
	call->local = g_slice_new0(struct contact);
	call->local->name = local_name ? g_convert_utf8(local_name, -1) : g_strdup("");
	call->local->number = local_number ? g_strdup(local_number) : g_strdup("");
	call->duration = duration ? g_strdup(duration) : g_strdup("");

	/* Extended */
	call->remote->company = g_strdup("");
	call->remote->city = g_strdup("");
	call->priv = priv;

	/* Append call sorted to the list */
	list = g_slist_insert_sorted(journal, call, call_sort_by_date);

	/* Return new call list */
	return list;
}

/**
 * \brief Free call structure
 * \param data pointer to call structure
 */
void call_free(gpointer data)
{
	struct call *call = data;

	/* Free entries */
	g_free(call->date_time);
	g_free(call->remote->name);
	g_free(call->remote->number);
	g_free(call->local->name);
	g_free(call->local->number);
	g_free(call->duration);

	/* Extended */
	g_free(call->remote->company);
	g_free(call->remote->city);

	/* Free structure */
	g_slice_free(struct contact, call->remote);
	g_slice_free(struct contact, call->local);
	g_slice_free(struct call, call);
}

/**
 * call_scramble_number:
 * \brief Scramble number so we can print it to log files
 * \param number input number
 * Returns: (transfer none) scrambled number
 */
gchar *call_scramble_number(const gchar *number)
{
	static gchar scramble[255];
	gint len;

	memset(scramble, 0, sizeof(scramble));
	strncpy(scramble, number, sizeof(scramble));
	len = strlen(scramble);

	if (len > 2) {
		gint index;
		gint end = len;

		if (len > 4) {
			end = len - 1;
		}

		for (index = 2; index < end; index++) {
			scramble[index] = 'X';
		}
	}

	return scramble;
}

/**
 * call_by_call_prefix_length:
 * @number: input number string
 *
 * Get call-by-call prefix length
 *
 * Returns: length of call-by-call prefix
 */
gint call_by_call_prefix_length(const gchar *number)
{
	gchar *my_country_code = router_get_country_code(profile_get_active());
	struct call_by_call_entry *entry;

	if (EMPTY_STRING(my_country_code)) {
		g_free(my_country_code);
		return 0;
	}

	for (entry = call_by_call_table; strlen(entry->country_code); entry++) {
		if (!strcmp(my_country_code, entry->country_code) && !strncmp(number, entry->prefix, strlen(entry->prefix))) {
			g_free(my_country_code);
			return entry->prefix_length;
		}
	}

	g_free(my_country_code);

	return 0;
}

/**
 * \brief Canonize number (valid chars: 0123456789#*)
 * \param number input number
 * \return canonized number
 */
gchar *call_canonize_number(const gchar *number)
{
	GString *new_number;

	new_number = g_string_sized_new(strlen(number));
	while (*number) {
		if (isdigit(*number) || *number == '*' || *number == '#') {
			g_string_append_c(new_number, *number);
		} else if (*number == '+') {
			g_string_append(new_number, router_get_international_prefix(profile_get_active()));
		}

		number++;
	}

	return g_string_free(new_number, FALSE);
}

/**
 * \brief Format number according to phone standard
 * \param number input number
 * \output_format selected number output format
 * \return real number
 */
gchar *call_format_number(struct profile *profile, const gchar *number, enum number_format output_format)
{
	gchar *tmp;
	gchar *canonized;
	gchar *international_prefix;
	gchar *national_prefix;
	gchar *my_country_code;
	gchar *my_area_code;
	gint number_format = NUMBER_FORMAT_UNKNOWN;
	const gchar *my_prefix;
	gchar *result = NULL;

	/* Check for internal sip numbers first */
	if (strchr(number, '@')) {
		return g_strdup(number);
	}

	canonized = tmp = call_canonize_number(number);

	international_prefix = router_get_international_prefix(profile);
	national_prefix = router_get_national_prefix(profile);
	my_country_code = router_get_country_code(profile);
	my_area_code = router_get_area_code(profile);

	/* we only need to check for international prefix, as call_canonize_number() already replaced '+'
	 * Example of the following:
	 *    tmp = 00494012345678  with international_prefix 00 and my_country_code 49
	 *    number_format = NUMBER_FORMAT_UNKNOWN
	 */
	if (!strncmp(tmp, international_prefix, strlen(international_prefix))) {
		/* International format number */
		tmp += strlen(international_prefix);
		number_format = NUMBER_FORMAT_INTERNATIONAL;

		/* Example:
		 * tmp = 494012345678
		 * number_format = NUMBER_FORMAT_INTERNATIONAL
		 */
		if (!strncmp(tmp, my_country_code, strlen(my_country_code)))  {
			/* national number */
			tmp = tmp + strlen(my_country_code);
			number_format = NUMBER_FORMAT_NATIONAL;

			/* Example:
			 * tmp = 4012345678
			 * number_format = NUMBER_FORMAT_NATIONAL
			 */
		}
	} else {
		/* not an international format, test for national or local format */
		if (!EMPTY_STRING(national_prefix) && !strncmp(tmp, national_prefix, strlen(national_prefix))) {
			tmp = tmp + strlen(national_prefix);
			number_format = NUMBER_FORMAT_NATIONAL;

			/* Example:
			 * Input was: 04012345678
			 * tmp = 4012345678
			 * number_format = NUMBER_FORMAT_NATIONAL
			 */
		} else {
			number_format = NUMBER_FORMAT_LOCAL;
			/* Example:
			 * Input was 12345678
			 * tmp = 12345678
			 * number_format = NUMBER_FORMAT_LOCAL
			 */
		}
	}

	if ((number_format == NUMBER_FORMAT_NATIONAL) && (!strncmp(tmp, my_area_code, strlen(my_area_code)))) {
		/* local number */
		tmp = tmp + strlen(my_area_code);
		number_format = NUMBER_FORMAT_LOCAL;

		/* Example:
		 * Input was 4012345678
		 * tmp = 12345678
		 * number_format = NUMBER_FORMAT_LOCAL
		 */
	}

	switch (output_format) {
	case NUMBER_FORMAT_LOCAL:
	/* local number format */
	case NUMBER_FORMAT_NATIONAL:
		/* national number format */
		switch (number_format) {
		case NUMBER_FORMAT_LOCAL:
			if (output_format == NUMBER_FORMAT_LOCAL) {
				result = g_strdup(tmp);
			} else {
				result = g_strconcat(national_prefix, my_area_code, tmp, NULL);
			}
			break;
		case NUMBER_FORMAT_NATIONAL:
			result = g_strconcat(national_prefix, tmp, NULL);
			break;
		case NUMBER_FORMAT_INTERNATIONAL:
			result = g_strconcat(international_prefix, tmp, NULL);
			break;
		}
		break;
	case NUMBER_FORMAT_INTERNATIONAL:
	/* international prefix + international format */
	case NUMBER_FORMAT_INTERNATIONAL_PLUS:
		/* international format prefixed by a + */
		my_prefix = (output_format == NUMBER_FORMAT_INTERNATIONAL_PLUS) ? "+" : international_prefix;
		switch (number_format) {
		case NUMBER_FORMAT_LOCAL:
			result = g_strconcat(my_prefix, my_country_code, my_area_code, tmp, NULL);
			break;
		case NUMBER_FORMAT_NATIONAL:
			result = g_strconcat(my_prefix, my_country_code, tmp, NULL);
			break;
		case NUMBER_FORMAT_INTERNATIONAL:
			result = g_strconcat(my_prefix, tmp, NULL);
			break;
		}
		break;
	default:
		g_assert(output_format);
		break;
	}

	g_free(international_prefix);
	g_free(national_prefix);
	g_free(my_country_code);
	g_free(my_area_code);

	g_free(canonized);
	g_assert(result != NULL);

	return result;
}

/**
 * \brief Convenient function to retrieve standardized number without call by call prefix
 * \param number input phone number
 * \param country_code_prefix whether we want a international or national phone number format
 * \return canonized and formatted phone number
 */
gchar *call_full_number(const gchar *number, gboolean country_code_prefix)
{
	if (EMPTY_STRING(number)) {
		return NULL;
	}

	/* Skip numbers with leading '*' or '#' */
	if (number[0] == '*' || number[0] == '#') {
		return g_strdup(number);
	}

	/* Remove call-by-call (carrier preselect) prefix */
	number += call_by_call_prefix_length(number);

	/* Check if it is an international number */
	if (!strncmp(number, "00", 2)) {
		gchar *out;
		gchar *my_country_code;

		if (country_code_prefix) {
			return g_strdup(number);
		}

		my_country_code = router_get_country_code(profile_get_active());
		if (!strncmp(number + 2, my_country_code, strlen(my_country_code)))  {
			out = g_strdup_printf("0%s", number + 4);
		} else {
			out = g_strdup(number);
		}

		return out;
	}

	return call_format_number(profile_get_active(), number, country_code_prefix ? NUMBER_FORMAT_INTERNATIONAL : NUMBER_FORMAT_NATIONAL);
}
