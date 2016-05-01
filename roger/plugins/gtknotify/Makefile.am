plugin_in_files = gtknotify.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/gtknotify

plugin_LTLIBRARIES = libgtknotify.la

libgtknotify_la_SOURCES = \
	gtknotify.c

libgtknotify_la_CFLAGS = $(plugin_cflags) $(EXTRA_CFLAGS)
libgtknotify_la_LDFLAGS = $(plugin_ldflags) $(EXTRA_LIBS)
libgtknotify_la_LIBADD  = $(plugin_libadd) $(EXTRA_LIBS)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.gtknotify.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-gtknotify.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
