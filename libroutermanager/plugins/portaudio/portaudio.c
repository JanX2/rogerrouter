/**
 * Parts are based upon pablio.c:
 *
 * Portable Audio Blocking Input/Output utility.
 *
 * Author: Phil Burk, http://www.softsynth.com
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <portaudio.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/audio.h>
#include <libroutermanager/gstring.h>

#define ROUTERMANAGER_TYPE_PORTAUDIO_PLUGIN (routermanager_portaudio_plugin_get_type())
#define ROUTERMANAGER_PORTAUDIO_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_PORTAUDIO_PLUGIN, RouterManagerPortAudioPlugin))

typedef struct {
	guint id;
} RouterManagerPortAudioPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_PORTAUDIO_PLUGIN, RouterManagerPortAudioPlugin, routermanager_portaudio_plugin)

/* Does not work at the moment */
#define USE_SPEEX 1

/** predefined backup values */
static gint port_channels = 1;
static gint port_sample_rate = 8000;
static gint port_bits_per_sample = 16;

#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__x86_64__)
#define pa_util_read_memory_barrier() __asm__ volatile("lfence":::"memory")
#define pa_util_write_memory_barrier() __asm__ volatile("sfence":::"memory")
#else
#define pa_util_read_memory_barrier()
#define pa_util_write_memory_barrier()
#endif

#ifdef USE_SPEEX
#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>
#endif

#define FRAME_SIZE 160

struct pa_util_ring_buffer {
	long buffer_size;
	long write_index;
	long read_index;
	long big_mask;
	long small_mask;
	char *buffer;
};

typedef struct pa_util_ring_buffer pa_util_ring_buffer;

struct port_private {
	PaStream *in_stream;
	PaStream *out_stream;
	pa_util_ring_buffer out_fifo;
	pa_util_ring_buffer in_fifo;
	pa_util_ring_buffer echo_fifo;
	int bytes_per_frame;
#ifdef USE_SPEEX
	SpeexEchoState *echo_state;
	SpeexPreprocessState *preprocess_state;
#endif
};

/**
 * \brief Clear buffer
 * \param buffer ring buffer structure
 */
void pa_util_flush_ring_buffer(pa_util_ring_buffer *ring_buffer)
{
	ring_buffer->write_index = ring_buffer->read_index = 0;
}

/**
 * \brief Initialize ring buffer
 * \param buffer ring buffer structure
 * \param bytes buffer size, must be power of 2
 * \param data allocated buffer
 * \return 0 on success, otherwise error
 */
long pa_util_initialize_ring_buffer(pa_util_ring_buffer *ring_buffer, long bytes, void *data)
{
	if (((bytes - 1) & bytes) != 0) {
		g_warning("Size not power of 2");
		return -1;
	}

	ring_buffer->buffer_size = bytes;
	ring_buffer->buffer = (char *) data;
	pa_util_flush_ring_buffer(ring_buffer);

	ring_buffer->big_mask = (bytes * 2) - 1;
	ring_buffer->small_mask = (bytes) - 1;

	return 0;
}

/**
 * \brief Return number of bytes available for reading
 * \param buffer ring buffer structure
 * \return available size in bytes
 */
long pa_util_get_ring_buffer_read_available(pa_util_ring_buffer *ring_buffer)
{
	pa_util_read_memory_barrier();

	return ((ring_buffer->write_index - ring_buffer->read_index) & ring_buffer->big_mask);
}

/**
 * \brief Return number of bytes available for writing
 * \param buffer ring buffer structure
 * \return available size in bytes
 */
long pa_util_get_ring_buffer_write_available(pa_util_ring_buffer *ring_buffer)
{
	return (ring_buffer->buffer_size - pa_util_get_ring_buffer_read_available(ring_buffer));
}

/**
 * \brief Get address of region(s) to which we can write data
 * \param buffer ring buffer structure
 * \param bytes length of data
 * \param data_ptr1 first data pointer
 * \param size_ptr1 first data pointer length
 * \param data_ptr2 second data pointer
 * \param size_ptr2 second data pointer length
 * \return room available to be written
 */
