/*******************************************************************
 * libfaxophone                                                    *
 * Created by Jan-Michael Brummer                                  *
 * All parts are distributed under the terms of GPLv2. See COPYING *
 *******************************************************************/

/**
 * \file fax.h
 * \brief Fax structures
 */

#ifndef FAX_H
#define FAX_H

#include <spandsp.h>

/* Service indicator (0x04=speech, 0x11=fax/g3) */
#define SPEECH_CIP			0x04
#define FAX_CIP				0x11

enum fax_phase {
	IDLE = -1,
	CONNECT = 1,
	PHASE_B = 2,
	PHASE_D = 3,
	PHASE_E = 4,
};

struct fax_status {
	gchar tiff_file[256];
	gchar src_no[64];
	gchar trg_no[64];
	gchar ident[64];
	gchar header[64];
	gchar remote_ident[64];

	enum fax_phase phase;
	gint error_code;
	gboolean sending;
	gchar ecm;
	gchar modem;
	gint bitrate;
	gint encoding;
	gint bad_rows;
	gint page_current;
	gint page_total;
	gint bytes_received;
	gint bytes_sent;
	gint bytes_total;
	gboolean manual_hookup;
	gboolean done;

	fax_state_t *fax_state;
};

struct capi_connection *fax_send(const gchar *tiff_file, gint modem, gint ecm, gint controller, const gchar *src_no, const gchar *trg_no, const gchar *lsi, const gchar *local_header_info, gint call_anonymous);
gint fax_recv(const gchar *tiff_file, gint modem, gint ecm, const gchar *src_no, gchar *trg_no, const gchar *lsi, const gchar *local_header_info, gint manual_hookup);

void fax_transfer(struct capi_connection *connection, _cmsg message);
void fax_clean(struct capi_connection *connection);

void fax_spandsp_workaround(struct capi_connection *connection);

#endif
