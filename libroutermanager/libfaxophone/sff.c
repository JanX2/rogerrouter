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
 * \file sff.c
 * \brief SFF handling functions
 */

#include <string.h>

#include <glib.h>

#include <libroutermanager/file.h>

#include <libfaxophone/faxophone.h>
#include <libfaxophone/fax.h>
#include <libfaxophone/sff.h>

static gchar *sff_data = NULL;
static gsize sff_len = 0;
static gsize sff_pos = 0;

/**
 * \brief Receive/Transmit fax state
 * \param connection capi connection pointer
 * \param capi_message current capi message
 */
static inline void sff_transfer(struct capi_connection *connection, _cmsg capi_message)
{
	struct session *session = faxophone_get_session();
	struct fax_status *status = connection->priv;
	_cmsg cmsg;
	gint transfer = CAPI_PACKETS;

	g_debug("processing fax");

	if (sff_len - sff_pos < transfer) {
		transfer = sff_len - sff_pos;
	}

	isdn_lock();
	DATA_B3_REQ(&cmsg, session->appl_id, 0, connection->ncci, sff_data + sff_pos, transfer, session->message_number++, 0);
	isdn_unlock();

	sff_pos += transfer;

	status->bytes_total = sff_len;
	status->bytes_sent = sff_pos;

	status->progress_status = 1;
	session->handlers->status(connection, 1);

	if (sff_pos == sff_len) {
		g_debug("EOF");
	} else {
		g_debug("Pos: %" G_GSIZE_FORMAT "/%" G_GSIZE_FORMAT " (%" G_GSIZE_FORMAT "%%)", sff_pos, sff_len, sff_pos * 100 / sff_len);
	}
}

gpointer sff_transfer_thread(gpointer data)
{
	struct capi_connection *connection = data;
	_cmsg cmsg;

	while (connection->state == STATE_CONNECTED && sff_pos < sff_len) {
		if (connection->use_buffers && connection->buffers < CAPI_BUFFERCNT) {
			g_debug("Buffers: %d", connection->buffers);
			sff_transfer(connection, cmsg);
			connection->buffers++;
		}

		g_usleep(50);
	}

	capi_hangup(connection);

	while (connection->state == STATE_CONNECTED && connection->buffers) {
		g_usleep(50);
	}


	return NULL;
}

void sff_init_data(struct capi_connection *connection)
{
	g_thread_new("sff transfer", sff_transfer_thread, connection);
}

/**
 * \brief Send Fax
 * \param sff_file The sff file to send
 * \param modem 0-3 (2400-14400)
 * \param ecm Error correction mode (on/off)
 * \param controller The controller for sending the fax
 * \param src_no MSN
 * \param trg_no Target fax number
 * \param ident Fax ident
 * \param header Fax header line
 * \param call_anonymous Send fax anonymous
 * \return error code
 */
struct capi_connection *sff_send(gchar *sff_file, gint modem, gint ecm, gint controller, const gchar *src_no, const gchar *trg_no, const gchar *ident, const gchar *header, gint call_anonymous)
{
	struct capi_connection *connection;
	_cstruct b1;
	_cstruct b2;
	_cstruct b3;
	int i = 0, j;

	g_debug(" ** SFF **");
	g_debug("sff: %s, modem: %d, ecm: %s, controller: %d, src: %s, trg: %s, ident: %s, header: %s, anonymous: %d)", sff_file, modem, ecm ? "on" : "off", controller, src_no, trg_no, (ident != NULL ? ident : "(null)"), (header != NULL ? header : "(null)"), call_anonymous);

	b1 = g_malloc0(2 + 2 + 2 + 2);
	b2 = NULL;
	b3 = g_malloc0(1 + 2 + 2 + 1 + strlen(ident) + 1 + strlen(header));

	/* Length */
	b3[i++] = 1 + 2 + 2 + 1 + strlen(ident) + 1 + strlen(header);
	/* Resolution: Standard */
	b3[i++] = 0;
	b3[i++] = 0;
	/* Format: SFF */
	b3[i++] = 0;
	b3[i++] = 0;

	/* Station ID */
	b3[i++] = strlen(ident);
	for (j = 0; j < strlen(ident); j++) {
		b3[i++] = ident[j];
	}

	/* Header */
	b3[i++] = strlen(header);
	for (j = 0; j < strlen(header); j++) {
		b3[i++] = header[j];
	}

	/* Open SFF file */
	sff_data = file_load(sff_file, &sff_len);
	sff_pos = 0;

	connection = capi_call(controller, src_no, trg_no, (guint) call_anonymous, SESSION_SFF, SFF_CIP, 4, 4, 5, b1, b2, b3);
	if (connection) {
		struct fax_status *status = NULL;

		connection->buffers = 0;
		connection->use_buffers = TRUE;

		status = malloc(sizeof(struct fax_status));
		memset(status, 0, sizeof(struct fax_status));

		connection->priv = status;
	}

	return connection;
}

/**
 * \brief Cleanup private fax structure from capi connection
 * \param connection capi connection
 */
void sff_clean(struct capi_connection *connection)
{
	connection->priv = NULL;
}