long pa_util_get_ring_buffer_write_regions(pa_util_ring_buffer *ring_buffer, long bytes, void **data_ptr1, long *size_ptr1, void **data_ptr2, long *size_ptr2)
{
	long index;
	long available = pa_util_get_ring_buffer_write_available(ring_buffer);

	if (bytes > available) {
		bytes = available;
	}

	/* Check to see if write is not contiguous */
	index = ring_buffer->write_index & ring_buffer->small_mask;

	if ((index + bytes) > ring_buffer->buffer_size) {
		/* Write data in two blocks that wrap the buffer */
		long first_half = ring_buffer->buffer_size - index;

		*data_ptr1 = &ring_buffer->buffer[index];
		*size_ptr1 = first_half;
		*data_ptr2 = &ring_buffer->buffer[0];
		*size_ptr2 = bytes - first_half;
	} else {
		*data_ptr1 = &ring_buffer->buffer[index];
		*size_ptr1 = bytes;
		*data_ptr2 = NULL;
		*size_ptr2 = 0;
	}

	return bytes;
}

/**
 * \brief Get address of region(s) to which we can read data
 * \param buffer ring buffer structure
 * \param bytes length of data
 * \param data_ptr1 first data pointer
 * \param size_ptr1 first data pointer length
 * \param data_ptr2 second data pointer
 * \param size_ptr2 second data pointer length
 * \return room available to be read
 */
long pa_util_get_ring_buffer_read_regions(pa_util_ring_buffer *ring_buffer, long bytes, void **data_ptr1, long *size_ptr1, void **data_ptr2, long *size_ptr2)
{
	long index;
	long available = pa_util_get_ring_buffer_read_available(ring_buffer);

	if (bytes > available) {
		bytes = available;
	}

	/* Check to see if write is not contiguous */
	index = ring_buffer->read_index & ring_buffer->small_mask;

	if ((index + bytes) > ring_buffer->buffer_size) {
		/* Write data in two blocks that wrap the buffer */
		long first_half = ring_buffer->buffer_size - index;

		*data_ptr1 = &ring_buffer->buffer[index];
		*size_ptr1 = first_half;
		*data_ptr2 = &ring_buffer->buffer[0];
		*size_ptr2 = bytes - first_half;
	} else {
		*data_ptr1 = &ring_buffer->buffer[index];
		*size_ptr1 = bytes;
		*data_ptr2 = NULL;
		*size_ptr2 = 0;
	}

	return bytes;
}

/**
 * \brief Advance write index
 * \param buffer ring buffer structure
 * \param bytes number of bytes
 * \return new write index
 */
long pa_util_advance_ring_buffer_write_index(pa_util_ring_buffer *ring_buffer, long bytes)
{
	pa_util_write_memory_barrier();

	return ring_buffer->write_index = (ring_buffer->write_index + bytes) & ring_buffer->big_mask;
}

/**
 * \brief Advance read index
 * \param buffer ring buffer structure
 * \param bytes number of bytes
 * \return new read index
 */
long pa_util_advance_ring_buffer_read_index(pa_util_ring_buffer *ring_buffer, long bytes)
{
	pa_util_write_memory_barrier();

	return ring_buffer->read_index = (ring_buffer->read_index + bytes) & ring_buffer->big_mask;
}

/**
 * \brief Write to ring buffer
 * \param buffer ring buffer structure
 * \param data data that needs to be written
 * \param bytes length of data
 * \return number of written bytes
 */
long pa_util_write_ring_buffer(pa_util_ring_buffer *ring_buffer, const void *data, long bytes)
{
	long size1;
	long size2;
	long written;
	void *data1;
	void *data2;

	written = pa_util_get_ring_buffer_write_regions(ring_buffer, bytes, &data1, &size1, &data2, &size2);
	if (size2 > 0) {
		memcpy(data1, data, size1);
		data = ((char *) data) + size1;
		memcpy(data2, data, size2);
	} else {
		memcpy(data1, data, size1);
	}

	pa_util_advance_ring_buffer_write_index(ring_buffer, written);

	return written;
}

