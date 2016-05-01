plugin_in_files = vcard.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/vcard

plugin_LTLIBRARIES = libvcard.la

libvcard_la_SOURCES = \
	vcard.c \
	vcard.h

libvcard_la_CFLAGS = $(plugin_cflags) $(EXTRA_CFLAGS)
libvcard_la_LDFLAGS = $(plugin_ldflags) $(EXTRA_LIBS)
libvcard_la_LIBADD  = $(plugin_libadd) $(EXTRA_LIBS)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.vcard.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-vcard.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
