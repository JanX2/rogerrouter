#ifndef REMOTE_H
#define REMOTE_H

#include <gtk/gtk.h>

typedef void (*remote_call_func)(gpointer, gpointer);

struct remote_call_port {
	gint fd[ 2 ];
	GIOChannel *channel_in;
	GIOChannel *channel_out;
	GCond condition;
	GMutex mutex;
	GThread *owner;
	guint gtk_input_tag;
};

struct remote_call {
	remote_call_func func;
	struct remote_call_port *port;
	gpointer context;
	gpointer data;
};

struct remote_call_port *remote_port_get_main(void);
gint remote_port_init_call(struct remote_call_port *port);
gint remote_port_register_call(struct remote_call_port *port);
gint remote_port_invoke_call(struct remote_call_port *port, remote_call_func func, gpointer context, gpointer data);
gint remote_port_init(void);

#endif