/**
 * \brief Read from ring buffer
 * \param buffer ring buffer structure
 * \param data data that needs to be written
 * \param bytes length of data
 * \return number of read bytes
 */
long pa_util_read_ring_buffer(pa_util_ring_buffer *ring_buffer, void *data, long bytes)
{
	long size1;
	long size2;
	long read;
	void *data1;
	void *data2;

	read = pa_util_get_ring_buffer_read_regions(ring_buffer, bytes, &data1, &size1, &data2, &size2);
	if (size2 > 0) {
		memcpy(data, data1, size1);
		data = ((char *) data) + size1;
		memcpy(data, data2, size2);
	} else {
		memcpy(data, data1, size1);
	}

	pa_util_advance_ring_buffer_read_index(ring_buffer, read);

	return read;
}

/**
 * \brief Detect supported audio devices
 * \return 0
 */
static GSList *port_audio_detect_devices(void)
{
	GSList *list = NULL;
	struct audio_device *device;
	gint num_devices;
	gint index;
	const PaDeviceInfo *info;

	Pa_Initialize();

	num_devices = Pa_GetDeviceCount();

	for (index = 0; index < num_devices; index++) {
		info = Pa_GetDeviceInfo(index);

		if (info->maxOutputChannels > 0) {
			device = g_slice_new0(struct audio_device);
			device->internal_name = g_strdup(info->name);
			device->name = g_strdup(info->name);
			device->type = AUDIO_OUTPUT;
			list = g_slist_prepend(list, device);
		}

		if (info->maxInputChannels > 0) {
			device = g_slice_new0(struct audio_device);
			device->internal_name = g_strdup(info->name);
			device->name = g_strdup(info->name);
			device->type = AUDIO_INPUT;
			list = g_slist_prepend(list, device);
		}
	}

	return list;
}

/**
 * \brief Initialize audio device
 * \param channels number of channels
 * \param sample_rate sample rate
 * \param bits_per_sample number of bits per samplerate
 * \return TRUE on success, otherwise error
 */
static int port_audio_init(unsigned char channels, unsigned short sample_rate, unsigned char bits_per_sample)
{
	Pa_Initialize();

	/* TODO: Check if configuration is valid and usable */
	port_channels = channels;
	port_sample_rate = sample_rate;
	port_bits_per_sample = bits_per_sample;

	return 0;
}

/**
 * \brief Perform echo cancellation - process input buffer with exitisting output buffer
 * \param private private data structure
 * \param input_buffer audio input buffer
 * \param samples number of samples
 * \return -1 if we have no existing output buffer, or 0 on success
 */
int echo_cancellation(struct port_private *private, short *input_buffer, int samples)
{
#ifdef USE_SPEEX
	short delayed[FRAME_SIZE];
	short cancelled[FRAME_SIZE];
	int speaking;

	/* If we have no existing output buffer, exit */
	if (pa_util_get_ring_buffer_read_available(&private->echo_fifo) < 2 * 160) {
		return -1;
	}

	/* Read output buffer from echo fifo */
	pa_util_read_ring_buffer(&private->echo_fifo, delayed, 2 * 160);

	/* Perform echo cancellation */
	speex_echo_cancellation(private->echo_state, input_buffer, delayed, cancelled);

	/* Overwrite input buffer with cancelled frame */
	memcpy(input_buffer, cancelled, samples * sizeof(short));

	/* Further preprocessing - e.g. advanced echo cancellation, VAG, AGC, redux */
	speaking = speex_preprocess_run(private->preprocess_state, input_buffer);
	if (!speaking) {
		memset(input_buffer, 0, samples * sizeof(short));
	}
#endif

	return 0;
}

/**
 * \brief Main audio callback system - called whenever we have an audio frame to process
 * \param input_buffer input buffer if it exist
 * \param output_buffer output buffer if it exist
 * \param frames_per_buffer number of frames per buffer
 * \param time_info additional time information
 * \param statusFlags status flags
 * \param user_data pointer to private data structure
 * \return 0
 */
