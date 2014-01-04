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

#include <stdlib.h>
#include <string.h>

#include <gst/base/gstadapter.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappbuffer.h>
#include <gst/interfaces/propertyprobe.h>

#include <pulse/pulseaudio.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/audio.h>
#include <libroutermanager/gstring.h>

#define ROUTERMANAGER_TYPE_GSTREAMER_PLUGIN (routermanager_gstreamer_plugin_get_type())
#define ROUTERMANAGER_GSTREAMER_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_GSTREAMER_PLUGIN, RouterManagerGStreamerPlugin))

typedef struct {
	guint id;
} RouterManagerGStreamerPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_GSTREAMER_PLUGIN, RouterManagerGStreamerPlugin, routermanager_gstreamer_plugin)

/** predefined backup values */
static gint gstreamer_channels = 2;
static gint gstreamer_sample_rate = 8000;
static gint gstreamer_bits_per_sample = 16;

/** Device information, description and name */
struct device_info {
	gchar type;
	gchar *descr;
	gchar *name;
};

/** Device test list */
struct device_test {
	gchar type;
	gchar *pipe;
	gchar *test;
} device_test[] = {
	{1, "pulsesink", "pulsesinkpresencetest"},
	{2, "pulsesrc", "pulsesrcpresencetest"},
	{1, "alsasink", "alsasinkpresencetest"},
	{2, "alsasrc", "alsasrcpresencetest"},
	{1, "osxaudiosink", "osxaudiosinkpresencetest"},
	{2, "osxaudiosrc", "osxaudiosrcpresencetest"},
	{1, "directsoundsink", "directsoundsinkpresencetest"},
	{2, "directsoundsrc", "directsoundsrcpresencetest"},
	{0, NULL, NULL}
};

struct pipes {
	GstElement *in_pipe;
	GstElement *out_pipe;
	GstElement *in_bin;
	GstElement *out_bin;
	GstAdapter *adapter;
};

/**
 * \brief Stop and clean gstreamer pipeline
 * \param bus gstreamer bus
 * \param message gstreamer message
 * \param pipeline pipeline to pipeline we want to stop
 * \return TRUE if we received a EOS for the requested pipeline, otherwise FALSE
 */
static gboolean pipeline_cleaner(GstBus *bus, GstMessage *message, gpointer pipeline)
{
	gboolean result = TRUE;

	/* If we receive a End-Of-Stream message for the requested pipeline, stop the pipeline */
	if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS && GST_MESSAGE_SRC(message) == GST_OBJECT_CAST(pipeline)) {
		result = FALSE;
		gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
		g_object_unref(pipeline);
	}

	return result;
}

/**
 * \brief Detect supported audio devices
 * \return 0
 */
static GSList *gstreamer_detect_devices(void)
{
	GSList *devices = NULL;
	struct audio_device *audio_device;
	int index = 0;
	int device_index = 0;

	for (device_index = 0; device_test[device_index].type != 0; device_index++) {
		GstElement *element = NULL;

		/* check if we have a sink within gstreamer */
		g_debug("Factory for: '%s'", device_test[device_index].pipe);
		element = gst_element_factory_make(device_test[device_index].pipe, device_test[device_index].test);
		if (!element) {
			continue;
		}

		/* Set playback mode to pause */
		gst_element_set_state(element, GST_STATE_PAUSED);

		GstPropertyProbe *probe = NULL;
		const GParamSpec *spec = NULL;
		GValueArray *array = NULL;
		probe = GST_PROPERTY_PROBE(element);
		spec = gst_property_probe_get_property(probe, "device");

		/* now try to receive the index name and add it to the internal list */
		array = gst_property_probe_probe_and_get_values(probe, spec);
		g_debug("array: %p", array);
		if (array != NULL) {
			g_debug("array values: %d", array->n_values);
			for (index = 0; index < array->n_values; index++) {
				GValue *device_val = NULL;
				gchar *name = NULL;

				device_val = g_value_array_get_nth(array, index);
				g_object_set(G_OBJECT(element), "device", g_value_get_string(device_val), NULL);
				g_object_get(G_OBJECT(element), "device-name", &name, NULL);
				g_debug("device %p", g_value_get_string(device_val));
				g_debug("device-name %p", name);
				if (name != NULL) {
					g_debug("name: %s", name);
					audio_device = g_slice_new0(struct audio_device);
					audio_device->internal_name = g_strdup_printf("%s device=%s", device_test[device_index].pipe, g_value_get_string(device_val));
					audio_device->name = g_strdup(name);

					audio_device->type = device_test[device_index].type == 1 ? AUDIO_OUTPUT : AUDIO_INPUT;
					devices = g_slist_prepend(devices, audio_device);
					g_free(name);
				}
			}
			g_value_array_free(array);
		}

		gst_element_set_state(element, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(element));

		if (device_test[device_index].type == 2  && g_slist_length(devices) > 0) {
			//	break;
		}
	}

	return devices;
}

