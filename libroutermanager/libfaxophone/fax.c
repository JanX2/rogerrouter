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
 * \file fax.c
 * \brief Fax handling functions
 */

#include <glib.h>

#define SPANDSP_EXPOSE_INTERNAL_STRUCTURES
#include <spandsp.h>

#include <libfaxophone/faxophone.h>
#include <libfaxophone/fax.h>
#include <libfaxophone/isdn-convert.h>

static int8_t *_linear16_2_law = (int8_t *) &linear16_2_law[32768];
static uint16_t *_law_2_linear16 = &law_2_linear16[0];

static gint log_level = 0;

static void (*logging)(gint level, const gchar *text) = NULL;

/**
 * \brief Dump spandsp messages
 * \param level spandsp loglevel
 * \param text message text
 */
static void spandsp_msg_log(gint level, const gchar *text)
{
	g_debug("%s", text);
}

/**
 * \brief Realtime frame handler which keeps tracks of data transfer
 * \param state t30 state pointer
 * \param user_data pointer to connection index
 * \param direction transmission direction
 * \param msg spandsp message
 * \param len len of frame
 */
static void real_time_frame_handler(t30_state_t *state, void *user_data, gint direction, const uint8_t *msg, gint len)
{
	struct capi_connection *connection = user_data;
	struct fax_status *status = connection->priv;
	struct session *session = faxophone_get_session();
	t30_stats_t stats;

	//g_debug("real_time_frame_handler() called (%d/%d/%d)", direction, len, msg[2]);
	if (msg[2] == 6) {
		t30_get_transfer_statistics(state, &stats);

		if (status->sending) {
			status->bytes_total = stats.image_size;
			status->bytes_sent += len;
		} else {
			status->bytes_received += len;
			status->bytes_total += len;
		}

		status->progress_status = 1;
		session->handlers->status(connection, 1);
	}
}

/**
 * \brief Phase B handler
 * \param state t30 state pointer
 * \param user_data pointer to connection
 * \param result result
 * \return error code
 */
static gint phase_handler_b(t30_state_t *state, void *user_data, gint result)
{
	struct capi_connection *connection = user_data;
	struct fax_status *status = connection->priv;
	struct session *session = faxophone_get_session();
	t30_stats_t stats;
	t30_state_t *t30;
	const gchar *ident;

	t30_get_transfer_statistics(state, &stats);
	t30 = fax_get_t30_state(status->fax_state);

	g_debug("Phase B handler (0x%X) %s", result, t30_frametype(result));
	g_debug(" - bit rate %d", stats.bit_rate);
	g_debug(" - ecm %s", (stats.error_correcting_mode) ? "on" : "off");

	if (status->sending) {
		ident = t30_get_rx_ident(t30);
	} else {
		ident = t30_get_tx_ident(t30);
	}
	snprintf(status->remote_ident, sizeof(status->remote_ident), "%s", ident ? ident : "");
	g_debug("Remote side: '%s'", status->remote_ident);
	if (t30_get_rx_sender_ident(t30)) {
		g_debug("Remote side sender: '%s'", t30_get_rx_sender_ident(t30));
	}
	if (t30_get_rx_country(t30)) {
		g_debug("Remote side country: '%s'", t30_get_rx_country(t30));
	}
	if (t30_get_rx_vendor(t30)) {
		g_debug("Remote side vendor: '%s'", t30_get_rx_vendor(t30));
	}
	if (t30_get_rx_model(t30)) {
		g_debug("Remote side model: '%s'", t30_get_rx_model(t30));
	}

	status->phase = PHASE_B;
	status->bytes_sent = 0;
	status->bytes_total = 0;
	status->bytes_received = 0;
	status->ecm = stats.error_correcting_mode;
	status->bad_rows = stats.bad_rows;
	status->encoding = stats.encoding;
	status->bitrate = stats.bit_rate;
	//status->page_total = stats.pages_in_file;
	status->progress_status = 0;

	status->page_current = status->sending ? stats.pages_tx + 1 : stats.pages_rx + 1;

	session->handlers->status(connection, 0);

	return 0;
}

