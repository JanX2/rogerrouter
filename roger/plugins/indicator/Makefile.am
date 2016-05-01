plugin_in_files = indicator.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/indicator

plugin_LTLIBRARIES = libindicator.la

libindicator_la_SOURCES = \
	indicator.c

libindicator_la_CFLAGS = $(plugin_cflags) $(APPINDICATOR_CFLAGS) $(EXTRA_CFLAGS)
libindicator_la_LDFLAGS = $(plugin_ldflags)
libindicator_la_LIBADD  = $(plugin_libadd) $(APPINDICATOR_LIBS)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.indicator.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-indicator.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
