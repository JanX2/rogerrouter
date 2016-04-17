plugin_in_files = evolution.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/evolution

plugin_LTLIBRARIES = libevolution.la

libevolution_la_SOURCES = \
	ebook-sources.h \
	evolution.c

libevolution_la_CFLAGS = $(plugin_cflags) $(EBOOK_CFLAGS)
libevolution_la_LDFLAGS = $(plugin_ldflags)  $(EBOOK_FLAGS)
libevolution_la_LIBADD  = $(plugin_libadd) $(EBOOK_LIBS)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.evolution.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-evolution.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
