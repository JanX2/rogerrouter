SUBDIRS = . plugins data images

AM_CFLAGS = -I$(top_srcdir)

sharedir = $(datadir)/roger
share_DATA = \
	images/address-book.svg \
	images/app.png \
	images/roger-call-blocked.svg \
	images/roger-call-blocked-symbolic.svg \
	images/roger-call-in.svg \
	images/roger-call-in-symbolic.svg \
	images/roger-call-missed.svg \
	images/roger-call-missed-symbolic.svg \
	images/roger-call-out.svg \
	images/roger-call-out-symbolic.svg \
	images/roger-call-voice.svg \
	images/roger-call-voice-symbolic.svg \
	images/roger-fax.svg \
	images/roger-fax-symbolic.svg \
	images/roger-fax-report.svg \
	images/roger-fax-report-symbolic.svg \
	images/roger-record.svg \
	images/roger-record-symbolic.svg

EXTRA_DIST = $(share_DATA) roger_winres.rc roger.gresource.xml assistant.glade authenticate.glade answeringmachine.glade contacts.glade settings.glade shortcuts.glade
DISTCLEANFILES = \
	roger_gresource.c \
	roger_gresource.h

bin_PROGRAMS = roger

roger_CFLAGS = $(EXTRA_CFLAGS) $(GTK_CFLAGS) $(GMODULE_CFLAGS) -Wall -O2 -g $(PEAS_GTK_CFLAGS) $(SNDFILE_CFLAGS) $(AM_CFLAGS) $(OS_CFLAGS) $(TIFF_CFLAGS) $(SOUP_CFLAGS)
roger_LDFLAGS = $(EXTRA_LIBS) $(GTK_LIBS) $(GMODULE_LIBS) -export-dynamic $(top_builddir)/libroutermanager/libroutermanager.la $(PEAS_GTK_LIBS) $(SNDFILE_LIBS) $(SPANDSP_LIBS) $(OS_LIBS) $(TIFF_LIBS) $(SOUP_LIBS)

GLIB_COMPILE_RESOURCES = $(shell $(PKG_CONFIG) --variable=glib_compile_resources gio-2.0)

roger_gresource.c: roger_gresource.h roger.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies roger.gresource.xml)
	$(GLIB_COMPILE_RESOURCES) roger.gresource.xml --target=$@ --sourcedir=. --generate-source

roger_gresource.h: roger.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies roger.gresource.xml)
	$(GLIB_COMPILE_RESOURCES) roger.gresource.xml --target=$@ --sourcedir=. --generate-header

roger_core_SOURCES =\
	about.c\
	about.h\
	answeringmachine.c\
	answeringmachine.h\
	application.c\
	application.h\
	assistant.c\
	assistant.h\
	contacts.c\
	contacts.h\
	crash.c\
	crash.h\
	fax.c\
	fax.h\
	icons.c\
	icons.h\
	journal.c\
	journal.h\
	main.h\
	main_ui.c \
	phone.c\
	phone.h\
	print.c \
	print.h \
	settings.c \
	settings.h \
	shortcuts.c \
	shortcuts.h \
	uitools.c \
	uitools.h \
	roger_gresource.c \
	roger_gresource.h

roger_SOURCES = $(roger_core_SOURCES)

if WIN32
roger_SOURCES +=\
	winmain.c

roger_LDFLAGS += roger_winres.res

BUILT_SOURCES = roger_winres.res

roger_winres.res: roger_winres.rc
	$(WINDRES) -i $< -o $@ -O coff

roger_coredir=$(bindir)
roger_core_LTLIBRARIES = libroger_core.la
libroger_core_la_SOURCES = $(roger_core_SOURCES)
libroger_core_la_CFLAGS = $(roger_CFLAGS)
libroger_core_la_LDFLAGS = $(roger_LDFLAGS) -shared -Wl,--output-def,roger_core.def,--out-implib,roger_core.dll.a -o roger_core.dll -no-undefined -avoid-version
endif

if MACOSX
roger_coredir=$(bindir)
roger_core_LTLIBRARIES = libroger_core.la
libroger_core_la_SOURCES = $(roger_core_SOURCES)
libroger_core_la_CFLAGS = $(roger_CFLAGS)
libroger_core_la_LDFLAGS = $(roger_LDFLAGS) -shared -no-undefined -avoid-version
endif
