plugin_in_files = faxophone.plugin

include ../Makefile.plugins

plugindir = $(libdir)/routermanager/faxophone

plugin_LTLIBRARIES = libfaxophone.la

libfaxophone_la_SOURCES = \
	fax.c\
	fax.h\
	faxophone.c\
	faxophone.h\
	isdn-convert.c\
	isdn-convert.h\
	phone.c\
	phone.h\
	ringtone.c\
	ringtone.h\
	sff.c\
	sff.h

libfaxophone_la_CFLAGS = $(plugin_cflags)
libfaxophone_la_LDFLAGS = $(plugin_ldflags)
libfaxophone_la_LIBADD  = $(plugin_libadd)