/**
 * \brief Set buffer size we want to use
 * \param buffer_size requested buffer size
 */
static void gstreamer_set_buffer_output_size(gpointer priv, unsigned buffer_size)
{
	GstElement *src = NULL;
	GstElement *pipeline = priv;

	/* Try to receive pipeline bin name */
	src = gst_bin_get_by_name(GST_BIN(pipeline), "routermanager_src");
	if (src != NULL) {
		/* set blocksize */
		g_object_set(G_OBJECT(src), "blocksize", buffer_size, NULL);
		g_object_unref(src);
	}
}

static void gstreamer_set_buffer_input_size(gpointer priv, unsigned buffer_size)
{
	GstElement *sink = NULL;
	GstElement *pipeline = priv;

	/* Try to receive pipeline bin name */
	sink = gst_bin_get_by_name(GST_BIN(pipeline), "routermanager_sink");
	if (sink != NULL) {
		/* set blocksize */
		g_object_set(G_OBJECT(sink), "blocksize", buffer_size, NULL);
		g_object_unref(sink);
	}
}

/**
 * \brief Initialize audio device
 * \param channels number of channels
 * \param sample_rate sample rate
 * \param bits_per_sample number of bits per samplerate
 * \return TRUE on success, otherwise error
 */
static int gstreamer_init(unsigned char channels, unsigned short sample_rate, unsigned char bits_per_sample)
{
	/* TODO: Check if configuration is valid and usable */
	gstreamer_channels = channels;
	gstreamer_sample_rate = sample_rate;
	gstreamer_bits_per_sample = bits_per_sample;

	return 0;
}

void gstreamer_set_properties(GstElement *element)
{
	GstStructure *props;

	return;
#if 1
	props = gst_structure_new("stream-properties", "media.role", "phone", NULL);

	gst_structure_set(props, "filter.want", G_TYPE_STRING, "echo-cancel", NULL);
	gst_structure_set(props, "media.role", G_TYPE_STRING, "phone", NULL);

	g_object_set(element, "stream-properties", props, NULL);
	gst_structure_free(props);
#else
	gchar *tmp = g_strdup_printf("props,media.role=phone,filter.want=echo-cancel");
	props = gst_structure_from_string(tmp, NULL);
	g_object_set(element, "stream-properties", props, NULL);
	gst_structure_free(props);

	g_free(tmp);
#endif
}

static gboolean gstreamer_start_pipeline(struct pipes *pipes, gchar *command)
{
	GstElement *pipe;
	GstElement *bin;
	GError *error = NULL;

	/* Start pipeline */
	pipe = gst_parse_launch(command, &error);

	if (error != NULL || !pipe) {
		g_debug("Error: Could not launch output pipe. => %s", error->message);
		g_error_free(error);

		return FALSE;
	}

	/* Try to set pipeline state to playing */
	gst_element_set_state(pipe, GST_STATE_PLAYING);

	gstreamer_set_properties(pipe);

	bin = gst_bin_get_by_name(GST_BIN(pipe), "routermanager_src");
	if (bin) {
		pipes->out_pipe = pipe;
		pipes->out_bin = bin;
		gstreamer_set_buffer_output_size(pipe, 160);
	} else {
		bin = gst_bin_get_by_name(GST_BIN(pipe), "routermanager_sink");

		pipes->in_pipe = pipe;
		pipes->in_bin = bin;
		gstreamer_set_buffer_input_size(pipe, 160);
	}

	return TRUE;
}

/**
 * \brief Open audio device
 * \return private data or NULL on error
 */
