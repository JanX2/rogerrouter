plugin_in_files = reverselookup.plugin

include ../Makefile.plugins

plugindir = $(libdir)/routermanager/plugins/reverselookup

plugin_LTLIBRARIES = libreverselookup.la

libreverselookup_la_SOURCES = \
	reverselookup.c

libreverselookup_la_CFLAGS = $(plugin_cflags) -Wall -Werror -DPLUGIN_DIR=\"$(plugindir)\" $(SOUP_CFLAGS)
libreverselookup_la_LDFLAGS = $(plugin_ldflags)
libreverselookup_la_LIBADD  = $(plugin_libadd) $(SOUP_LIBADD)
