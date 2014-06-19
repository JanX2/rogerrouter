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

	audio_device = g_slice_new0(struct audio_device);
	audio_device->internal_name = g_strdup("autoaudiosink");
	audio_device->name = g_strdup("Standard");
	audio_device->type = AUDIO_OUTPUT;
	devices = g_slist_prepend(devices, audio_device);

	audio_device = g_slice_new0(struct audio_device);
	audio_device->internal_name = g_strdup("autoaudiosrc");
	audio_device->name = g_strdup("Standard");
	audio_device->type = AUDIO_INPUT;
	devices = g_slist_prepend(devices, audio_device);

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

static gboolean gstreamer_start_pipeline(struct pipes *pipes, gchar *command)
{
	GstElement *pipe;
	GstElement *bin;
	GError *error = NULL;
	GstStateChangeReturn ret;

	/* Start pipeline */
	//g_debug("command: '%s'", command);
	pipe = gst_parse_launch(command, &error);

	if (error != NULL || !pipe) {
		g_warning("Error: Could not launch output pipe. => %s", error->message);
		g_error_free(error);

		return FALSE;
	}

	/* Try to set pipeline state to playing */
	ret = gst_element_set_state(pipe, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_warning("Error: cannot start pipeline => %d", ret);
		return FALSE;
	}

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
	if (EMPTY_STRING(output)) {
		output = g_strdup("autoaudiosink");
	}
	if (EMPTY_STRING(input)) {
		input = g_strdup("autoaudiosrc");
	}

	/* Create command for output pipeline */
	command = g_strdup_printf("appsrc is-live=true name=routermanager_src format=3 block=1 max-bytes=320"
		" ! audio/x-raw,format=S16LE"
		",rate=%d"
		",channels=%d"
		" ! %s",
		gstreamer_sample_rate, gstreamer_channels,
		output);

	gstreamer_start_pipeline(pipes, command);
	g_free(command);

	/* Create command for input pipeline */
	command = g_strdup_printf("%s ! appsink drop=true max_buffers=2"
	                          " caps=audio/x-raw,format=S16LE"
	                          ",rate=%d"
	                          ",channels=%d"
	                          " name=routermanager_sink",
	                          input,
	                          gstreamer_sample_rate, gstreamer_channels);

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
	GstBuffer *buffer = NULL;
	struct pipes *pipes = priv;
	GstElement *src = pipes->out_bin;
	gchar *tmp;

	if (!pipes || !src) {
		return 0;
	}

	tmp = g_malloc0(size);
	memcpy((char *) tmp, (char *) data, size);

	buffer = gst_buffer_new_wrapped(tmp, size);
	gst_app_src_push_buffer(GST_APP_SRC(src), buffer);

	return size;
}

static gsize gstreamer_read(void *priv, guchar *data, gsize size)
{
	GstSample *sample = NULL;
	struct pipes *pipes = priv;
	GstElement *sink = pipes->in_bin;
	unsigned int read =  0;

	sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
	if (sample != NULL) {
		gst_adapter_push(pipes->adapter, gst_sample_get_buffer(sample));
	}

	read = MIN(gst_adapter_available(pipes->adapter), size);
	gst_adapter_copy(pipes->adapter, data, 0, read);
	gst_adapter_flush(pipes->adapter, read);

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
	gst_init(NULL, NULL);

	g_setenv("PULSE_PROP_media.role", "phone", TRUE);
	g_setenv("PULSE_PROP_filter.want", "echo-cancel", TRUE);

	routermanager_audio_register(&gstreamer);
}

/**
 * \brief Deactivate plugin (remote net event)
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
}
