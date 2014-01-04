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

#include <libroutermanager/gstring.h>

/**
 * \brief Search for case-sensitive needle in haystack
 * \param haystack haystack
 * \param needle needle
 * \return pointer to position or NULL
 */
gchar *g_strcasestr(const gchar *haystack, const gchar *needle)
{
	size_t n = strlen(needle);

	if (!haystack || !needle) {
		return NULL;
	}

	for (; *haystack; haystack++) {
		if (g_ascii_strncasecmp(haystack, needle, n) == 0) {
			return (gchar *) haystack;
		}
	}

	return NULL;
}

/**
 * \brief Convert string (if needed) to UTF-8
 * \param text input text string
 * \return input string in UTF-8 (must be freed)
 */
gchar *g_convert_utf8(const gchar *text)
{
	GError *error = NULL;
	gsize read_bytes, written_bytes;
	guchar *chr;
	gchar *str = NULL;

	if (!text) {
		g_assert_not_reached();
	}

	if (g_utf8_validate(text, -1, NULL)) {
		return g_strdup(text);
	}

	for (chr = (guchar *) text; *chr != 0; chr++) {
		if (*chr < 32 && *chr != '\n') {
			*chr = ' ';
		}
	}

	str = g_convert(text, strlen(text), "UTF-8", "ISO-8859-1", &read_bytes, &written_bytes, &error);
	if (str == NULL) {
		str = g_strdup(text);

		for (chr = (guchar *) str; *chr != 0; chr++) {
			if (*chr > 128) {
				*chr = '?';
			}
		}
	}

	if (error) {
		g_error_free(error);
	}

	return str;
}

/** iso8859_1 entities */
static gchar *entities_iso8859_1[] = {
	"iexcl", "cent", "pound", "curren", "yen", "brvbar", "sect", "uml", "copy", "ordf",
	"laquo", "not", "shy", "reg", "macr", "deg", "plusmn", "sup2", "sup3", "acute", "micro",
	"para", "middot", "cedil", "sup1", "ordm", "raquo", "frac14", "frac12", "frac34", "iquest",
	"Agrave", "Aacute", "Acirc", "Atilde", "Auml", "Aring", "AElig", "Ccedil", "Egrave",
	"Eacute", "Ecirc", "Euml", "Igrave", "Iacute", "Icirc", "Iuml", "ETH", "Ntilde", "Ograve",
	"Oacute", "Ocirc", "Otilde", "Ouml", "times", "Oslash", "Ugrave", "Uacute", "Ucirc", "Uuml",
	"Yacute", "THORN", "szlig", "agrave", "aacute", "acirc", "atilde", "auml", "aring", "aelig",
	"ccedil", "egrave", "eacute", "ecirc", "euml", "igrave", "iacute", "icirc", "iuml", "eth",
	"ntilde", "ograve", "oacute", "ocirc", "otilde", "ouml", "divide", "oslash", "ugrave",
	"uacute", "ucirc", "uuml", "yacute", "thorn", "yuml", NULL
};

/** entities symbols */
static gchar *entities_symbols[] = {
	"fnof", "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta", "Iota", "Kappa",
	"Lambda", "Mu", "Nu", "Xi", "Omicron", "Pi", "Rho", "Sigma", "Tau", "Uplsilon", "Phi",
	"Chi", "Psi", "Omega", "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
	"iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", "pi", "rho", "sigmaf", "sigma",
	"tau", "upsilon", "phi", "chi", "psi", "omega", "thetasym", "upsih", "piv", "bull",
	"hellip", "prime", "Prime", "oline", "frasl", "weierp", "image", "real", "trade", "alefsym",
	"larr", "uarr", "rarr", "darr", "harr", "crarr", "lArr", "uArr", "rArr", "dArr", "hArr",
	"forall", "part", "exist", "empty", "nabla", "isin" "notin", "ni", "prod", "sum", "minus",
	"lowast", "radic", "prop", "infin", "ang", "and", "or", "cap", "cup", "int", "there4",
	"sim", "cong", "asymp", "ne", "equiv", "le", "ge", "sub", "sup", "nsub", "sube", "supe",
	"oplus", "otimes", "perp", "sdot", "lceil", "rceil", "lfloor", "rfloor", "lang", "rang",
	"loz", "spades", "clubs", "hearts", "diams", NULL
};

/** special entities */
static gchar *entities_special[] = {
	"OElig", "oelig", "Scaron", "scaron", "Yuml", "circ", "tilde",
	"ensp", "emsp", "thinsp", "zwnj", "zwj", "lrm", "rlm", "ndash", "mdash", "lsquo", "rsquo",
	"sbquo", "ldquo", "rdquo", "bdquo", "dagger", "Dagger", "permil", "lsaquo", "rsaquo",
	"euro", NULL
};

/** xml entities */
static gchar *entities_xml[] = {
	"nbsp", "quot", "amp", "lt", "gt", NULL
};

