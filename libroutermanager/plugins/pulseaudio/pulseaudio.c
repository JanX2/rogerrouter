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
#include <stdio.h>
#include <errno.h>

#include <glib.h>

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/audio.h>
#include <libroutermanager/gstring.h>

#define ROUTERMANAGER_TYPE_PULSEAUDIO_PLUGIN (routermanager_pulseaudio_plugin_get_type())
#define ROUTERMANAGER_PULSEAUDIO_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_PULSEAUDIO_PLUGIN, RouterManagerPulseAudioPlugin))

typedef struct {
	guint id;
} RouterManagerPulseAudioPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_PULSEAUDIO_PLUGIN, RouterManagerPulseAudioPlugin, routermanager_pulseaudio_plugin)

struct pulse_pipes {
	pa_simple *simple_in;
	pa_simple *simple_out;
};

/** predefined backup values */
static gint pulse_channels = 1;
static gint pulse_sample_rate = 8000;
static gint pulse_bits_per_sample = 16;

struct pulse_device_list {
	gchar initialized;
	gchar name[512];
	gint index;
	gchar description[256];
};

/** This is where we'll store the input device list */
struct pulse_device_list input_device_list[16];
/** This is where we'll store the output device list */
struct pulse_device_list output_device_list[16];

/**
 * \brief This callback gets called when our context changes state.
 * \param context pulseaudio context
 * \param user_data pa ready flag
 */
static void pulse_state_cb(pa_context *context, void *user_data)
{
	pa_context_state_t state;
	int *pulse_ready = user_data;

	state = pa_context_get_state(context);

	switch (state) {
	/* There are just here for reference */
	case PA_CONTEXT_UNCONNECTED:
	case PA_CONTEXT_CONNECTING:
	case PA_CONTEXT_AUTHORIZING:
	case PA_CONTEXT_SETTING_NAME:
	default:
		break;
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		*pulse_ready = 2;
		break;
	case PA_CONTEXT_READY:
		*pulse_ready = 1;
		break;
	}
}

/**
 * \brief mainloop will call this function when it's ready to tell us about a sink.
 * \param context pulseaudio context
 * \param sink_info sink information
 * \param eol end-of-list
 * \param user_data pointer to device list
 */
static void pulse_sink_list_cb(pa_context *context, const pa_sink_info *sink_info, int eol, void *user_data)
{
	struct pulse_device_list *device_list = user_data;
	int index = 0;

	/* If eol is set to a positive number, you're at the end of the list */
	if (eol > 0) {
		return;
	}

	for (index = 0; index < 16; index++) {
		if (!device_list[index].initialized) {
			strncpy(device_list[index].name, sink_info->name, 511);
			strncpy(device_list[index].description, sink_info->description, 255);
			device_list[index].index = sink_info->index;
			device_list[index].initialized = 1;
			break;
		}
	}
}

/**
 * \brief See above.  This callback is pretty much identical to the previous
 * \param context pulseaudio context
 * \param source_info source information
 * \param eol end-of-list
 * \param user_data pointer to device list
 */
static void pulse_source_list_cb(pa_context *context, const pa_source_info *source_info, int eol, void *user_data)
{
	struct pulse_device_list *device_list = user_data;
	int index = 0;

	if (eol > 0) {
		return;
	}

	for (index = 0; index < 16; index++) {
		if (!device_list[index].initialized) {
			strncpy(device_list[index].name, source_info->name, 511);
			strncpy(device_list[index].description, source_info->description, 255);
			device_list[index].index = source_info->index;
			device_list[index].initialized = 1;
			break;
		}
	}
}

/**
 * \brief Get device list for input and output devices
 * \param input pointer input device list
 * \param output pointer output device list
 * \return error code
 */
