plugin_in_files = statusicon.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/statusicon

plugin_LTLIBRARIES = libstatusicon.la

libstatusicon_la_SOURCES = \
	gtk.c

libstatusicon_la_CFLAGS = $(plugin_cflags)
libstatusicon_la_LDFLAGS = $(plugin_ldflags)
libstatusicon_la_LIBADD  = $(plugin_libadd)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.statusicon.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-statusicon.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
