#include <glib.h>

#ifdef G_OS_WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <remote.h>

/** Main remote call port */
static struct remote_call_port *main_port;

/**
 * \brief Process remote call request
 * \param port remote call port
 * \param event remote call event
 * \return 0
 */
static int remote_call_process(struct remote_call_port *port, struct remote_call *event) {
	event->func(event->context, event->data);

	/* Wake up the thread which singalled main */
	g_mutex_lock(&port->mutex);
	g_cond_signal(&port->condition);
	g_mutex_unlock(&port->mutex);

	return 0;
}

/**
 * \brief Handle remote call port
 * \param channel io channel
 * \param condition io condition
 * \param data user data pointer to remote call port structure
 * \\eturn TRUE
 */
gboolean remote_call_handle(GIOChannel *channel, GIOCondition condition, gpointer data) {
	struct remote_call_port *port = data;
	struct remote_call event;
	GIOStatus status;
	GError *error = NULL;
	gsize read;

	/* Read one remote call item from pipe */
	status = g_io_channel_read_chars(port->channel_in, (gchar *) &event, sizeof(struct remote_call), &read, &error);
	if (status == G_IO_STATUS_NORMAL && read == sizeof(event)) {
		remote_call_process(port, &event);
	} else {
		g_warning("Warning, status = %d, read = %" G_GSIZE_FORMAT, status, read);
	}

	return TRUE;
}

/**
 * \brief Initialize remote call port
 * \param port remote call port
 * \return error code (0 on success, otherwise error)
 */
int remote_port_init_call(struct remote_call_port *port) {
	//if (!g_thread_supported()) {
	//	g_thread_init(NULL);
	//}

#ifdef G_OS_WIN32
	if (_pipe(port->fd, 2, O_BINARY) == -1) {
#else
	if (pipe(port->fd) < 0) {
#endif
		g_debug("Failed to create pipe");
		return -1;
	}

#ifdef G_OS_WIN32
	port->channel_in = g_io_channel_win32_new_fd(port->fd[0]);
	port->channel_out = g_io_channel_win32_new_fd(port->fd[1]);
#else
	port->channel_in = g_io_channel_unix_new(port->fd[0]);
	port->channel_out = g_io_channel_unix_new(port->fd[1]);
#endif
	g_io_channel_set_encoding(port->channel_in, NULL, NULL);
	g_io_channel_set_encoding(port->channel_out, NULL, NULL);

	g_cond_init(&port->condition);
	g_mutex_init(&port->mutex);
	port->gtk_input_tag = 0;

	port->owner = g_thread_self();

	return 0;
}

/**
 * \brief Register a new remote call port
 * \param port remote call port
 * \return error code (0 on success, otherwise error)
 */
int remote_port_register_call(struct remote_call_port *port) {
	if (port->gtk_input_tag != 0) {
		return 0;
	}

	port->gtk_input_tag = g_io_add_watch_full(port->channel_in, G_PRIORITY_HIGH, G_IO_IN, remote_call_handle, port, NULL);
	if (port->gtk_input_tag != 0) {
		return 0;
	}

	g_warning("Could not register remote call port");
	return -1;
}

/**
 * \brief Remote call invoke
 * \param port remote call port
 * \param func remote call function
 * \param context context pointer
 * \param data user data
 * \return error code (0 on success, otherwise error)
 */
int remote_port_invoke_call(struct remote_call_port *port, remote_call_func func, gpointer context, gpointer data) {
	struct remote_call event;
	gsize written;
	GError *error = NULL;
	GIOStatus status;
	gchar *buf;

	if (port == NULL) {
		g_warning("port == NULL");
		return -1;
	}

	if (g_thread_self() == port->owner) {
		func(context, data);
		return 0;
	}

	event.func = func;
	event.port = port;
	event.context = context;
	event.data = data;

	buf = (gchar *) &event;

	status = g_io_channel_write_chars(port->channel_out, buf, sizeof(event), &written, &error);
	if (error) {
		g_debug("Error: %s", error->message);
		return -1;
	}

	if (status == G_IO_STATUS_NORMAL) {
		gint64 end_time = g_get_monotonic_time () + 1 * G_TIME_SPAN_SECOND;

		g_mutex_lock(&port->mutex);
		g_io_channel_flush(port->channel_out, NULL);
		if (!g_cond_wait_until(&port->condition, &port->mutex, end_time)) {
			g_mutex_unlock(&port->mutex);

			return -1;
		}
		g_mutex_unlock(&port->mutex);

		return 0;
	}

	g_debug("Failed...");

	return -1;
}

/**
 * \brief Get main remote port
 * \return main port
 */
struct remote_call_port *remote_port_get_main(void) {
	return main_port;
}

/**
 * \brief Initialize remote port
 * \return 0
 */
int remote_port_init(void) {
	main_port = g_malloc0(sizeof(struct remote_call_port));

	remote_port_init_call(main_port);
	remote_port_register_call(main_port);

	return 0;
}
