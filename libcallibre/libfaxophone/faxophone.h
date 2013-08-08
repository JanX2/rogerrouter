/*******************************************************************
 * libfaxophone                                                    *
 * Created by Jan-Michael Brummer                                  *
 * All parts are distributed under the terms of GPLv2. See COPYING *
 *******************************************************************/

/**
 * \file faxophone.h
 * \brief faxophone main header
 */

#ifndef FAXOPHONE_H
#define FAXOPHONE_H

/* CAPI headers */
#include <capi20.h>

#ifdef __linux__
#include <linux/capi.h>
#else

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef struct capi_profile {
	uint16_t ncontroller;
	uint16_t nbchannel;
	uint32_t goptions;
	uint32_t support1;
	uint32_t support2;
	uint32_t support3;
	uint32_t reserved[6];
	uint32_t manu[5];
} capi_profile;
#endif

/* GLIB */
#include <glib.h>

/* C */
#include <ctype.h>
#include <unistd.h>

#include <sndfile.h>

#define CAPI_CONNECTIONS 5
/* Packet size */
#define CAPI_PACKETS 160
/* Packer buffer count */
#define CAPI_BUFFERCNT 6
/* max. B-Channels */
#define CAPI_BCHANNELS 2

#define isdn_lock() do { g_mutex_lock(&session->isdn_mutex); } while (0);
#define isdn_unlock() do { g_mutex_unlock(&session->isdn_mutex); } while (0);

#undef CREATE_THREAD
#define CREATE_THREAD(name, func, data) g_thread_new(name, func, data)

enum state {
	STATE_IDLE = 0,
	STATE_CONNECT_REQ,
	STATE_CONNECT_WAIT,
	STATE_CONNECT_ACTIVE,
	STATE_CONNECT_B3_WAIT,
	STATE_CONNECTED,
	STATE_DISCONNECT_B3_REQ,
	STATE_DISCONNECT_B3_WAIT,
	STATE_DISCONNECT_ACTIVE,
	STATE_DISCONNET_WAIT,
	STATE_RINGING,
	STATE_INCOMING_WAIT,
	STATE_MAXSTATE
};

enum session_type {
	SESSION_NONE,
	SESSION_FAX,
	SESSION_PHONE
};

#define RECORDING_BUFSIZE 32768
#define RECORDING_JITTER 200

enum recording {
	RECORDING_LOCAL,
	RECORDING_REMOTE
};

struct record_channel {
	gint64 position;
	short buffer[RECORDING_BUFSIZE];
};

struct recorder {
	SNDFILE *file;
	char *file_name;

	gint64 start_time;
	struct record_channel local;
	struct record_channel remote;
	gint64 last_write;
};

struct capi_connection {
	enum state state;
	enum session_type type;

	unsigned int id;
	unsigned int controller;
	unsigned long int plci;
	unsigned long int ncci;
	unsigned int capi_code;
	char *source;
	char *target;
	void *priv;
	int early_b3;
	int hold;
	time_t connect_time;
	int mute;
	int recording;
	double line_level_in_state;
	double line_level_out_state;
	struct recorder recorder;

	gpointer audio;

	void (*data)(struct capi_connection *connection, _cmsg capi_message);
	void (*clean)(struct capi_connection *connection);
};

struct session_handlers {
	gpointer (*audio_open)(void);
	gsize (*audio_input)(gpointer audio, guchar *buf, gsize len);
	gsize (*audio_output)(gpointer audio, guchar *buf, gsize len);
	gboolean (*audio_close)(gpointer audio);

	void (*connected)(struct capi_connection *connection);
	void (*disconnected)(struct capi_connection *connection);
	void (*ring)(struct capi_connection *connection);

	void (*code)(struct capi_connection *connection, int code);

	void (*status)(struct capi_connection *connection, int error_code);
};

struct session {
	GMutex isdn_mutex;

	struct capi_connection connection[CAPI_CONNECTIONS];
	int appl_id;
	int message_number;
	int input_thread_state;

	struct session_handlers *handlers;
};

struct capi_connection *capi_get_free_connection(void);
struct capi_connection *capi_call(unsigned, const char *, const char *, unsigned, unsigned, unsigned);
void capi_send_dtmf_code(struct capi_connection *connection, unsigned char nCode);
void capi_hangup(struct capi_connection *connection);
int capi_pickup(struct capi_connection *connection, int type);

struct session *faxophone_get_session(void);
struct session *faxophone_init(struct session_handlers *handlers, const char *host, gint controller);
int faxophone_close(int force);

#endif
