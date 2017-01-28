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

#define _XOPEN_SOURCE 600

#include <config.h>
#include <glib.h>

#include <unistd.h>
#include <stdlib.h>

#ifndef	__APPLE__
#if  defined  G_OS_WIN32
 #include  <windows.h>
 #include  <wincrypt.h>
#else
#include <crypt.h>
#endif
#endif

#include <string.h>

#include <libroutermanager/password.h>

#define PASSWORD_KEY ")!$cvjh)"

/** Internal password manager list */
static GSList *pm_plugins = NULL;
guchar crypt_cfb_iv[64];
gint crypt_cfb_blocksize = 8;

#ifndef G_OS_WIN32
/*
* Shift len bytes from end of to buffer to beginning, then put len
* bytes from from at the end.  Caution: the to buffer is unpacked,
* but the from buffer is not.
*/
static void crypt_cfb_shift(unsigned char *to, const unsigned char *from, unsigned len)
{
	unsigned i;
	unsigned j;
	unsigned k;

	if (len < crypt_cfb_blocksize) {
		i = len * 8;
		j = crypt_cfb_blocksize * 8;
		for (k = i; k < j; k++) {
			to[0] = to[i];
			++to;
		}
	}

	for (i = 0; i < len; i++) {
		j = *from++;
		for (k = 0x80; k; k >>= 1)
			*to++ = ((j & k) != 0);
	}
}

/*
* XOR len bytes from from into the data at to.  Caution: the from buffer
* is unpacked, but the to buffer is not.
*/
static void crypt_cfb_xor(unsigned char *to, const unsigned char *from, unsigned len)
{
	unsigned i;
	unsigned j;
	unsigned char c;

	for (i = 0; i < len; i++) {
		c = 0;
		for (j = 0; j < 8; j++)
			c = (c << 1) | *from++;
		*to++ ^= c;
	}
}

/*
* Take the 8-byte array at *a (must be able to hold 64 bytes!) and unpack
* each bit into its own byte.
*/
static void crypt_unpack(unsigned char *a)
{
	int i, j;

	for (i = 7; i >= 0; --i)
		for (j = 7; j >= 0; --j)
			a[(i << 3) + j] = (a[i] & (0x80 >> j)) != 0;
}

static void crypt_cfb_buf(const char key[8], unsigned char *buf, unsigned len, unsigned chunksize, int decrypt)
{
	unsigned char temp[64];

	memcpy(temp, key, 8);
	crypt_unpack(temp);
	setkey((const char *) temp);
	memset(temp, 0, sizeof(temp));

	memset(crypt_cfb_iv, 0, sizeof(crypt_cfb_iv));

	if (chunksize > crypt_cfb_blocksize)
		chunksize = crypt_cfb_blocksize;

	while (len) {
		memcpy(temp, crypt_cfb_iv, sizeof(temp));
		encrypt((char *) temp, 0);
		if (chunksize > len)
			chunksize = len;
		if (decrypt)
			crypt_cfb_shift(crypt_cfb_iv, buf, chunksize);
		crypt_cfb_xor((unsigned char *) buf, temp, chunksize);
		if (!decrypt)
			crypt_cfb_shift(crypt_cfb_iv, buf, chunksize);
		len -= chunksize;
		buf += chunksize;
	}
}

static void password_encrypt(gchar *password, guint len)
{
	crypt_cfb_buf(PASSWORD_KEY, (guchar*)password, len, 1, 0);
}

static void password_decrypt(gchar *password, guint len)
{
	crypt_cfb_buf(PASSWORD_KEY, (guchar*)password, len, 1, 1);
}

gchar *password_encode(const gchar *in)
{
	gchar *tmp;
	gchar *b64;
	gsize len;

	tmp = g_strdup(in);
	len = strlen(tmp);
	password_encrypt(tmp, len);
	b64 = g_base64_encode((guchar*)tmp, len);
	memset(tmp, 0, len);
	g_free(tmp);

	return b64;
}

guchar *password_decode(const gchar *in)
{
	guchar *tmp;
	gsize len;

	tmp = g_base64_decode(in, &len);
	password_decrypt((gchar*)tmp, len);

	return tmp;
}
#else
gchar *password_encode(const gchar *in)
{
	return g_strdup(in);
}

guchar *password_decode(const gchar *in)
{
	return g_strdup(in);
}

#endif

/**
 * \brief Find password manager as requested by profile
 * \param profile profile structure
 * \return password manager pointer or NULL on error
 */
struct password_manager *password_manager_find(struct profile *profile)
{
	gchar *name = g_settings_get_string(profile->settings, "password-manager");
	GSList *list;

	for (list = pm_plugins; list != NULL; list = list->next) {
		struct password_manager *pm = list->data;

		if (pm && pm->name && name && !strcmp(pm->name, name)) {
			return pm;
		}
	}

	return pm_plugins ? pm_plugins->data : NULL;
}

/**
 * \brief Set password in manager
 * \param profile profile structure
 * \param name description of password
 * \param password password string
 */
void password_manager_set_password(struct profile *profile, const gchar *name, const gchar *password)
{
	struct password_manager *pm = password_manager_find(profile);

	pm->set_password(profile, name, password);
}

/**
 * \brief Get password from manager
 * \param profile profile structure
 * \param name description of password
 * \return password on success, NULL on error
 */
gchar *password_manager_get_password(struct profile *profile, const gchar *name)
{
	struct password_manager *pm = password_manager_find(profile);

	return pm->get_password(profile, name);
}

/**
 * \brief Remove password from manager
 * \param profile profile structure
 * \param name description of password
 * \return TRUE on success, FALSE on error
 */
gboolean password_manager_remove_password(struct profile *profile, const gchar *name)
{
	struct password_manager *pm = password_manager_find(profile);

	return pm->remove_password(profile, name);
}

/**
 * \brief Register password manager plugin
 * \param manager password manager plugin
 */
void password_manager_register(struct password_manager *manager)
{
	pm_plugins = g_slist_prepend(pm_plugins, manager);
}

/**
 * \brief Get a list of all password manager plugins
 * \return list of password manager plugins
 */
GSList *password_manager_get_plugins(void)
{
	return pm_plugins;
}
