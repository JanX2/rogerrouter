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

#define _XOPEN_SOURCE 600

#include <rmconfig.h>
#include <glib.h>

#include <unistd.h>
#include <stdlib.h>

#ifndef __APPLE__
#if  defined  G_OS_WIN32
 #include  <windows.h>
 #include  <wincrypt.h>
#else
#include <crypt.h>
#endif
#endif

#include <string.h>

#include <rm/rmpassword.h>

#define PASSWORD_KEY ")!$cvjh)"

/** Internal password manager list */
static GSList *rm_password_plugins = NULL;
guchar rm_crypt_cfb_iv[64];
gint rm_crypt_cfb_blocksize = 8;

#ifndef G_OS_WIN32
/*
 * rm_crypt_cfb_shift:
 * @to: shift to
 * @from: shift from
 * @len: length of shift
 *
 * Shift len bytes from end of to buffer to beginning, then put len
 * bytes from from at the end.  Caution: the to buffer is unpacked,
 * but the from buffer is not.
 */
static void rm_crypt_cfb_shift(unsigned char *to, const unsigned char *from, unsigned len)
{
	unsigned i;
	unsigned j;
	unsigned k;

	if (len < rm_crypt_cfb_blocksize) {
		i = len * 8;
		j = rm_crypt_cfb_blocksize * 8;
		for (k = i; k < j; k++) {
			to[0] = to[i];
			++to;
		}
	}

	for (i = 0; i < len; i++) {
		j = *from++;

		for (k = 0x80; k; k >>= 1) {
			*to++ = ((j & k) != 0);
		}
	}
}

/*
 * rm_crypt_cfb_xor:
 * @to: shift to
 * @from: shift from
 * @len: length of shift
 *
 * XOR len bytes from from into the data at to.  Caution: the from buffer
 * is unpacked, but the to buffer is not.
 */
static void rm_crypt_cfb_xor(unsigned char *to, const unsigned char *from, unsigned len)
{
	unsigned i;
	unsigned j;
	unsigned char c;

	for (i = 0; i < len; i++) {
		c = 0;

		for (j = 0; j < 8; j++) {
			c = (c << 1) | *from++;
		}

		*to++ ^= c;
	}
}

/*
 * rm_crypt_unpack:
 * @a: 8-byte array
 *
 * Take the 8-byte array at @a (must be able to hold 64 bytes!) and unpack
 * each bit into its own byte.
 */
static void rm_crypt_unpack(unsigned char *a)
{
	int i, j;

	for (i = 7; i >= 0; --i) {
		for (j = 7; j >= 0; --j) {
			a[(i << 3) + j] = (a[i] & (0x80 >> j)) != 0;
		}
	}
}

static void crypt_cfb_buf(const char key[8], unsigned char *buf, unsigned len, unsigned chunksize, int decrypt)
{
	unsigned char temp[64];

	memcpy(temp, key, 8);
	rm_crypt_unpack(temp);
	setkey((const char *) temp);
	memset(temp, 0, sizeof(temp));

	memset(rm_crypt_cfb_iv, 0, sizeof(rm_crypt_cfb_iv));

	if (chunksize > rm_crypt_cfb_blocksize)
		chunksize = rm_crypt_cfb_blocksize;

	while (len) {
		memcpy(temp, rm_crypt_cfb_iv, sizeof(temp));
		encrypt((char *) temp, 0);
		if (chunksize > len)
			chunksize = len;
		if (decrypt)
			rm_crypt_cfb_shift(rm_crypt_cfb_iv, buf, chunksize);
		rm_crypt_cfb_xor((unsigned char *) buf, temp, chunksize);
		if (!decrypt)
			rm_crypt_cfb_shift(rm_crypt_cfb_iv, buf, chunksize);
		len -= chunksize;
		buf += chunksize;
	}
}

/**
 * rm_password_encrypt_cfb:
 * @password: password string
 * @len: len of password
 *
 * Encrypts a given @password using cfb.
 */
static void rm_password_encrypt_cfb(gchar *password, guint len)
{
	crypt_cfb_buf(PASSWORD_KEY, (guchar*)password, len, 1, 0);
}

