plugin_in_files = sip.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/sip

plugin_LTLIBRARIES = libsip.la

libsip_la_SOURCES = \
	sip.c

libsip_la_CFLAGS = $(plugin_cflags) $(SIP_CFLAGS)
libsip_la_LDFLAGS = $(plugin_ldflags) $(SIP_LIBS)
libsip_la_LIBADD  = $(plugin_libadd) $(SIP_LIBS)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.sip.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-sip.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