static void *gstreamer_open(void)
{
	gchar *command = NULL;
	struct pipes *pipes = NULL;
	struct profile *profile = profile_get_active();
	gchar *output;
	gchar *input;

	pipes = g_malloc(sizeof(struct pipes));
	if (pipes == NULL) {
		return NULL;
	}

	output = g_settings_get_string(profile->settings, "audio-output");
	input = g_settings_get_string(profile->settings, "audio-input");
	//if (EMPTY_STRING(output)) {
	output = g_strdup("autoaudiosink");
	//}
	if (EMPTY_STRING(input)) {
		input = g_strdup("autoaudiosrc");
	}

	/* Create command for output pipeline */
	GstElement *source = gst_element_factory_make("appsrc", "routermanager_src");

	gst_app_src_set_stream_type(GST_APP_SRC(source), GST_APP_STREAM_TYPE_STREAM);

	GstPad *sourcepad = gst_element_get_static_pad(source, "src");

	gst_pad_set_caps(sourcepad,
	                 gst_caps_new_simple(
	                     "audio/x-raw-int",
	                     "rate", G_TYPE_INT, gstreamer_sample_rate,
	                     "channels", G_TYPE_INT, gstreamer_channels,
	                     "width", G_TYPE_INT, gstreamer_bits_per_sample,
	                     "depth", G_TYPE_INT, gstreamer_bits_per_sample,
	                     "signed", G_TYPE_BOOLEAN, TRUE,
	                     "endianness", G_TYPE_INT, 1234,
	                     NULL));
	gst_object_unref(sourcepad);

	GstElement *sink = gst_element_factory_make(output, "sink");
	GstElement *pipeline = gst_pipeline_new("routermanager_src_pipe");
	gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
	gst_element_link_many(source, sink, NULL);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	pipes->out_pipe = pipeline;
	pipes->out_bin = gst_bin_get_by_name(GST_BIN(pipeline), "routermanager_src");
	gstreamer_set_buffer_output_size(pipeline, 160);

	/* Create command for input pipeline */
	command = g_strdup_printf("%s ! appsink drop=true max_buffers=2"
	                          " caps=audio/x-raw-int"
	                          ",rate=%d"
	                          ",channels=%d"
	                          ",width=%d"
	                          " name=routermanager_sink",
	                          input,
	                          gstreamer_sample_rate, gstreamer_channels, gstreamer_bits_per_sample);

	gstreamer_start_pipeline(pipes, command);
	g_free(command);

	pipes->adapter = gst_adapter_new();

	return pipes;
}

/**
 * \brief Write audio data
 * \param priv private data
 * \param pnData audio data
 * \param size size of audio data
 * \return bytes written or error code
 */
static gsize gstreamer_write(void *priv, guchar *data, gsize size)
{
	gchar *tmp = NULL;
	GstBuffer *buffer = NULL;
	struct pipes *pipes = priv;
	GstElement *src = pipes->out_bin;
	unsigned int written = -1;

	if (src) {
		tmp = g_malloc0(size);
		memcpy((char *) tmp, (char *) data, size);
		buffer = gst_app_buffer_new(tmp, size, (GstAppBufferFinalizeFunc) g_free, tmp);
		gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
		written = size;
	}
	g_usleep(20 * G_TIME_SPAN_MILLISECOND);

	return written;
}

static gsize gstreamer_read(void *priv, guchar *data, gsize size)
{
	GstBuffer *buffer = NULL;
	struct pipes *pipes = priv;
	GstElement *sink = pipes->in_bin;
	unsigned int read =  0;

	buffer = gst_app_sink_pull_buffer(GST_APP_SINK(sink));
	if (buffer != NULL) {
		gst_adapter_push(pipes->adapter, buffer);
	}

	read = MIN(gst_adapter_available(pipes->adapter), size);
	gst_adapter_copy(pipes->adapter, data, 0, read);
	gst_adapter_flush(pipes->adapter, read);

	g_usleep(20 * G_TIME_SPAN_MILLISECOND);

	return read;
}

/**
 * \brief Stop and remove pipeline
 * \param priv private data
 * \param force force quit
 * \return error code
 */
int gstreamer_close(void *priv, gboolean force)
{
	struct pipes *pipes = priv;

	if (pipes == NULL) {
		return 0;
	}

	GstElement *src = pipes->out_bin;
	if (src != NULL) {
		GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipes->out_pipe));
		gst_bus_add_watch(bus, pipeline_cleaner, pipes->out_pipe);
		gst_app_src_end_of_stream(GST_APP_SRC(src));
		gst_object_unref(bus);
		pipes->out_pipe = NULL;
	}

	if (pipes->in_pipe != NULL) {
		gst_element_set_state(pipes->in_pipe, GST_STATE_NULL);
		gst_object_unref(pipes->in_pipe);
		pipes->out_pipe = NULL;
	}

	free(pipes);
	pipes = NULL;

	return 0;
}

int gstreamer_shutdown(void)
{
	return 0;
}

/** audio definition */
struct audio gstreamer = {
	"GStreamer",
	gstreamer_init,
	gstreamer_open,
	gstreamer_write,
	gstreamer_read,
	gstreamer_close,
	gstreamer_shutdown,
	gstreamer_detect_devices
};

/**
 * \brief Activate plugin (add net event)
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	gst_init_check(NULL, NULL, NULL);

	//g_setenv("PULSE_PROP_media.role", "phone", TRUE);
	//g_setenv("PULSE_PROP_filter.want", "echo-cancel", TRUE);

	routermanager_audio_register(&gstreamer);
}

/**
 * \brief Deactivate plugin (remote net event)
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
}
