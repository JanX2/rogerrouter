SUBDIRS = . plugins data
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
	action.c\
	action.h\
	address-book.c\
	address-book.h\
	appobject-emit.c\
	appobject-emit.h\
	appobject.c\
	appobject.h\
	audio.c\
	audio.h\
	call.c\
	call.h\
	routermanager.c\
	routermanager.h\
	contact.c\
	contact.h\
	connection.c\
	connection.h\
	csv.c\
	csv.h\
	fax_phone.c \
	fax_phone.h \
	fax_spooler.c\
	fax_server.c\
	fax_printer.h\
	file.c\
	file.h\
	filter.c\
	filter.h\
	ftp.c\
	ftp.h\
	gstring.c\
	gstring.h\
	logging.c\
	logging.h\
	lookup.c\
	lookup.h\
	net_monitor.c\
	net_monitor.h\
	network.c\
	network.h\
	number.h\
	osdep.c\
	osdep.h\
	password.c\
	password.h\
	phone.h\
	plugins.c\
	plugins.h\
	profile.c\
	profile.h\
	router.c\
	router.h\
	ui_ops.h\
	voxplay.c\
	voxplay.h\
	libfaxophone/fax.c \
	libfaxophone/fax.h \
	libfaxophone/faxophone.c \
	libfaxophone/faxophone.h \
	libfaxophone/isdn-convert.c \
	libfaxophone/isdn-convert.h \
	libfaxophone/phone.c \
	libfaxophone/phone.h

if WITH_DBUS
libroutermanager_la_SOURCES += \
	bluetooth.c\
	bluetooth.h
endif

libroutermanager_la_LDFLAGS = -rdynamic
libroutermanager_la_LIBADD = -lc $(EXTRA_LIBS) $(PEAS_LIBS) $(SOUP_LIBS) $(CAPI_LIBS) $(SPANDSP_LIBS) $(DBUS_LIBS) $(OS_LIBS)
libroutermanager_la_CFLAGS = -fno-strict-aliasing $(EXTRA_CFLAGS) $(APP_CFLAGS) $(PEAS_CFLAGS) -Wall $(SOUP_CFLAGS) $(CAPI_CFLAGS) $(SPANDSP_CFLAGS) -Werror $(AM_CFLAGS) $(DBUS_CFLAGS) $(OS_CFLAGS)

if WIN32
libroutermanager_la_LDFLAGS += -no-undefined -avoid-version
endif

pcdir = ${libdir}/pkgconfig
pc_DATA = routermanager.pc

$(OBJECTS): libtool

export SED

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
