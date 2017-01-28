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
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include <rm/rmnumber.h>
#include <rm/rmstring.h>
#include <rm/rmprofile.h>
#include <rm/rmrouter.h>

/**
 * SECTION:rmnumber
 * @Title: RmNumber
 * @Short_description: Number functions
 *
 * Phone number manipulation functions
 */

/** Call-by-call number table */
struct rm_call_by_call_entry rm_call_by_call_table[] = {
	{"49", "0100", 6},
	{"49", "010", 5},
	{"31", "16", 4},
	{ "", "", 0},
};

/**
 * rm_number_scramble:
 * @number: input number
 *
 * Scramble number so we can print it to log files.
 *
 * Returns: scrambled number
 */
gchar *rm_number_scramble(const gchar *number)
{
	gchar *scramble;
	gint len;

	if (!number) {
		return g_strdup("");
	}

	scramble = g_strdup(number);

	len = strlen(number);

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
 * rm_call_by_call_prefix_length:
 * @number: input number string
 *
 * Get call-by-call prefix length
 *
 * Returns: length of call-by-call prefix
 */
gint rm_call_by_call_prefix_length(const gchar *number)
{
	gchar *my_country_code = rm_router_get_country_code(rm_profile_get_active());
	struct rm_call_by_call_entry *entry;

	if (RM_EMPTY_STRING(my_country_code)) {
		g_free(my_country_code);
		return 0;
	}

	for (entry = rm_call_by_call_table; strlen(entry->country_code); entry++) {
		if (!strcmp(my_country_code, entry->country_code) && !strncmp(number, entry->prefix, strlen(entry->prefix))) {
			g_free(my_country_code);
			return entry->prefix_length;
		}
	}

	g_free(my_country_code);

	return 0;
}

/**
 * rm_number_canonize:
 * @number: input number
 *
 * Canonize number (valid chars: 0123456789#*).
 *
 * Returns: canonized number
 */
gchar *rm_number_canonize(const gchar *number)
{
	GString *new_number;

	new_number = g_string_sized_new(strlen(number));
	while (*number) {
		if (isdigit(*number) || *number == '*' || *number == '#') {
			g_string_append_c(new_number, *number);
		} else if (*number == '+') {
			g_string_append(new_number, rm_router_get_international_access_code(rm_profile_get_active()));
		}

		number++;
	}

	return g_string_free(new_number, FALSE);
}

/**
 * rm_number_format:
 * @profile: a #RmProfile
 * @number: input number
 * @output_format: selected number output format
 *
 * Format number according to phone standard.
 *
 * Returns: real number
 */
gchar *rm_number_format(RmProfile *profile, const gchar *number, enum rm_number_format output_format)
{
	gchar *tmp;
	gchar *canonized;
	gchar *international_access_code;
	gchar *national_prefix;
	gchar *my_country_code;
	gchar *my_area_code;
	gint number_format = RM_NUMBER_FORMAT_UNKNOWN;
	const gchar *my_prefix;
	gchar *result = NULL;

	/* Check for internal sip numbers first */
	if (strchr(number, '@')) {
		return g_strdup(number);
	}

	canonized = tmp = rm_number_canonize(number);

	international_access_code = rm_router_get_international_access_code(profile);
	national_prefix = rm_router_get_national_prefix(profile);
	my_country_code = rm_router_get_country_code(profile);
	my_area_code = rm_router_get_area_code(profile);

	/* we only need to check for international prefix, as rm_number_canonize() already replaced '+'
	 * Example of the following:
	 *    tmp = 00494012345678  with international_access_code 00 and my_country_code 49
	 *    number_format = NUMBER_FORMAT_UNKNOWN
	 */
	if (!strncmp(tmp, international_access_code, strlen(international_access_code))) {
		/* International format number */
		tmp += strlen(international_access_code);
		number_format = RM_NUMBER_FORMAT_INTERNATIONAL;

		/* Example:
		 * tmp = 494012345678
		 * number_format = NUMBER_FORMAT_INTERNATIONAL
		 */
		if (!strncmp(tmp, my_country_code, strlen(my_country_code)))  {
			/* national number */
			tmp = tmp + strlen(my_country_code);
			number_format = RM_NUMBER_FORMAT_NATIONAL;

			/* Example:
			 * tmp = 4012345678
			 * number_format = NUMBER_FORMAT_NATIONAL
			 */
		}
	} else {
		/* not an international format, test for national or local format */
		if (!RM_EMPTY_STRING(national_prefix) && !strncmp(tmp, national_prefix, strlen(national_prefix))) {
			tmp = tmp + strlen(national_prefix);
			number_format = RM_NUMBER_FORMAT_NATIONAL;

			/* Example:
			 * Input was: 04012345678
			 * tmp = 4012345678
			 * number_format = NUMBER_FORMAT_NATIONAL
			 */
		} else {
			number_format = RM_NUMBER_FORMAT_LOCAL;
			/* Example:
			 * Input was 12345678
			 * tmp = 12345678
			 * number_format = NUMBER_FORMAT_LOCAL
			 */
		}
	}

	if ((number_format == RM_NUMBER_FORMAT_NATIONAL) && (!strncmp(tmp, my_area_code, strlen(my_area_code)))) {
		/* local number */
		tmp = tmp + strlen(my_area_code);
		number_format = RM_NUMBER_FORMAT_LOCAL;

		/* Example:
		 * Input was 4012345678
		 * tmp = 12345678
		 * number_format = NUMBER_FORMAT_LOCAL
		 */
	}

	switch (output_format) {
	case RM_NUMBER_FORMAT_LOCAL:
	/* local number format */
	case RM_NUMBER_FORMAT_NATIONAL:
		/* national number format */
		switch (number_format) {
		case RM_NUMBER_FORMAT_LOCAL:
			if (output_format == RM_NUMBER_FORMAT_LOCAL) {
				result = g_strdup(tmp);
			} else {
				result = g_strconcat(national_prefix, my_area_code, tmp, NULL);
			}
			break;
		case RM_NUMBER_FORMAT_NATIONAL:
			result = g_strconcat(national_prefix, tmp, NULL);
			break;
		case RM_NUMBER_FORMAT_INTERNATIONAL:
			result = g_strconcat(international_access_code, tmp, NULL);
			break;
		}
		break;
	case RM_NUMBER_FORMAT_INTERNATIONAL:
	/* international prefix + international format */
	case RM_NUMBER_FORMAT_INTERNATIONAL_PLUS:
		/* international format prefixed by a + */
		my_prefix = (output_format == RM_NUMBER_FORMAT_INTERNATIONAL_PLUS) ? "+" : international_access_code;
		switch (number_format) {
		case RM_NUMBER_FORMAT_LOCAL:
			result = g_strconcat(my_prefix, my_country_code, my_area_code, tmp, NULL);
			break;
		case RM_NUMBER_FORMAT_NATIONAL:
			result = g_strconcat(my_prefix, my_country_code, tmp, NULL);
			break;
		case RM_NUMBER_FORMAT_INTERNATIONAL:
			result = g_strconcat(my_prefix, tmp, NULL);
			break;
		}
		break;
	default:
		g_assert(output_format);
		break;
	}

	g_free(international_access_code);
	g_free(national_prefix);
	g_free(my_country_code);
	g_free(my_area_code);

	g_free(canonized);
	g_assert(result != NULL);

	return result;
}

/**
 * rm_number_full:
 * @number: input phone number
 * @country_code_prefix: whether we want a international or national phone number format
 *
 * Convenient function to retrieve standardized number without call by call prefix
 *
 * Returns: canonized and formatted phone number
 */
gchar *rm_number_full(const gchar *number, gboolean country_code_prefix)
{
	if (RM_EMPTY_STRING(number)) {
		return NULL;
	}

	/* Skip numbers with leading '*' or '#' */
	if (number[0] == '*' || number[0] == '#') {
		return g_strdup(number);
	}

	/* Remove call-by-call (carrier preselect) prefix */
	number += rm_call_by_call_prefix_length(number);

	/* Check if it is an international number */
	if (!strncmp(number, "00", 2)) {
		gchar *out;
		gchar *my_country_code;

		if (country_code_prefix) {
			return g_strdup(number);
		}

		my_country_code = rm_router_get_country_code(rm_profile_get_active());
		if (!strncmp(number + 2, my_country_code, strlen(my_country_code)))  {
			out = g_strdup_printf("0%s", number + 4);
		} else {
			out = g_strdup(number);
		}

		return out;
	}

	return rm_number_format(rm_profile_get_active(), number, country_code_prefix ? RM_NUMBER_FORMAT_INTERNATIONAL : RM_NUMBER_FORMAT_NATIONAL);
}
