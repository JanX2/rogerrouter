plugin_in_files = notification.plugin

include ../Makefile.plugins

plugindir = $(libdir)/roger/notification

plugin_LTLIBRARIES = libnotification.la

libnotification_la_SOURCES = \
	notification.c

libnotification_la_CFLAGS = $(plugin_cflags) $(LIBNOTIFY_CFLAGS)
libnotification_la_LDFLAGS = $(plugin_ldflags)
libnotification_la_LIBADD  = $(plugin_libadd) $(LIBNOTIFY_LIBS)

#This has to be defined, because GSETTINGS_CHECK_RULE adds to it
MOSTLYCLEANFILES =

@INTLTOOL_SCHEMAS_RULE@

gsettings_SCHEMAS = org.tabos.roger.plugins.notify.gschema.xml
@GSETTINGS_RULES@

appstreamdir = $(datadir)/appdata
appstream_in_files = roger-plugins-notification.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(gsettings_SCHEMAS) $(appstream_in_files)

DISTCLEANFILES += $(appstream_DATA)
