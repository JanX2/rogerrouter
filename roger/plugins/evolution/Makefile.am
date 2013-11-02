SUBDIRS = data

plugin_in_files = evolution.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/plugins/evolution

plugin_LTLIBRARIES = libevolution.la

libevolution_la_SOURCES = \
	ebook-sources.h \
	evolution.c

if HAVE_EBOOK_SOURCE_REGISTRY
libevolution_la_SOURCES += sources-registry.c
else
libevolution_la_SOURCES += sources-list.c
endif

libevolution_la_CFLAGS = $(plugin_cflags) $(EBOOK_CFLAGS)
libevolution_la_LDFLAGS = $(plugin_ldflags)  $(EBOOK_FLAGS)
libevolution_la_LIBADD  = $(plugin_libadd) $(EBOOK_LIBS)
