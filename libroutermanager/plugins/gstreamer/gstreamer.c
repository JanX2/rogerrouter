/**
 * The libroutermanager project
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

/** This flag informs about gstreamer initialization status */
static gboolean audio_init = FALSE;
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
	{1, "alsasink", "alsasinkpresencetest"},
	{1, "osxaudiosink", "osxaudioinkpresencetest"},
	{2, "pulsesrc", "pulsesrcpresencetest"},
	{2, "alsasrc", "alsasrcpresencetest"},
	{2, "osxaudiosrc", "osxaudiosrcpresencetest"},
	{0, NULL, NULL}
};

struct pipes {
	GstElement *in_pipe;
	GstElement *out_pipe;
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
	GError *error = NULL;
	GSList *devices = NULL;
	struct audio_device *audio_device;
	int index = 0;

	/* initialize gstreamer */
	audio_init = gst_init_check(NULL, NULL, &error);
	if (!audio_init) {
		g_warning("GStreamer failed to initialized (%s)", error ? error->message : "");
		if (error != NULL) {
			g_error_free(error);
		}

		return devices;
	}

	for (index = 0; device_test[index].type != 0; index++) {
		GstElement *element = NULL;

		/* check if we have a sink within gstreamer */
		element = gst_element_factory_make(device_test[index].pipe, device_test[index].test);
		if (!element) {
			continue;
		}

		/* Set playback mode to pause */
		gst_element_set_state(element, GST_STATE_PAUSED);

		g_debug("long name: %s", gst_element_get_name(element));

#if 0
		GstPropertyProbe *probe = NULL;
		const GParamSpec *spec = NULL;
		GValueArray *array = NULL;
		probe = GST_PROPERTY_PROBE(element);
		spec = gst_property_probe_get_property(probe, "index");

		/* now try to receive the index name and add it to the internal list */
		array = gst_property_probe_probe_and_get_values(probe, spec);
		if (array != NULL) {
			for (index = 0; index < array->n_values; index++) {
				GValue *device_val = NULL;
				gchar *name = NULL;

				device_val = g_value_array_get_nth(array, index);
				g_object_set_property(G_OBJECT(element), "index", device_val);
				if (device_test[index].type == 1) {
					g_object_get(G_OBJECT(element), "index-name", &name, NULL);
				}
				if (name == NULL) {
					g_object_get(G_OBJECT(element), "index", &name, NULL);
				}
				if (name != NULL) {
					audio_device = g_slice_new0(struct audio_device);
					audio_device->internal_name = g_strdup_printf("%s index=%s", device_test[index].pipe, g_value_get_string(device_val));
					if (index == 0) {
						gchar *spec = strrchr(name, '.');
						if (spec != NULL) {
							audio_device->name = g_strdup_printf("Pulseaudio (%s)", spec + 1);
						} else {
							audio_device->name = g_strdup("Pulseaudio");
						}
					} else {
						audio_device->name = g_strdup(name);
					}

					audio_device->type = device_test[index].type == 1 ? AUDIO_OUTPUT : AUDIO_INPUT;
					devices = g_slist_prepend(devices, audio_device);
						g_free(name);
				}
			}
			g_value_array_free(array);
		}
#endif
		gst_element_set_state(element, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(element));
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

/**
 * \brief Open audio device
 * \return private data or NULL on error
 */
static void *gstreamer_open(void)
{
	gchar *command = NULL;
	GError *error = NULL;
	GstState current;
	struct pipes *pipes = NULL;

	g_debug("started");
	/*if (gstreamerGetSelectedOutputDevice(profile_get_active()) == NULL) {
		return NULL;
	}

	if (gstreamerGetSelectedInputDevice(profile_get_active()) == NULL) {
		return NULL;
	}*/

	pipes = g_malloc(sizeof(struct pipes));
	if (pipes == NULL) {
		return NULL;
	}

	/* Create command for output pipeline */
	command = g_strdup_printf("appsrc is-live=true name=routermanager_src"
		" ! audio/x-raw-int"
		",rate=%d"
		",channels=%d"
		",width=%d"
		",depth=%d"
		",signed=true,endianness=1234"
		" ! %s",
		gstreamer_sample_rate, gstreamer_channels, gstreamer_bits_per_sample, gstreamer_bits_per_sample,
		/*gstreamerGetSelectedOutputDevice(profile_get_active())*/"pulsesrc");
	g_debug("command %s", command);
	
	/* Start pipeline */
	pipes->out_pipe = gst_parse_launch(command, &error);
	g_free(command);
	if (error == NULL) {
		/* Try to set pipeline state to playing */
		gst_element_set_state(pipes->out_pipe, GST_STATE_PLAYING);

		/* Now read the current state and verify that we have a good pipeline */
		gst_element_get_state(pipes->out_pipe, &current, NULL, GST_SECOND);

		if (!(current == GST_STATE_PLAYING || current == GST_STATE_PAUSED)) {
			gst_element_set_state(pipes->out_pipe, GST_STATE_NULL);
			gst_object_unref(GST_OBJECT(pipes->out_pipe));
			pipes->out_pipe = NULL;
			g_free(pipes);
			pipes = NULL;
			g_debug("Error: Could not start output pipe (%d)", current);
			return NULL;
		} else {
			gstreamer_set_buffer_output_size(pipes->out_pipe, 160);

			//g_debug("Ok");
		}
	} else {
		g_error_free(error);
		pipes->out_pipe = NULL;

		g_free(pipes);
		pipes = NULL;
		g_debug("Error: Could not launch output pipe");

		return NULL;
	}

	/* Create command for input pipeline */
	command = g_strdup_printf("%s ! appsink drop=true max_buffers=1"
		" caps=audio/x-raw-int"
		",rate=%d"
		",channels=%d"
		",width=%d"
		" name=routermanager_sink",
		/*gstreamerGetSelectedInputDevice(profile_get_active())*/"pulsesink",
		gstreamer_sample_rate, gstreamer_channels, gstreamer_bits_per_sample);
	g_debug("command %s", command);
	
	/* Start pipeline */
	pipes->in_pipe = gst_parse_launch(command, &error);
	if (error == NULL) {
		/* Try to set pipeline state to playing */
		gst_element_set_state(pipes->in_pipe, GST_STATE_PLAYING);

		/* Now read the current state and verify that we have a good pipeline */
		gst_element_get_state(pipes->in_pipe, &current, NULL, GST_SECOND);

		if (current != GST_STATE_PLAYING) {
			gst_element_set_state(pipes->in_pipe, GST_STATE_NULL);
			gst_object_unref(GST_OBJECT(pipes->in_pipe));
			pipes->in_pipe = NULL;
			/* TODO: free output pipe */
			g_free(pipes);
			pipes = NULL;
			g_debug("Error: Could not start input pipe");
		} else {
			gstreamer_set_buffer_input_size(pipes->in_pipe, 160);

			//g_debug("Ok");
		}
	} else {
		g_error_free(error);
		pipes->in_pipe = NULL;

		g_free(pipes);
		pipes = NULL;
		g_debug("Error: Could not launch input pipe");
	}

	g_free(command);

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
#if 0
	gchar *tmp = NULL;
	GstBuffer *buffer = NULL;
	GstElement *src = NULL;
	struct pipes *pipes = priv;
	unsigned int written = -1;

	if (pipes == NULL) {
		return 0;
	}

	g_return_val_if_fail(GST_IS_BIN(pipes->out_pipe), -2);

	src = gst_bin_get_by_name(GST_BIN(pipes->out_pipe), "routermanager_src");
	if (src != NULL) {
		tmp = (gchar *) g_malloc0(size);
		memcpy((char *) tmp, (char *) data, size);
		buffer = gst_app_buffer_new(tmp, size, (GstAppBufferFinalizeFunc) g_free, tmp);
		gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
		written = size;
		g_object_unref(src);
	}

	return written;
#endif
	return 0;
}

static gsize gstreamer_read(void *priv, guchar *data, gsize size)
{
#if 0
	GstBuffer *buffer = NULL;
	GstElement *sink = NULL;
	struct pipes *pipes = priv;
	unsigned int read =  0;

	if (pipes == NULL) {
		return 0;
	}

	g_return_val_if_fail(GST_IS_BIN(pipes->in_pipe), -2);

	sink = gst_bin_get_by_name(GST_BIN(pipes->in_pipe), "routermanager_sink");
	if (sink != NULL) {
		buffer = gst_app_sink_pull_buffer(GST_APP_SINK(sink));
		if (buffer != NULL) {
			read = MIN(GST_BUFFER_SIZE(buffer), size);
			memcpy(data, GST_BUFFER_DATA(buffer), read);
			gst_buffer_unref(buffer);
		}
		g_object_unref(sink);
	}

	return read;
#endif
	return 0;
}

/**
 * \brief Stop and remove pipeline
 * \param priv private data
 * \param force force quit
 * \return error code
 */
int gstreamer_close(void *priv, gboolean force) {
	struct pipes *pipes = priv;

	if (pipes == NULL) {
		return 0;
	}

	if (pipes->out_pipe != NULL) {
		GstElement *src = gst_bin_get_by_name(GST_BIN(pipes->out_pipe), "routermanager_src");
		if (src != NULL) {
			GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipes->out_pipe));
			gst_bus_add_watch(bus, pipeline_cleaner, pipes->out_pipe);
			gst_app_src_end_of_stream(GST_APP_SRC(src));
			gst_object_unref(bus);
			pipes->out_pipe = NULL;
		}
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

int gstreamer_shutdown(void) {
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
	//RouterManagerGStreamerPlugin *gstreamer_plugin = ROUTERMANAGER_GSTREAMER_PLUGIN(plugin);

	routermanager_audio_register(&gstreamer);
}

/**
 * \brief Deactivate plugin (remote net event)
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	//RouterManagerGStreamerPlugin *gstreamer_plugin = ROUTERMANAGER_GSTREAMER_PLUGIN(plugin);
}
