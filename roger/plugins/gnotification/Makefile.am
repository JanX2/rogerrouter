plugin_in_files = gnotification.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/gnotification

plugin_LTLIBRARIES = libgnotification.la

libgnotification_la_SOURCES = \
	gnotification.c

libgnotification_la_CFLAGS = $(plugin_cflags)
libgnotification_la_LDFLAGS = $(plugin_ldflags)
libgnotification_la_LIBADD  = $(plugin_libadd)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.gnotify.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-gnotify.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