static int pulse_get_device_list(struct pulse_device_list *input, struct pulse_device_list *output)
{
	/* Define our pulse audio loop and connection variables */
	pa_mainloop *main_loop;
	pa_mainloop_api *main_api;
	pa_operation *operation = NULL;
	pa_context *context;
	/* We'll need these state variables to keep track of our requests */
	int state = 0;
	int ready = 0;

	/* Initialize our device lists */
	memset(input, 0, sizeof(struct pulse_device_list) * 16);
	memset(output, 0, sizeof(struct pulse_device_list) * 16);

	/* Create a mainloop API and connection to the default server */
	main_loop = pa_mainloop_new();
	main_api = pa_mainloop_get_api(main_loop);
	context = pa_context_new(main_api, "test");

	/* This function connects to the pulse server */
	pa_context_connect(context, NULL, 0, NULL);

	/**
	 * This function defines a callback so the server will tell us it's state.
	 * Our callback will wait for the state to be ready.  The callback will
	 * modify the variable to 1 so we know when we have a connection and it's
	 * ready.
	 * If there's an error, the callback will set pa_ready to 2
	 */
	pa_context_set_state_callback(context, pulse_state_cb, &ready);

	/**
	 * Now we'll enter into an infinite loop until we get the data we receive
	 * or if there's an error
	 */
	for (; ;) {
		/**
		 * We can't do anything until PA is ready, so just iterate the mainloop
		 * and continue
		 */
		if (ready == 0) {
			pa_mainloop_iterate(main_loop, 1, NULL);
			continue;
		}

		/* We couldn't get a connection to the server, so exit out */
		if (ready == 2) {
			pa_context_disconnect(context);
			pa_context_unref(context);
			pa_mainloop_free(main_loop);
			return -1;
		}

		/**
		 * At this point, we're connected to the server and ready to make
		 * requests
		 */
		switch (state) {
		/* State 0: we haven't done anything yet */
		case 0:
			operation = pa_context_get_sink_info_list(context, pulse_sink_list_cb, output);

			/* Update state for next iteration through the loop */
			state++;
			break;
		case 1:
			if (pa_operation_get_state(operation) == PA_OPERATION_DONE) {
				pa_operation_unref(operation);

				operation = pa_context_get_source_info_list(context, pulse_source_list_cb, input);

				/* Update the state so we know what to do next */
				state++;
			}
			break;
		case 2:
			if (pa_operation_get_state(operation) == PA_OPERATION_DONE) {
				pa_operation_unref(operation);
				pa_context_disconnect(context);
				pa_context_unref(context);
				pa_mainloop_free(main_loop);
				return 0;
			}
			break;
		default:
			g_warning("in state %d", state);
			return -1;
		}

		pa_mainloop_iterate(main_loop, 1, NULL);
	}
}

/**
 * \brief Detect pulseaudio devices
 * \return list of audio devices
 */
