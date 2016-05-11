plugin_in_files = webjournal.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/webjournal

plugin_LTLIBRARIES = libwebjournal.la

sharedir = $(libdir)/roger/webjournal
share_DATA  = share/header.html
share_DATA  += share/entry.html
share_DATA  += share/footer.html
share_DATA  += share/dragtable.js
share_DATA  += share/sortable.js
share_DATA  += share/styling.css

EXTRA_DIST += $(share_DATA)

libwebjournal_la_SOURCES = \
	webjournal.c \
	webjournal.h

libwebjournal_la_CFLAGS = $(plugin_cflags) -DPLUGIN_DIR=\"$(plugindir)\" $(SOUP_CFLAGS)
libwebjournal_la_LDFLAGS = $(plugin_ldflags)
libwebjournal_la_LIBADD  = $(plugin_libadd) $(SOUP_LIBADD)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.webjournal.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-webjournal.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
