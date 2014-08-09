plugin_in_files = fritzbox.plugin

include ../Makefile.plugins

plugindir = $(libdir)/routermanager/plugins/fritzbox

plugin_LTLIBRARIES = libfritzbox.la

libfritzbox_la_SOURCES = \
	fritzbox.c \
	fritzbox.h \
	firmware-common.c \
	firmware-common.h \
	firmware-plain.c \
	firmware-plain.h \
	firmware-04-74.c \
	firmware-04-74.h \
	firmware-05-50.c \
	firmware-05-50.h \
	csv.c \
	csv.h

libfritzbox_la_CFLAGS = $(plugin_cflags) $(SOUP_CFLAGS)
libfritzbox_la_LDFLAGS = $(plugin_ldflags)
libfritzbox_la_LIBADD  = $(plugin_libadd) $(SOUP_LIBS)