GSList *pulse_audio_detect_devices(void)
{
	int found_in = 0;
	int found_out = 0;
	int index;
	GSList *list = NULL;
	struct audio_device *device;

	if (pulse_get_device_list(input_device_list, output_device_list) < 0) {
		g_warning("failed to get device list");
		return list;
	}

	for (index = 0; index < 16; index++) {
		if (!output_device_list[index].initialized || strstr(output_device_list[index].name, ".monitor")) {
			continue;
		}
		found_out++;

		device = g_slice_new0(struct audio_device);
		device->internal_name = g_strdup(output_device_list[index].name);
		device->name = g_strdup(output_device_list[index].description);
		device->type = AUDIO_OUTPUT;
		list = g_slist_prepend(list, device);
	}

	for (index = 0; index < 16; index++) {
		if (!input_device_list[index].initialized || strstr(input_device_list[index].name, ".monitor")) {
			continue;
		}

		device = g_slice_new0(struct audio_device);
		device->internal_name = g_strdup(input_device_list[index].name);
		device->name = g_strdup(input_device_list[index].description);
		device->type = AUDIO_INPUT;
		list = g_slist_prepend(list, device);

		found_in++;
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
static int pulse_audio_init(unsigned char channels, unsigned short sample_rate, unsigned char bits_per_sample)
{
	/* TODO: Check if configuration is valid and usable */
	pulse_channels = channels;
	pulse_sample_rate = sample_rate;
	pulse_bits_per_sample = bits_per_sample;

	return 0;
}

/**
 * \brief Open new playback and record pipes
 * \return pipe pointer or NULL on error
 */
static void *pulse_audio_open(void)
{
	pa_sample_spec sample_spec = {
		.format = PA_SAMPLE_S16LE,
	};
	int error;
	pa_buffer_attr buffer = {
		.fragsize = 320,
		.maxlength = -1,
		.minreq = -1,
		.prebuf = -1,
		.tlength = -1,
	};
	struct pulse_pipes *pipes = malloc(sizeof(struct pulse_pipes));
	const gchar *output;
	const gchar *input;

	if (!pipes) {
		g_warning("Could not get memory for pipes");
		return NULL;
	}

	sample_spec.rate = pulse_sample_rate;
	sample_spec.channels = pulse_channels;
	if (pulse_bits_per_sample == 2) {
		sample_spec.format = PA_SAMPLE_S16LE;
	}

	output = g_settings_get_string(profile_get_active()->settings, "audio-output");
	if (EMPTY_STRING(output)) {
		output = NULL;
	}

	pipes->simple_out = pa_simple_new(NULL, "Roger Router", PA_STREAM_PLAYBACK, output, "phone", &sample_spec, NULL, NULL, &error);
	if (pipes->simple_out == NULL) {
		g_debug("Pulseaudio - Could not open output device '%s'. Error: %s", output ? output : "", pa_strerror(error));
		free(pipes);
		return NULL;
	}

	input = g_settings_get_string(profile_get_active()->settings, "audio-input");
	if (EMPTY_STRING(input)) {
		input = NULL;
	}

	pipes->simple_in = pa_simple_new(NULL, "Roger Router", PA_STREAM_RECORD, input, "phone", &sample_spec, NULL, &buffer, &error);
	if (pipes->simple_in == NULL) {
		g_debug("Pulseaudio - Could not open input device '%s'. Error: %s", input ? input : "", pa_strerror(error));
		//pa_simple_free(pipes->simple_out);
		//free(pipes);
		//return NULL;
	}

	return pipes;
}

/**
 * \brief Close audio pipelines
 * \param priv pointer to private input/output pipes
 * \return error code
 */
static int pulse_audio_close(void *priv)
{
	struct pulse_pipes *pipes = priv;

	if (!pipes) {
		return -EINVAL;
	}

	if (pipes->simple_out) {
		pa_simple_free(pipes->simple_out);
		pipes->simple_out = NULL;
	}

	if (pipes->simple_in) {
		pa_simple_free(pipes->simple_in);
		pipes->simple_in = NULL;
	}

	free(pipes);

	return 0;
}

/**
 * \brief Write data to audio device
 * \param priv pointer to private pipes
 * \param buf audio data pointer
 * \param len length of buffer
 * \return error code
 */
static gsize pulse_audio_write(void *priv, guchar *buf, gsize len)
{
	struct pulse_pipes *pipes = priv;
	int error;

	if (pipes == NULL || pipes->simple_out == NULL) {
		return -1;
	}

	if (pa_simple_write(pipes->simple_out, buf, (size_t) len, &error) < 0) {
		g_debug("Failed: %s", pa_strerror(error));
	}

	return 0;
}

/**
 * \brief Read data from audio device
 * \param priv pointer to private pipes
 * \param buf audio data pointer
 * \param len maximal length of buffer
 * \return error code
 */
static gsize pulse_audio_read(void *priv, guchar *buf, gsize len)
{
	struct pulse_pipes *pipes = priv;
	int nRet = 0;
	int error;

	if (!pipes || !pipes->simple_in) {
		return 0;
	}

	nRet = pa_simple_read(pipes->simple_in, buf, len, &error);
	if (nRet < 0) {
		g_debug("Failed: %s", pa_strerror(error));
		len = 0;
	}

	return len;
}

/**
 * \brief Shutdown audio interface
 * \return 0
 */
static int pulse_audio_shutdown(void)
{
	return 0;
}

/** audio definition */
struct audio pulse_audio = {
	"PulseAudio",
	pulse_audio_init,
	pulse_audio_open,
	pulse_audio_write,
	pulse_audio_read,
	pulse_audio_close,
	pulse_audio_shutdown,
	pulse_audio_detect_devices
};

/**
 * \brief Activate plugin (add net event)
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	//RouterManagerPulseAudioPlugin *pulseaudio_plugin = ROUTERMANAGER_PULSEAUDIO_PLUGIN(plugin);

	/* Set media role */
	g_setenv("PULSE_PROP_media.role", "phone", TRUE);
	g_setenv("PULSE_PROP_filter.want", "echo-cancel", TRUE);

	routermanager_audio_register(&pulse_audio);
}

/**
 * \brief Deactivate plugin (remote net event)
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	//RouterManagerPulseAudioPlugin *pulseaudio_plugin = ROUTERMANAGER_PULSEAUDIO_PLUGIN(plugin);
}
