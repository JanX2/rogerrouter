plugin_in_files = kwallet.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/kwallet

plugin_LTLIBRARIES = libkwallet.la

libkwallet_la_SOURCES = \
	kwallet.cpp \
	kwallet_wrapper.cpp

libkwallet_la_CXXFLAGS = $(plugin_cflags) $(KWALLET_CFLAGS)
libkwallet_la_LDFLAGS = $(plugin_ldflags) $(KWALLET_LIBS)
libkwallet_la_LIBADD  = $(plugin_libadd) $(KWALLET_LIBS)

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-kwallet.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
