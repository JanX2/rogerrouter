plugin_in_files = gstreamer.plugin

include ../Makefile.plugins

plugindir = $(libdir)/routermanager/plugins/gstreamer

plugin_LTLIBRARIES = libgstreamer.la

libgstreamer_la_SOURCES = \
	gstreamer.c

libgstreamer_la_CFLAGS = $(plugin_cflags) $(GSTREAMER0_CFLAGS) -DGST_DISABLE_DEPRECATED
libgstreamer_la_LDFLAGS = $(plugin_ldflags)
libgstreamer_la_LIBADD  = $(plugin_libadd) $(GSTREAMER0_LIBS)
