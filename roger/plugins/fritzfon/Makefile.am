plugin_in_files = fritzfon.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/fritzfon

plugin_LTLIBRARIES = libfritzfon.la

libfritzfon_la_SOURCES = \
	fritzfon.c

libfritzfon_la_CFLAGS = $(plugin_cflags) $(EXTRA_CFLAGS) $(SOUP_CFLAGS)
libfritzfon_la_LDFLAGS = $(plugin_ldflags) $(EXTRA_LIBS) $(SOUP_LIBS)
libfritzfon_la_LIBADD  = $(plugin_libadd) $(EXTRA_LIBS) $(SOUP_LIBS)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.fritzfon.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-fritzfon.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