static int audio_cb(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer, const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags statusFlags, void *user_data)
{
	struct port_private *private = user_data;
	long bytes = private->bytes_per_frame * frames_per_buffer;
	int ret = 0;

	if (output_buffer != NULL) {
		int index;
		int numRead = pa_util_read_ring_buffer(&private->out_fifo, output_buffer, bytes);

		for (index = numRead; index < bytes; index++) {
			((char *) output_buffer)[index] = 0;
		}

#ifdef USE_SPEEX
		/* Write it to echo fifo */
		pa_util_write_ring_buffer(&private->echo_fifo, output_buffer, bytes);
#endif
	}

	if (input_buffer != NULL) {
		/* Call echo cancellation */
		ret = echo_cancellation(private, (short *) input_buffer, frames_per_buffer);

		if (ret == 0) {
			/* If we have a valid frame, write it to the input fifo */
			int numRead = pa_util_write_ring_buffer(&private->in_fifo, input_buffer, bytes);

			if (numRead != bytes) {
				pa_util_flush_ring_buffer(&private->in_fifo);
				pa_util_write_ring_buffer(&private->in_fifo, input_buffer, bytes);
			}
		}
	}

	return 0;
}

/**
 * \brief Initialize fifo - allocate ring buffer size and initialize it
 * \param buffer ring buffer we need to setup
 * \param frames number of frames
 * \param bytes_per_frame bytes per frame
 */
PaError init_fifo(pa_util_ring_buffer *ring_buffer, long frames, long bytes_per_frame)
{
	/* Calculate whole size in bytes */
	long bytes = frames * bytes_per_frame;
	char *buffer = g_malloc0(bytes);

	if (buffer == NULL) {
		return paInsufficientMemory;
	}

	return (PaError) pa_util_initialize_ring_buffer(ring_buffer, bytes, buffer);
}

/**
 * \brief Terminate fifo
 * \param buffer ring buffer pointer
 */
PaError term_fifo(pa_util_ring_buffer *buffer)
{
	if (buffer && buffer->buffer) {
		g_free(buffer->buffer);
	}

	buffer->buffer = NULL;

	return paNoError;
}

/**
 * \brief Round number to next power of 2
 * \param n number
 * \return power of 2
 */
unsigned long RoundUpToNextPowerOf2(unsigned long n)
{
	long bits = 0;

	if (((n - 1) & n) == 0) {
		return n;
	}

	while (n > 0) {
		n = n >> 1;
		bits++;
	}

	return (1 << bits);
}

/**
 * \brief Stop and remove pipeline
 * \param priv private data
 * \return error code
 */
int port_audio_close(void *priv)
{
	struct port_private *private = priv;
	//int bytesEmpty;

	if (private == NULL) {
		return 0;
	}

	g_debug("now out stream..");
	if (private->out_stream) {
		/*int nByteSize = private->out_fifo.buffer_size;

		if (nByteSize > 0) {
			bytesEmpty = pa_util_get_ring_buffer_write_available(&private->out_fifo);

			while (bytesEmpty < nByteSize) {
				Pa_Sleep(10);
				bytesEmpty = pa_util_get_ring_buffer_write_available(&private->out_fifo);
			}
		}*/

		if (!Pa_IsStreamStopped(private->out_stream)) {
			Pa_AbortStream(private->out_stream);
		}
		Pa_CloseStream(private->out_stream);

		private->out_stream = NULL;

		term_fifo(&private->out_fifo);
	}
	g_debug("now in stream..");

	if (private->in_stream) {
		g_debug("Check Pa_IsStreamStopped()");
		if (!Pa_IsStreamStopped(private->in_stream)) {
			g_debug("Pa_AbortStream()");
			Pa_AbortStream(private->in_stream);
		}

		g_debug("Pa_CloseStream()");
		Pa_CloseStream(private->in_stream);
		private->in_stream = NULL;

		g_debug("term_fifo()");
		term_fifo(&private->in_fifo);
	}

#ifdef USE_SPEEX
	g_debug("kill speex");

	if (private->preprocess_state) {
		speex_preprocess_state_destroy(private->preprocess_state);
		private->preprocess_state = NULL;
	}
	if (private->echo_state) {
		speex_echo_state_destroy(private->echo_state);
		private->echo_state = NULL;
	}
#endif
	g_debug("end");
	g_free(private);

	return 0;
}

