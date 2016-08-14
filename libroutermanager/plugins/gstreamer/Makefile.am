plugin_in_files = gstreamer.plugin

include ../Makefile.plugins

plugindir = $(libdir)/routermanager/gstreamer

plugin_LTLIBRARIES = libgstreamer.la

libgstreamer_la_SOURCES = \
	gstreamer.c

libgstreamer_la_CFLAGS = $(plugin_cflags) $(GSTREAMER1_CFLAGS) -DGST_DISABLE_DEPRECATED
libgstreamer_la_LDFLAGS = $(plugin_ldflags)
libgstreamer_la_LIBADD  = $(plugin_libadd) $(GSTREAMER1_LIBS)
