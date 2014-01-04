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

#ifndef FIRMWARE_COMMON_H
#define FIRMWARE_COMMON_H

#include <string.h>

#include <libroutermanager/router.h>

G_BEGIN_DECLS

#define FIRMWARE_IS(major, minor) (((profile->router_info->maj_ver_id == major) && (profile->router_info->min_ver_id >= minor)) || (profile->router_info->maj_ver_id > major))

struct voice_data {
	/* 0 */
	gint header;
	/* 4 */
	gint index;
	/* 8 (2=own message, 3=remote message) */
	gint type;
	/* 12 */
	guint sub_type;
	/* 16 */
	guint size;
	/* 20 */
	guint duration;
	/* 24 */
	guint status;
	/* 28 */
	guchar tmp0[24];
	/* 52 */
	gchar remote_number[54];
	/* 106 */
	gchar tmp1[18];
	/* 124 */
	gchar file[32];
	/* 151 */
	gchar path[128];
	/* 279 */
	guchar day;
	guchar month;
	guchar year;
	guchar hour;
	guchar minute;
	guchar tmp2[31];
	gchar local_number[24];
	gchar tmp3[4];
};

struct voice_box {
	gsize len;
	gpointer data;
};

extern struct phone_port fritzbox_phone_ports[NUM_PHONE_PORTS];

gchar *xml_extract_tag(const gchar *data, gchar *tag);
gchar *xml_extract_input_value(const gchar *data, gchar *tag);
gchar *xml_extract_list_value(const gchar *data, gchar *tag);
gchar *html_extract_assignment(const gchar *data, gchar *tag, gboolean p);
gboolean fritzbox_present(struct router_info *router_info);
gboolean fritzbox_logout(struct profile *profile, gboolean force);
void fritzbox_read_msn(struct profile *profile, const gchar *data);
gboolean fritzbox_dial_number(struct profile *profile, gint port, const gchar *number);
gboolean fritzbox_hangup(struct profile *profile, gint port, const gchar *number);
gchar *fritzbox_load_fax(struct profile *profile, const gchar *filename, gsize *len);
gchar *fritzbox_load_voice(struct profile *profile, const gchar *filename, gsize *len);
GSList *fritzbox_load_voicebox(GSList *journal);
GSList *fritzbox_load_faxbox(GSList *journal);
gint fritzbox_find_phone_port(gint dial_port);
gchar *fritzbox_get_ip(struct profile *profile);
gboolean fritzbox_reconnect(struct profile *profile);
gboolean fritzbox_delete_fax(struct profile *profile, const gchar *filename);
gboolean fritzbox_delete_voice(struct profile *profile, const gchar *filename);

/**
 * \brief Make dots (UTF8 -> UTF16)
 * \param str UTF8 string
 * \return UTF16 string
 */
static inline gchar *make_dots(const gchar *str)
{
	GString *new_str = g_string_new("");
	gunichar chr;
	gchar *next;

	while (str && *str) {
		chr = g_utf8_get_char(str);
		next = g_utf8_next_char(str);

		if (chr > 255) {
			new_str = g_string_append_c(new_str, '.');
		} else {
			new_str = g_string_append_c(new_str, chr);
		}

		str = next;
	}

	return g_string_free(new_str, FALSE);
}

/**
 * \brief Compute md5 sum of input string
 * \param input - input string
 * \return md5 in hex or NULL on error
 */
static inline gchar *md5(gchar *input)
{
	GError *error = NULL;
	gchar *ret = NULL;
	gsize written;
	gchar *bin = g_convert(input, -1, "UTF-16LE", "UTF-8", NULL, &written, &error);

	if (error == NULL) {
		ret = g_compute_checksum_for_string(G_CHECKSUM_MD5, (gchar *) bin, written);
		g_free(bin);
	} else {
		g_debug("Error converting utf8 to utf16: '%s'", error->message);
		g_error_free(error);
	}

	return ret;
}

G_END_DECLS

#endif