/**
 * \brief Open audio device
 * \return private data or NULL on error
 */
static void *port_audio_open(void)
{
	struct port_private *private = g_malloc(sizeof(struct port_private));
	PaStreamParameters output_parameters;
	PaStreamParameters input_parameters;
	PaError err;
	struct profile *profile = profile_get_active();
	const gchar *playback = g_settings_get_string(profile->settings, "audio-output");
	const gchar *capture = g_settings_get_string(profile->settings, "audio-input");
	const PaDeviceInfo *info;
	int num_devices = Pa_GetDeviceCount();
	int index;

	if (private == NULL) {
		return private;
	}
	memset(private, 0, sizeof(struct port_private));

	index = num_devices;
	if (playback != NULL) {
		for (index = 0; index < num_devices; index++) {
			info = Pa_GetDeviceInfo(index);

			if ((info->maxOutputChannels > 0) && !strcmp(info->name, playback)) {
				g_debug("Settings wants output device: %d:%s (channels: %d)", index, playback, info->maxOutputChannels);
				break;
			}
		}
	}

	if (index >= num_devices) {
		index = Pa_GetDefaultOutputDevice();
		info = Pa_GetDeviceInfo(index);
		if (info) {
			g_debug("Settings wants output device: %s (channels: %d)", playback, info->maxOutputChannels);
			g_debug("Device can not be found, so we are using the systems default device: %d:%s", index, info->name);
		}
	}

	private->bytes_per_frame = 2;

	int num_frames = RoundUpToNextPowerOf2(FRAME_SIZE * 10);

#ifdef USE_SPEEX
	/* Maximum attenuation of residual echo in dB (negative number) */
#define ECHO_SUPPRESS -60
	/* Maximum attenuation of residual echo when near end is active, in dB (negative number) */
#define ECHO_SUPPRESS_ACTIVE -60

	int nTmp;

	/* Echo canceller with 20ms tail length */
	private->echo_state = speex_echo_state_init(FRAME_SIZE, 512);
	private->preprocess_state = speex_preprocess_state_init(FRAME_SIZE, port_sample_rate);

	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, private->echo_state);
	nTmp = ECHO_SUPPRESS;
	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &nTmp);
	nTmp = ECHO_SUPPRESS_ACTIVE;
	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &nTmp);

	/* Enable denoising */
	nTmp = 1;
	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &nTmp);

	/* Enable Automatic Gain Control */
	nTmp = 1;
	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_AGC, &nTmp);

	/* Set AGC increment */
	nTmp = 12;
	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &nTmp);
	/* Set AGC decrement */
	nTmp = -40;
	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &nTmp);

	/* Enable Voice Activity Detection */
	//nTmp = 1;
	//speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_VAD, &nTmp);

	/* Enable reverberation removal */
	nTmp = 1;
	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_DEREVERB, &nTmp);
#if 0
	/*nTmp = 35;
	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_PROB_START, &nTmp);
	nTmp = 20;
	speex_preprocess_ctl(private->preprocess_state, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &nTmp);*/
#endif

	err = init_fifo(&private->echo_fifo, num_frames, private->bytes_per_frame);
	if (err) {
		g_debug("Could not init echo fifo");
		port_audio_close(private);

		return NULL;
	}
