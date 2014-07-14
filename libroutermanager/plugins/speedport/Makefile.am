plugin_in_files = speedport.plugin

include ../Makefile.plugins

plugindir = $(libdir)/routermanager/plugins/speedport

plugin_LTLIBRARIES = libspeedport.la

libspeedport_la_SOURCES = \
	speedport.c \
	speedport.h

libspeedport_la_CFLAGS = $(plugin_cflags) $(SOUP_CFLAGS)
libspeedport_la_LDFLAGS = $(plugin_ldflags)
libspeedport_la_LIBADD  = $(plugin_libadd) $(SOUP_LIBS)