/**
 * \brief Phase D handler
 * \param state t30 state pointer
 * \param user_data pointer to connection
 * \param result result
 * \return error code
 */
static gint phase_handler_d(t30_state_t *state, void *user_data, gint result)
{
	struct capi_connection *connection = user_data;
	struct fax_status *status = connection->priv;
	struct session *session = faxophone_get_session();
	t30_stats_t stats;

	t30_get_transfer_statistics(state, &stats);

	g_debug("Phase D handler (0x%X) %s", result, t30_frametype(result));
	g_debug(" - pages transferred %d", status->sending ? stats.pages_tx : stats.pages_rx);
	/*g_debug(" - image size %d x %d", stats.width, stats.length);
	g_debug(" - bad rows %d", stats.bad_rows);
	g_debug(" - longest bad row run %d", stats.longest_bad_row_run);
	g_debug(" - image size %d", stats.image_size);*/

	status->phase = PHASE_D;
	/*status->ecm = stats.error_correcting_mode;
	status->bad_rows = stats.bad_rows;
	status->encoding = stats.encoding;
	status->bitrate = stats.bit_rate;*/

	if (status->sending) {
		status->page_current = (stats.pages_in_file >= stats.pages_tx + 1 ? stats.pages_tx + 1 : stats.pages_tx);
	} else {
		status->page_current = stats.pages_rx;
	}

	//status->page_total = stats.pages_in_file;
	status->bytes_received = 0;
	status->bytes_sent = 0;
	status->progress_status = 0;

	session->handlers->status(connection, 0);

	return 0;
}

/**
 * \brief Phase E handler
 * \param state T30 state
 * \param user_data pointer to current capi connection
 * \param result result code
 */
static void phase_handler_e(t30_state_t *state, void *user_data, gint result)
{
	struct capi_connection *connection = user_data;
	struct fax_status *status = connection->priv;
	struct session *session = faxophone_get_session();
	gint transferred = 0;
	t30_stats_t stats;
	t30_state_t *t30;
	const gchar *ident;

	t30_get_transfer_statistics(state, &stats);

	g_debug("Phase E handler (0x%X) %s", result, t30_completion_code_to_str(result));

	transferred = status->sending ? stats.pages_tx : stats.pages_rx;
	g_debug(" - pages transferred %d", transferred);
	/*g_debug(" - image resolution %d x %d", stats.x_resolution, stats.y_resolution);
	g_debug(" - compression type %d", stats.encoding);
	g_debug(" - coding method %s", t4_encoding_to_str(stats.encoding));*/

	status->phase = PHASE_E;
	/*status->ecm = stats.error_correcting_mode;
	status->bad_rows = stats.bad_rows;
	status->encoding = stats.encoding;
	status->bitrate = stats.bit_rate;*/

	status->page_current = (status->sending ? stats.pages_tx : stats.pages_rx);
	//status->page_total = stats.pages_in_file;
	status->error_code = result;

	t30 = fax_get_t30_state(status->fax_state);
	if (status->sending) {
		ident = t30_get_rx_ident(t30);
	} else {
		ident = t30_get_tx_ident(t30);
	}
	status->progress_status = 0;

	snprintf(status->remote_ident, sizeof(status->remote_ident), "%s", ident ? ident : "");
	g_debug("Remote station id: %s", status->remote_ident);

	session->handlers->status(connection, 0);
}

/**
 * \brief Get total pages in TIFF file
 * \param file filename
 * \return number of pages
 */
static int get_tiff_total_pages(const char *file)
{
	TIFF *tiff_file;
	int max;

	if ((tiff_file = TIFFOpen(file, "r")) == NULL) {
		return -1;
	}

	max = 0;
	while (TIFFSetDirectory(tiff_file, (tdir_t) max)) {
		max++;
	}

	TIFFClose(tiff_file);

	return max;
}

/**
 * \brief Initialize spandsp
 * \param tiff_file tiff file
 * \param sending sending flag
 * \param modem supported modem
 * \param ecm error correction mode flag
 * \param lsi lsi
 * \param local_header_info local header
 * \param connection capi connection poiner
 * \return error code
 */
