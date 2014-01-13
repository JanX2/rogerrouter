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
 * \file phone.c
 * \brief Phone handling functions
 */

#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdint.h>

#include "faxophone.h"
#include "phone.h"
#include "isdn-convert.h"

/* Close recording */
int recording_close(struct recorder *recorder);

/**
 * \brief Input audio handler
 * \param data capi connection pointer
 * \return NULL
 */
gpointer phone_input_thread(gpointer data)
{
	struct session *session = faxophone_get_session();
	struct capi_connection *connection = data;
	guchar audio_buffer_rx[CAPI_PACKETS];
	guchar audio_buffer[CAPI_PACKETS * 2];
	guint audio_buf_len;
	short rec_buffer[CAPI_PACKETS];
	_cmsg cmsg;

	while (session->input_thread_state == 1) {
		int len;

		len = session->handlers->audio_input(connection->audio, (guchar *) audio_buffer_rx, sizeof(audio_buffer_rx));

		/* Check if we have some audio data to process */
		if (len > 0) {
			/* convert audio data to isdn format */
			convert_audio_to_isdn(connection, (guchar *) audio_buffer_rx, len, audio_buffer, &audio_buf_len, rec_buffer);

			isdn_lock();
			DATA_B3_REQ(&cmsg, session->appl_id, 0, connection->ncci, audio_buffer, audio_buf_len, session->message_number++, 0);
			isdn_unlock();
		}
	}

	session->input_thread_state = 0;

	if (connection->recording == 1) {
		recording_close(&connection->recorder);
	}

	return NULL;
}

void phone_init_data(struct capi_connection *connection)
{
	struct session *session = faxophone_get_session();

	g_debug("phone_init_data()");
	if (session->input_thread_state == 0) {
		session->input_thread_state = 1;

		CREATE_THREAD("phone-input", phone_input_thread, connection);
	}
}

/**
 * \brief Phone transfer routine which accepts incoming data, converts and outputs the audio
 * \param connection active capi connection
 * \param sCapiMessage current capi message
 */
void phone_transfer(struct capi_connection *connection, _cmsg capi_message)
{
	struct session *session = faxophone_get_session();
	_cmsg cmsg;
	guchar audio_buffer[CAPI_PACKETS * 2];
	guint len = DATA_B3_IND_DATALENGTH(&capi_message);
	guint audio_buf_len;
	short rec_buffer[8192];

	/* convert isdn to audio format */
	convert_isdn_to_audio(connection, DATA_B3_IND_DATA(&capi_message), len, audio_buffer, &audio_buf_len, rec_buffer);
	/* Send capi response */
	isdn_lock();
	DATA_B3_RESP(&cmsg, session->appl_id, session->message_number++, connection->ncci, DATA_B3_IND_DATAHANDLE(&capi_message));
	isdn_unlock();

	/* Send data to soundcard */
	session->handlers->audio_output(connection->audio, audio_buffer, audio_buf_len);
}

/**
 * \brief Dial a given number and set connection type to SESSION_PHONE
 * \param nController capi controller
 * \param source source number (own MSN)
 * \param target remote number (we want to dial)
 * \param anonymous anonymous flag (suppress number)
 * \return seee capiCall
 */
struct capi_connection *phone_call(guchar controller, const char *source, const char *target, gboolean anonymous)
{
	return capi_call(controller, source, target, anonymous, SESSION_PHONE, PHONE_CIP, 1, 1, 0, NULL, NULL, NULL);
}

/**
 * \brief Mute/unmute active capi connection
 * \param connection active capi connection
 * \param mute mute flag
 */
void phone_mute(struct capi_connection *connection, guchar mute)
{
	/* Just set the flag, the audio input thread handle the mute case */
	connection->mute = mute;
}

/**
 * \brief Return current time in microseconds
 * \return time in microseconds
 */
guint64 microsec_time(void)
{
	struct timeval time_val;

	gettimeofday(&time_val, 0);

	return time_val.tv_sec * ((guint64) 1000000) + time_val.tv_usec;
}

/**
 * \brief Initialize recording structure
 * \param recorder pointer to recorder structure
 * \return 0
 */
int recording_init(struct recorder *recorder)
{
	memset(recorder, 0, sizeof(struct recorder));

	return 0;
}

/**
 * \brief Open recording file
 * \param recorder pointer to recorder structure
 * \param file record file
 * \return 0 on success, otherwise error
 */