/** the unicode characters for iso8859_1 are 161 + the index in the array */
static guint entity_unicode_symbols[] = {
	402, 913, 914, 915, 916, 917, 918, 919, 920, 921, 922, 923, 924, 925, 926, 927, 928, 929, 931,
	932, 933, 934, 935, 936, 937, 945, 946, 947, 948, 949, 950, 951, 952, 953, 954, 955, 956,
	957, 958, 959, 960, 961, 962, 963, 964, 965, 966, 967, 968, 977, 978, 982, 8226, 8230, 8242,
	8443, 8254, 8260, 8472, 8465, 8476, 8482, 8501, 8592, 8593, 8594, 8595, 8596, 8629, 8656,
	8657, 8658, 8659, 8660, 8704, 8706, 8707, 8709, 8711, 8712, 8713, 8715, 8719, 8721, 8722,
	8727, 8730, 8733, 8734, 8736, 8743, 8744, 8745, 8746, 8747, 8756, 8764, 8773, 8776, 8800,
	8801, 8804, 8805, 8834, 8835, 8836, 8838, 8839, 8853, 8855, 8869, 8901, 8968, 8969, 8970,
	8971, 9001, 9002, 9674, 9824, 9827, 9829, 9830, -1
};

/* special unicode entity */
static guint entity_unicode_special[] = {
	338, 339, 352, 353, 376, 710, 732, 8194, 8195, 8201, 8204, 8205, 8206, 8207,
	8211, 8212, 8216, 8217, 8218, 8220, 8221, 8222, 8224, 8225, 8240, 8249, 8250, 8364, -1
};

/** unicode xml entity */
static guint entity_unicode_xml[] = {
	160, 34, 38, 60, 62, -1
};

/**
 * \brief Returns index in array
 * \param arr array
 * \param string string to search in array
 * \return index or -1 on error
 */
static gint index_in_array(gchar **arr, gchar *string)
{
	gint i;

	for (i = 0; arr[i] != NULL; i++) {
		if (strcmp(arr[i], string) == 0) {
			return i;
		}
	}

	return -1;
}

/**
 * \brief Get unichar for entity
 * \param entity entity
 * \param numerical numerical format?
 * \param iso8859_1 test iso8859_1
 * \param symbols test symbols
 * \param specials test specials
 * \param xml test xml
 * \return unicode char
 */
static gunichar unichar_for_entity(gchar *entity, gboolean numerical, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml)
{
	gint indx;

	if (!entity) {
		return -1;
	}

	if (entity[0] == '#') {
		if (numerical) {
			/* convert the remaining characters to a number */
			if (entity[1] == 'x') {
				/* from hexadecimal */
				return g_ascii_strtoull(&entity[2], NULL, 16);
			} else {
				/* from decimal */
				return g_ascii_strtoull(&entity[1], NULL, 10);
			}
		}
		return -1;
	}

	if (iso8859_1) {
		indx = index_in_array(entities_iso8859_1, entity);

		if (indx != -1) {
			return indx + 161;
		}
	}

	if (symbols) {
		indx = index_in_array(entities_symbols, entity);

		if (indx != -1) {
			return entity_unicode_symbols[indx];
		}
	}

	if (specials) {
		indx = index_in_array(entities_special, entity);

		if (indx != -1) {
			return entity_unicode_special[indx];
		}
	}

	if (xml) {
		indx = index_in_array(entities_xml, entity);

		if (indx != -1) {
			return entity_unicode_xml[indx];
		}
	}

	return -1;
}

/**
 * \brief Convert entities in string
 * \param inbuf input buffer
 * \return converted string
 */
static gchar *convert_entities(const gchar *inbuf)
{
	const gchar *found, *prevfound;
	gchar *outbuf, *outbufloc;

	outbuf = g_malloc0(strlen(inbuf) + 1);
	prevfound = inbuf;
	outbufloc = outbuf;
	found = g_utf8_strchr(inbuf, -1, '&');

	while (found) {
		gchar *endfound;

		endfound = g_utf8_strchr(found, -1, ';');
		if (endfound && endfound - found <= 7) {
			gchar *entity, tmp[7];
			gint len;
			gunichar unic;

			entity = g_strndup(found + 1, (endfound - found) - 1);
			len = (found - prevfound);
			memcpy(outbufloc, prevfound, len);
			outbufloc += len;
			unic = unichar_for_entity(entity, TRUE, TRUE, TRUE, TRUE, TRUE);
			if (unic != -1) {
				memset(tmp, 0, 7);
				len = g_unichar_to_utf8(unic, tmp);
				len = strlen(tmp);
				memcpy(outbufloc, tmp, len);
				outbufloc += len;
			} else {
				len = endfound - found + 1;
				memcpy(outbufloc, found, len);
				outbufloc += len;
			}
			g_free(entity);

			prevfound = g_utf8_next_char(endfound);
			found = g_utf8_strchr(prevfound, -1, '&');
		} else {
			found = g_utf8_strchr(g_utf8_next_char(found), -1, '&');
		}
	}

	memcpy(outbufloc, prevfound, strlen(prevfound) + 1);

	return outbuf;
}

/**
 * \brief Strip html entities from word
 * \param word html string
 * \return stripped string
 */
gchar *strip_html(gchar *word)
{
	GRegex *tags = g_regex_new("<(.|\n)*?>", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	GRegex *space = g_regex_new("&nbsp;", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	GRegex *misc = g_regex_new("Â", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	gchar *stripped = g_regex_replace_literal(tags, word, -1, 0, "", 0, NULL);
	gchar *spaced_str = g_regex_replace_literal(space, stripped, -1, 0, " ", 0, NULL);
	gchar *misc_str = g_regex_replace_literal(misc, spaced_str, -1, 0, "", 0, NULL);
	gchar *return_str = convert_entities(misc_str);
	g_free(misc_str);
	g_free(spaced_str);
	g_free(stripped);

	g_strstrip(return_str);

	return return_str;
}