gint spandsp_init(const gchar *tiff_file, gboolean sending, gchar modem, gchar ecm, const gchar *lsi, const gchar *local_header_info, struct capi_connection *connection)
{
	t30_state_t *t30;
	logging_state_t *log_state;
	gint supported_resolutions = 0;
	gint supported_image_sizes = 0;
	gint supported_modems = 0;
	struct fax_status *status = connection->priv;

	status->fax_state = fax_init(NULL, sending);
	g_debug("status->fax_state: %p", status->fax_state);

	fax_set_transmit_on_idle(status->fax_state, TRUE);
	fax_set_tep_mode(status->fax_state, FALSE);

	t30 = fax_get_t30_state(status->fax_state);

	/* Supported resolutions */
	supported_resolutions = 0;
	supported_resolutions |= T30_SUPPORT_STANDARD_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_FINE_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_SUPERFINE_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_R8_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_R16_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_300_300_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_400_400_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_600_600_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_1200_1200_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_300_600_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_400_800_RESOLUTION;
	supported_resolutions |= T30_SUPPORT_600_1200_RESOLUTION;

	/* Supported image sizes */
	supported_image_sizes = 0;
	supported_image_sizes |= T30_SUPPORT_215MM_WIDTH;
	supported_image_sizes |= T30_SUPPORT_255MM_WIDTH;
	supported_image_sizes |= T30_SUPPORT_303MM_WIDTH;
	supported_image_sizes |= T30_SUPPORT_UNLIMITED_LENGTH;
	supported_image_sizes |= T30_SUPPORT_A4_LENGTH;
	supported_image_sizes |= T30_SUPPORT_US_LETTER_LENGTH;
	supported_image_sizes |= T30_SUPPORT_US_LEGAL_LENGTH;

	/* Supported modems */
	supported_modems = 0;
	if (modem > 0) {
		supported_modems |= T30_SUPPORT_V27TER;
		if (modem > 1) {
			supported_modems |= T30_SUPPORT_V29;
		}
		if (modem > 2) {
			supported_modems |= T30_SUPPORT_V17;
		}
#if defined(T30_SUPPORT_V34)
		if (modem > 3) {
			supported_modems |= T30_SUPPORT_V34;
		}
#endif
	}

	t30_set_supported_modems(t30, supported_modems);

	/* Error correction */
	if (ecm) {
		/* Supported compressions */
#if defined(SPANDSP_SUPPORT_T85)
		t30_set_supported_compressions(t30, T30_SUPPORT_T4_1D_COMPRESSION | T30_SUPPORT_T4_2D_COMPRESSION | T30_SUPPORT_T6_COMPRESSION | T30_SUPPORT_T85_C
#else
		t30_set_supported_compressions(t30, T30_SUPPORT_T4_1D_COMPRESSION | T30_SUPPORT_T4_2D_COMPRESSION | T30_SUPPORT_T6_COMPRESSION);
#endif

		t30_set_ecm_capability(t30, ecm);
	}

	t30_set_supported_t30_features(t30, T30_SUPPORT_IDENTIFICATION | T30_SUPPORT_SELECTIVE_POLLING | T30_SUPPORT_SUB_ADDRESSING);
	t30_set_supported_resolutions(t30, supported_resolutions);
	t30_set_supported_image_sizes(t30, supported_image_sizes);

	/* spandsp loglevel */
	if (log_level >= 1) {
		log_state = t30_get_logging_state(t30);
		span_log_set_level(log_state, 0xFFFFFF);

		if (!logging) {
			logging = spandsp_msg_log;
		}

		span_log_set_message_handler(log_state, logging);
	}

	if (lsi) {
		t30_set_tx_ident(t30, lsi);
	}
	if (local_header_info) {
		t30_set_tx_page_header_info(t30, local_header_info);
	}

	if (sending == TRUE) {
		t30_set_tx_file(t30, tiff_file, -1, -1);
		status->page_total = get_tiff_total_pages(tiff_file);
	} else {
		t30_set_rx_file(t30, tiff_file, -1);
	}

	t30_set_phase_b_handler(t30, phase_handler_b, (void *) connection);
	t30_set_phase_d_handler(t30, phase_handler_d, (void *) connection);
	t30_set_phase_e_handler(t30, phase_handler_e, (void *) connection);

	t30_set_real_time_frame_handler(t30, real_time_frame_handler, (void *) connection);

	return 0;
}

/**
 * \brief Close spandsp
 * \param fax_state fax state
 * \return error code
 */
gint spandsp_close(fax_state_t *fax_state)
{
	struct fax_status *status = NULL;
	struct session *session = faxophone_get_session();
	gint i;

	g_debug("Close");
	if (fax_state != NULL) {
		fax_release(fax_state);
	} else {
		for (i = 0; i < CAPI_CONNECTIONS; i++) {
			status = session->connection[i].priv;

			if (status != NULL) {
				fax_release(status->fax_state);
			}
		}
	}

	return 0;
}

/**
 * \brief TX direction
 * \param fax_state fax state
 * \param buf transfer buffer
 * \param len length of buffer
 * \return error code
 */
gint spandsp_tx(fax_state_t *fax_state, uint8_t *buf, size_t len)
{
	int16_t buf_in[CAPI_PACKETS];
	uint8_t *alaw;
	gint err, i;

	err = fax_tx(fax_state, buf_in, CAPI_PACKETS);
	alaw = buf;

	for (i = 0; i != len; ++i, ++alaw) {
		*alaw = _linear16_2_law[(int16_t) buf_in[i]];
	}

	return err;
}

/**
 * \brief Process rx data through spandsp
 * \param fax_state fax state information
 * \param buf receive buffer
 * \param len length of buffer
 * \return error code
 */
gint spandsp_rx(fax_state_t *fax_state, uint8_t *buf, size_t len)
{
	int16_t buf_in[CAPI_PACKETS];
	int16_t *wave;
	gint err, i;

	wave = buf_in;

	for (i = 0; i != len; ++i, ++wave) {
		*wave = _law_2_linear16[(uint8_t) buf[i]];
	}

	err = fax_rx(fax_state, buf_in, CAPI_PACKETS);

	return err;
}

/**
 * \brief Receive/Transmit fax state
 * \param connection capi connection pointer
 * \param capi_message current capi message
 */
void fax_transfer(struct capi_connection *connection, _cmsg capi_message)
{
	struct fax_status *status = connection->priv;
	struct session *session = faxophone_get_session();
	_cmsg cmsg;
	guint8 alaw_buffer_tx[CAPI_PACKETS];
	gint32 len = DATA_B3_IND_DATALENGTH(&capi_message);

	/* RX/TX spandsp */
	spandsp_rx(status->fax_state, DATA_B3_IND_DATA(&capi_message), len);
	//isdn_lock();
	DATA_B3_RESP(&cmsg, session->appl_id, session->message_number++, connection->ncci, DATA_B3_IND_DATAHANDLE(&capi_message));
	//isdn_unlock();


	/* Send data to remote */
	len = CAPI_PACKETS;
	spandsp_tx(status->fax_state, alaw_buffer_tx, len);
	//isdn_lock();
	DATA_B3_REQ(&cmsg, session->appl_id, 0, connection->ncci, (void *) alaw_buffer_tx, len, session->message_number++, 0);
	//isdn_unlock();
}

/**
 * \brief Send Fax
 * \param tiff_file The Tiff file to send
 * \param modem 0-3 (2400-14400)
 * \param ecm Error correction mode (on/off)
 * \param controller The controller for sending the fax
 * \param src_no MSN
 * \param trg_no Target fax number
 * \param lsi Fax ident
 * \param local_header_info Fax header line
 * \param call_anonymous Send fax anonymous
 * \return error code
 */
struct capi_connection *fax_send(gchar *tiff_file, gint modem, gint ecm, gint controller, gint cip, const gchar *src_no, const gchar *trg_no, const gchar *lsi, const gchar *local_header_info, gint call_anonymous)
{
	struct fax_status *status;
	struct capi_connection *connection;

	g_debug("tiff: %s, modem: %d, ecm: %s, controller: %d, src: %s, trg: %s, ident: %s, header: %s, anonymous: %d)", tiff_file, modem, ecm ? "on" : "off", controller, src_no, trg_no, (lsi != NULL ? lsi : "(null)"), (local_header_info != NULL ? local_header_info : "(null)"), call_anonymous);

	//status = g_slice_new0(struct fax_status);
	status = malloc(sizeof(struct fax_status));
	memset(status, 0, sizeof(struct fax_status));

	status->phase = IDLE;
	status->error_code = -1;
	status->sending = 1;
	status->manual_hookup = 0;
	status->modem = modem;
	status->ecm = ecm;
	snprintf(status->header, sizeof(status->header), "%s", local_header_info);
	snprintf(status->ident, sizeof(status->ident), "%s", lsi);
	snprintf(status->src_no, sizeof(status->src_no), "%s", src_no);
	snprintf(status->trg_no, sizeof(status->trg_no), "%s", trg_no);
	snprintf(status->tiff_file, sizeof(status->tiff_file), "%s", tiff_file);

	connection = capi_call(controller, src_no, trg_no, (guint) call_anonymous, SESSION_FAX, cip, 1, 1, 0, NULL, NULL, NULL);
	if (connection) {
		status->connection = connection;
		connection->priv = status;
		spandsp_init(status->tiff_file, TRUE, status->modem, status->ecm, status->ident, status->header, connection);
	}

	return connection;
}

/**
 * \brief Set fax debug level
 * \param level debug level
 */
void fax_set_log_level(gint level)
{
	log_level = level;
}

/**
 * \brief Receive Fax
 * \param tiff_file The Tiff file for saving the fax
 * \param modem 0-3 (2400-14400)
 * \param ecm Error correction mode (on/off)
 * \param src_no MSN
 * \param trg_no After receiving a fax, dst_no is the senders fax number
 * \param manual_hookup: Hook up manually
 * \return error code
 */
gint fax_receive(struct capi_connection *connection, const gchar *tiff_file, gint modem, gint ecm, const gchar *src_no, gchar *trg_no, gint manual_hookup)
{
	struct fax_status *status = NULL;
	gint ret = -2;

	g_debug("tiff: %s, modem: %d, ecm: %s, src: %s, manual: %s)", tiff_file, modem, ecm ? "on" : "off", src_no, manual_hookup ? "on" : "off");

	if (!connection) {
		return ret;
	}

	status = malloc(sizeof(struct fax_status));
	memset(status, 0, sizeof(struct fax_status));

	status->phase = IDLE;
	status->sending = 0;
	status->modem = modem;
	status->ecm = ecm;
	status->manual_hookup = manual_hookup;
	status->error_code = -1;

	snprintf(status->src_no, sizeof(status->src_no), "%s", src_no);
	snprintf(status->tiff_file, sizeof(status->tiff_file), "%s", tiff_file);

	connection->priv = status;

	spandsp_init(status->tiff_file, FALSE, status->modem, status->ecm, status->ident, status->header, connection);

	snprintf(trg_no, sizeof(status->trg_no), "%s", status->trg_no);

	return 0;
}

/**
 * \brief Workaround for spandsp tx problem
 * \param connection capi connection
 */
void fax_spandsp_workaround(struct capi_connection *connection)
{
	struct fax_status *status = connection->priv;
	gint index;

	if (status->phase < PHASE_E) {
		g_debug("Spandsp is not yet completed - give it a little more time...");

		for (index = 0; index < 32768; index++) {
			uint8_t buf[CAPI_PACKETS];

			memset(buf, 128, CAPI_PACKETS);
			spandsp_rx(status->fax_state, buf, CAPI_PACKETS);
			spandsp_tx(status->fax_state, buf, CAPI_PACKETS);

			if (status->phase >= PHASE_E) {
				return;
			}
		}

		g_debug("Workaround failed, phase is still: %d", status->phase);
	}
}

/**
 * \brief Cleanup private fax structure from capi connection
 * \param connection capi connection
 */
void fax_clean(struct capi_connection *connection)
{
	struct fax_status *status = connection->priv;

	spandsp_close(status->fax_state);

	free(status);
	connection->priv = NULL;
}