int recording_open(struct recorder *recorder, char *file)
{
	SF_INFO sInfo;

	if (access(file, F_OK)) {
		/* File not present */
		sInfo.format = SF_FORMAT_WAV | SF_FORMAT_ULAW;
		sInfo.channels = 2;
		sInfo.samplerate = 8000;

		if (!(recorder->file = sf_open(file, SFM_WRITE, &sInfo))) {
			printf("Error creating record file\n");
			return -1;
		}
	} else {
		sInfo.format = 0;
		if (!(recorder->file = sf_open(file, SFM_RDWR, &sInfo))) {
			printf("Error opening record file\n");
			return -1;
		}
		if (sf_seek(recorder->file, 0, SEEK_END) == -1) {
			printf("Error seeking record file\n");
			return -1;
		}
	}

	recorder->file_name = strdup(file);
	recorder->last_write = 0;

	memset(&recorder->local, 0, sizeof(struct record_channel));
	memset(&recorder->remote, 0, sizeof(struct record_channel));

	recorder->start_time = microsec_time();

	return 0;
}

/**
 * \brief Write audio data to record file
 * \param recorder recorder structure
 * \param buf audio buffer
 * \param size size of audio buffer
 * \param channel channel type (local/remote)
 * \return 0 on success, otherwise error
 */
int recording_write(struct recorder *recorder, short *buf, int size, int channel)
{
	gint64 start = recorder->start_time;
	gint64 current, start_pos, position, end_pos;
	int buf_pos, split, delta;
	struct record_channel *buffer;

	if (start == 0) {
		return 0;
	}

	if (size < 1) {
		printf("Warning: Illegal size!\n");
		return -1;
	}

	switch (channel) {
	case RECORDING_LOCAL:
		buffer = &recorder->local;
		break;
	case RECORDING_REMOTE:
		buffer = &recorder->remote;
		break;
	default:
		printf("Recording to unknown channel!\n");
		return -1;
	}

	current = microsec_time() - start;

	end_pos = current * 8000 / 1000000LL;
	start_pos = end_pos - size;
	position = buffer->position;

	if (start_pos >= position - RECORDING_JITTER && start_pos <= position + RECORDING_JITTER) {
		start_pos = position;
		end_pos = position + size;
	}

	if (start_pos < position) {
		delta = (int) position - start_pos;
		start_pos = position;
		buf += delta;
		size -= delta;
		if (size <= 0) {
			return 0;
		}
	}

	buf_pos = start_pos % RECORDING_BUFSIZE;

	if (buf_pos + size <= RECORDING_BUFSIZE) {
		memcpy(buffer->buffer + buf_pos, buf, size * sizeof(short));
	} else {
		split = RECORDING_BUFSIZE - buf_pos;
		memcpy(buffer->buffer + buf_pos, buf, split * sizeof(short));
		buf += split;
		size -= split;
		memcpy(buffer->buffer, buf, size * sizeof(short));
	}

	buffer->position = end_pos;

	return 0;
}

/**
 * \brief Flush recording buffer
 * \param recorder recording structure
 * \param last last call flag
 * \return 0 on success, otherwise error
 */
int recording_flush(struct recorder *recorder, guint last)
{
	gint64 max_position = recorder->local.position;
	gint64 tmp = recorder->remote.position;
	gint64 start_position = recorder->last_write;
	short rec_buf[RECORDING_BUFSIZE * 2];
	gint64 src_ptr, dst_ptr, size;

	if (recorder->start_time == 0) {
		return 0;
	}

	if (tmp > max_position) {
		max_position = tmp;
	}

	if (start_position + (RECORDING_BUFSIZE * 7 / 8) < max_position) {
		start_position = max_position - (RECORDING_BUFSIZE * 7 / 8);
	}

	if (!last) {
		max_position -= RECORDING_BUFSIZE / 8;
	}

	size = (gint64)(max_position - start_position);
	if (max_position == 0 || start_position >= max_position || (!last && size < RECORDING_BUFSIZE / 8)) {
		return 0;
	}

	dst_ptr = 0;
	src_ptr = start_position % RECORDING_BUFSIZE;

	while (--size) {
		rec_buf[dst_ptr++] = recorder->local.buffer[src_ptr];
		recorder->local.buffer[src_ptr] = 0;
		rec_buf[dst_ptr++] = recorder->remote.buffer[src_ptr];
		recorder->remote.buffer[src_ptr] = 0;

		if (++src_ptr >= RECORDING_BUFSIZE) {
			src_ptr = 0;
		}
	}

	sf_writef_short(recorder->file, rec_buf, dst_ptr / 2);

	recorder->last_write = max_position;

	return 0;
}