/**
 * rm_password_decrypt_cfb:
 * @password: password string
 * @len: len of password
 *
 * Decrypts a given @password using cfb.
 */
static void rm_password_decrypt_cfb(gchar *password, guint len)
{
	crypt_cfb_buf(PASSWORD_KEY, (guchar*)password, len, 1, 1);
}

/**
 * rm_password_encode:
 * @in: input string
 *
 * Encodes @in string and returns result.
 *
 * Returns: encoded string
 */
gchar *rm_password_encode(const gchar *in)
{
	gchar *tmp;
	gchar *b64;
	gsize len;

	tmp = g_strdup(in);
	len = strlen(tmp);
	rm_password_encrypt_cfb(tmp, len);
	b64 = g_base64_encode((guchar*)tmp, len);
	memset(tmp, 0, len);
	g_free(tmp);

	return b64;
}

/**
 * rm_password_encode:
 * @in: input string
 *
 * Decodes @in string and returns result.
 *
 * Returns: decoded string
 */
guchar *rm_password_decode(const gchar *in)
{
	guchar *tmp;
	gsize len;

	tmp = g_base64_decode(in, &len);
	rm_password_decrypt_cfb((gchar*)tmp, len);

	return tmp;
}
#else
/**
 * rm_password_encode:
 * @in: input string
 *
 * Encodes @in string and returns result.
 *
 * Returns: encoded string
 */
gchar *rm_password_encode(const gchar *in)
{
	return g_strdup(in);
}

/**
 * rm_password_encode:
 * @in: input string
 *
 * Decodes @in string and returns result.
 *
 * Returns: decoded string
 */
guchar *rm_password_decode(const gchar *in)
{
	return (guchar*)g_strdup(in);
}

#endif

/**
 * rm_password_find_manager:
 * @profile: a #RmProfile
 *
 * Find password manager as requested by profile.
 *
 * Returns: password manager pointer or NULL on error.
 */
RmPasswordManager *rm_password_find_manager(RmProfile *profile)
{
	gchar *name = g_settings_get_string(profile->settings, "password-manager");
	GSList *list;

	for (list = rm_password_plugins; list != NULL; list = list->next) {
		RmPasswordManager *pm = list->data;

		if (pm && pm->name && name && !strcmp(pm->name, name)) {
			return pm;
		}
	}

	return rm_password_plugins ? rm_password_plugins->data : NULL;
}

/**
 * rm_password_set:
 * @profile: a #RmProfile
 * @name: description of password
 * @password: password string
 *
 * Set password in manager.
 */
void rm_password_set(RmProfile *profile, const gchar *name, const gchar *password)
{
	RmPasswordManager *pm = rm_password_find_manager(profile);

	g_assert(pm != NULL);

	pm->set(profile, name, password);
}

/**
 * rm_password_get:
 * @profile: a #RmProfile
 * @name: description of password
 *
 * Get password from manager.
 *
 * Returns: password on success, NULL on error
 */
gchar *rm_password_get(RmProfile *profile, const gchar *name)
{
	RmPasswordManager *pm = rm_password_find_manager(profile);

	g_assert(pm != NULL);

	return pm->get(profile, name);
}

/**
 * rm_password_remove:
 * @profile: a #RmProfile
 * @name: description of password
 *
 * Remove password from manager.
 *
 * Returns: TRUE on success, FALSE on error
 */
gboolean rm_password_remove(RmProfile *profile, const gchar *name)
{
	RmPasswordManager *pm = rm_password_find_manager(profile);

	g_assert(pm != NULL);

	return pm->remove(profile, name);
}

/**
 * rm_password_register:
 * @manager: a #RmPasswordManager
 *
 * Register password manager plugin.
 */
void rm_password_register(RmPasswordManager *manager)
{
	rm_password_plugins = g_slist_prepend(rm_password_plugins, manager);
}

/**
 * rm_password_get_plugins:
 *
 * Get a list of all password manager plugins
 *
 * Returns: list of password manager plugins
 */
GSList *rm_password_get_plugins(void)
{
	return rm_password_plugins;
}
