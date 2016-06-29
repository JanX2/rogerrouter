plugin_in_files = capi.plugin

include ../Makefile.plugins

plugindir = $(libdir)/routermanager/capi

plugin_LTLIBRARIES = librmcapi.la

librmcapi_la_SOURCES = \
	fax.c\
	fax.h\
	capi.c\
	capi.h\
	isdn-convert.c\
	isdn-convert.h\
	phone.c\
	phone.h\
	ringtone.c\
	ringtone.h\
	sff.c\
	sff.h

librmcapi_la_CFLAGS = $(plugin_cflags)
librmcapi_la_LDFLAGS = $(plugin_ldflags)
librmcapi_la_LIBADD  = $(plugin_libadd) -lcapi20