/**
 * \brief Close recording structure
 * \param recorder recorder structure
 * \return 0 on success, otherwise error
 */
int recording_close(struct recorder *recorder)
{
	int result = 0;

	if (recorder->start_time) {
		if (recording_flush(recorder, 1) < 0) {
			result = -1;
		}
		recorder->start_time = 0;

		if (recorder->file_name) {
			free(recorder->file_name);
			recorder->file_name = NULL;
		}

		if (sf_close(recorder->file) != 0) {
			printf("Error closing record file!\n");
			result = -1;
		}
	}

	return result;
}

/**
 * \brief Flush connection recorder
 * \param connection capi connection
 */
void phone_flush(struct capi_connection *connection)
{
	if (connection != NULL) {
		recording_flush(&connection->recorder, 0);
	}
}

/**
 * \brief Start/stop recording of active capi connection
 * \param connection active capi connection
 * \param record record flag
 * \param dir storage directory
 */
void phone_record(struct capi_connection *connection, guchar record, const char *dir)
{
	if (record == 1) {
		gchar *file = NULL;
		struct tm *time_val = localtime(&connection->connect_time);

		if (connection->recording == 0) {
			recording_init(&connection->recorder);
		}

		file = g_strdup_printf("%s/%2.2d.%2.2d.%4.4d-%2.2d-%2.2d-%s-%s.wav",
		                       dir,
		                       time_val->tm_mday, time_val->tm_mon, 1900 + time_val->tm_year,
		                       time_val->tm_hour, time_val->tm_min, connection->source, connection->target);

		recording_open(&connection->recorder, file);
		g_free(file);
	} else {
		if (connection->recording == 1) {
			recording_close(&connection->recorder);
		}
	}

	connection->recording = record;
}

/**
 * \brief Hold and retrieve active capi connection
 * \param connection active capi connection
 * \param hold hold flag
 */
void phone_hold(struct capi_connection *connection, guchar hold)
{
	struct session *session = faxophone_get_session();
	_cmsg message;
	_cbyte fac[9];

	/* Save state */
	connection->hold = hold;

	/* Generate facility structure */
	fac[0] = 3;
	fac[1] = (_cbyte)(0x0003 - hold) & 0xFF;
	fac[2] = (_cbyte)((0x0003 - hold) >> 8) & 0xFF;
	fac[3] = 0;

	isdn_lock();
	if (hold == 1) {
		/* Hold active connection */
		FACILITY_REQ(&message, session->appl_id, 0, connection->ncci, 3, (guchar *) fac);
	} else {
		/* Retrieve active connection */
		FACILITY_REQ(&message, session->appl_id, 0, connection->plci, 3, (guchar *) fac);
	}
	isdn_unlock();
}

/**
 * \brief Send DTMF codes on connection
 * \param connection active capi connection
 * \param code DTMF code
 */
void phone_send_dtmf_code(struct capi_connection *connection, guchar code)
{
	capi_send_dtmf_code(connection, code);
}

/**
 * \brief Hangup phone connection
 * \param connection active capi connection
 */
void phone_hangup(struct capi_connection *connection)
{
	if (connection == NULL) {
		return;
	}

	/* Hangup */
	capi_hangup(connection);
}

/**
 * \brief Pickup a phone call
 * \param connection active capi connection
 * \return see capiPickup
 */
int phone_pickup(struct capi_connection *connection)
{
	if (connection == NULL) {
		return -1;
	}

	/* Pickup connection and set connection type to SESSION_PHONE */
	return capi_pickup(connection, SESSION_PHONE);
}

void phone_conference(struct capi_connection *active, struct capi_connection *hold)
{
	struct session *session = faxophone_get_session();
	_cmsg message;
	_cbyte fac[24];
	void *ptr = &(fac[4]);

	/* Generate facility structure */
	memset(fac, 0, sizeof(fac));
	/* Len */
	fac[0] = 7;
	fac[1] = 0x07;
	fac[2] = 0x00;
	fac[3] = 4;

	((unsigned char *)ptr)[0] = hold->plci & 0xff;
	((unsigned char *)ptr)[1] = (hold->plci >> 8) & 0xff;
	((unsigned char *)ptr)[2] = (hold->plci >> 16) & 0xff;
	((unsigned char *)ptr)[3] = (hold->plci >> 24) & 0xff;

	isdn_lock();
	/* Hold active connection */
	FACILITY_REQ(&message, session->appl_id, 0, active->ncci, 3, (guchar *) fac);
	isdn_unlock();
}
