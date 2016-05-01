plugin_in_files = simple_pwd.plugin

include ../Makefile.plugins

plugindir = $(libdir)/routermanager/simple_pwd

plugin_LTLIBRARIES = libsimple_pwd.la

libsimple_pwd_la_SOURCES = \
	simple_pwd.c

libsimple_pwd_la_CFLAGS = $(plugin_cflags)
libsimple_pwd_la_LDFLAGS = $(plugin_ldflags)
libsimple_pwd_la_LIBADD  = $(plugin_libadd)
