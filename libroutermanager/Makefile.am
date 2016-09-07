SUBDIRS = . plugins data tests
AUTOMAKE_OPTIONS = no-dependencies foreign
CLEANFILES              = *~ comperr
DISTCLEANFILES			= intltool-extract intltool-merge intltool-update
MAINTAINERCLEANFILES    = configure aclocal.m4 Makefile.in config.h.in
			  stamp-h.in
lib_LTLIBRARIES = libroutermanager.la

AM_CFLAGS = -I$(top_srcdir)

libroutermanager_includedir = $(includedir)/libroutermanager/
libroutermanager_include_HEADERS = $(wildcard *.h)

libroutermanager_la_SOURCES = \
	rmaction.c\
	rmaction.h\
	rmaddressbook.c\
	rmaddressbook.h\
	rmaudio.c\
	rmaudio.h\
	rmcall.c\
	rmcall.h\
	rmdevice.c\
	rmdevice.h\
	rmcontact.c\
	rmcontact.h\
	rmconnection.c\
	rmconnection.h\
	rmcsv.c\
	rmcsv.h\
	rmfaxspooler.c\
	rmfaxserver.c\
	rmfaxprinter.h\
	rmfile.c\
	rmfile.h\
	rmfilter.c\
	rmfilter.h\
	rmftp.c\
	rmftp.h\
	rmstring.c\
	rmstring.h\
	rmlog.c\
	rmlog.h\
	rmlookup.c\
	rmlookup.h\
	rmnetmonitor.c\
	rmnetmonitor.h\
	rmnetwork.c\
	rmnetwork.h\
	rmnotification.c\
	rmnotification.h\
	rmmain.c\
	rmmain.h\
	rmobjectemit.c\
	rmobjectemit.h\
	rmobject.c\
	rmobject.h\
	rmosdep.c\
	rmosdep.h\
	rmpassword.c\
	rmpassword.h\
	rmplugins.c\
	rmplugins.h\
	rmprofile.c\
	rmprofile.h\
	rmphone.c\
	rmphone.h\
	router.c\
	router.h\
	rmsettings.c\
	rmsettings.h\
	rmssdp.c\
	rmssdp.h\
	rmvox.c\
	rmvox.h\
	xml.c\
	xml.h

libroutermanager_la_LDFLAGS = -rdynamic
libroutermanager_la_LIBADD = -lc $(EXTRA_LIBS) $(PEAS_LIBS) $(SOUP_LIBS) $(CAPI_LIBS) $(SPANDSP_LIBS) $(DBUS_LIBS) $(OS_LIBS)
libroutermanager_la_CFLAGS = -I. -fno-strict-aliasing -I.. $(EXTRA_CFLAGS) $(APP_CFLAGS) $(PEAS_CFLAGS) -Wall $(SOUP_CFLAGS) $(CAPI_CFLAGS) $(SPANDSP_CFLAGS) $(AM_CFLAGS) $(DBUS_CFLAGS) $(OS_CFLAGS) $(GTK_CFLAGS)

if WIN32
libroutermanager_la_LDFLAGS += -no-undefined -avoid-version
endif

pcdir = ${libdir}/pkgconfig
pc_DATA = routermanager.pc

$(OBJECTS): libtool

export SED

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

#if HAVE_INTROSPECTION
#-include $(INTROSPECTION_MAKEFILE)
#INTROSPECTION_GIRS =
#INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all --identifier-prefix=rm
#--accept-unprefixed
#INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

#router_manager-1.0.gir: libroutermanager.la
#router_manager_1_0_gir_NAMESPACE = router_manager
#router_manager_1_0_gir_VERSION = 1.8.2
#router_manager_1_0_gir_INCLUDES = GObject-2.0
#router_manager_1_0_gir_CFLAGS = $(libroutermanager_la_CFLAGS)
#router_manager_1_0_gir_LIBS = libroutermanager.la
#router_manager_1_0_gir_FILES = $(libroutermanager_la_SOURCES)
#INTROSPECTION_GIRS += router_manager-1.0.gir

#girdir = $(datadir)/gir-1.0
#gir_DATA = $(INTROSPECTION_GIRS)

#typelibdir = $(libdir)/girepository-1.0
#typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

#CLEANFILES += $(gir_DATA) $(typelib_DATA)
#endif
