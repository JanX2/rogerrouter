plugin_in_files = thunderbird.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/thunderbird

plugin_LTLIBRARIES = libthunderbird.la

libthunderbird_la_SOURCES = \
	thunderbird.c

libthunderbird_la_CFLAGS = $(plugin_cflags) $(EXTRA_CFLAGS) $(SOUP_CFLAGS)
libthunderbird_la_LDFLAGS = $(plugin_ldflags) $(EXTRA_LIBS) $(SOUP_LIBS)
libthunderbird_la_LIBADD  = $(plugin_libadd) $(EXTRA_LIBS) $(SOUP_LIBS)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.thunderbird.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-thunderbird.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