#endif

	g_debug("sample rate: %d, channels: %d", port_sample_rate, port_channels);
	output_parameters.device = index;
	output_parameters.channelCount = port_channels;
	output_parameters.sampleFormat = paInt16;
	output_parameters.hostApiSpecificStreamInfo = NULL;
	output_parameters.suggestedLatency = Pa_GetDeviceInfo(output_parameters.device)->defaultLowOutputLatency;

	err = init_fifo(&private->out_fifo, num_frames, private->bytes_per_frame);
	if (err) {
		g_debug("Could not init output fifo");
		port_audio_close(private);

		return NULL;
	}

	err = Pa_OpenStream(&private->out_stream, NULL, &output_parameters, port_sample_rate, FRAME_SIZE, paClipOff, &audio_cb, private);
	if (err == paNoError) {
		Pa_StartStream(private->out_stream);
	} else {
		g_warning("Sorry, could not open output stream (%s)", Pa_GetErrorText(err));
		port_audio_close(private);

		return NULL;
	}

	index = num_devices;
	if (capture != NULL) {
		for (index = 0; index < num_devices; index++) {
			info = Pa_GetDeviceInfo(index);

			if ((info->maxInputChannels > 0) && !strcmp(info->name, capture)) {
				g_debug("Settings wants input device: %d:%s", index, capture);
				break;
			}
		}
	}

	if (index >= num_devices) {
		index = Pa_GetDefaultInputDevice();
		info = Pa_GetDeviceInfo(index);
		g_debug("Settings wants input device: %s", capture);
		g_debug("Device can not be found, so we are using the systems default device: %d:%s", index, info->name);
	}

	input_parameters.device = index;
	input_parameters.channelCount = port_channels;
	input_parameters.sampleFormat = paInt16;
	input_parameters.hostApiSpecificStreamInfo = NULL;
	input_parameters.suggestedLatency = Pa_GetDeviceInfo(input_parameters.device)->defaultLowInputLatency;

	err = init_fifo(&private->in_fifo, num_frames, private->bytes_per_frame);
	if (err) {
		g_debug("Could not init input fifo");
		port_audio_close(private);

		return NULL;
	}

	err = Pa_OpenStream(&private->in_stream, &input_parameters, NULL, port_sample_rate, FRAME_SIZE, paClipOff, &audio_cb, private);
	if (err == paNoError) {
		Pa_StartStream(private->in_stream);
	} else {
		g_warning("Sorry, could not open input stream (%s)", Pa_GetErrorText(err));
		port_audio_close(private);

		private = NULL;
	}

	return private;
}

/**
 * \brief Write audio data
 * \param priv private data
 * \param data audio data
 * \param size size of audio data
 * \return bytes written or error code
 */
static gsize port_audio_write(void *priv, guchar *data, gsize size)
{
	struct port_private *private = priv;
	//PaError err;
	int written = 0;
	int offset = 0;

	if (private == NULL) {
		return 0;
	}

again:
	written = pa_util_write_ring_buffer(&private->out_fifo, data + offset, size - offset);
	if (written != size - offset) {
		int bytesEmpty = 0;
		bytesEmpty = pa_util_get_ring_buffer_write_available(&private->out_fifo);

		while (bytesEmpty < size - offset) {
			Pa_Sleep(1);
			bytesEmpty = pa_util_get_ring_buffer_write_available(&private->out_fifo);
		}
		offset = written;
		goto again;
	}

	return written;
}

/**
 * \brief Read audio data
 * \param priv private data pointer
 * \param data output buffer pointer
 * \param size size of buffer
 * \return total bytes read
 */
static gsize port_audio_read(void *priv, guchar *data, gsize size)
{
	struct port_private *private = priv;
	long total_bytes = 0;
	long avail;
	int max = 5000;
	long read;
	char *ptr = (char *) data;

	while (total_bytes < size && --max > 0) {
		avail = pa_util_get_ring_buffer_read_available(&private->in_fifo);

		read = 0;

		if (avail >= size) {
			read = pa_util_read_ring_buffer(&private->in_fifo, ptr, size);
			total_bytes += read;
			ptr += read;
		} else {
			Pa_Sleep(1);
		}
	}

	return total_bytes;
}

/**
 * \brief Deinit audio
 * \return see error code of Pa_Terminate()
 */
int port_audio_shutdown(void)
{
	return Pa_Terminate();
}

/** audio definition */
struct audio port_audio = {
	"PortAudio",
	port_audio_init,
	port_audio_open,
	port_audio_write,
	port_audio_read,
	port_audio_close,
	port_audio_shutdown,
	port_audio_detect_devices
};

/**
 * \brief Activate plugin (add net event)
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	routermanager_audio_register(&port_audio);
}

/**
 * \brief Deactivate plugin (remote net event)
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
}
