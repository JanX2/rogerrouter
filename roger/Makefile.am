SUBDIRS = . plugins data

AM_CFLAGS = -I$(top_srcdir)

sharedir = $(datadir)/roger
share_DATA = \
	images/app.png \
	images/callin.png \
	images/callmissed.png \
	images/callout.png

EXTRA_DIST = $(share_DATA) images/hicolor_apps_scalable_roger.svg roger_winres.rc

bin_PROGRAMS = roger

roger_CFLAGS = $(EXTRA_CFLAGS) $(GTK_CFLAGS) $(GMODULE_CFLAGS) -Wall -O2 -g $(PEAS_GTK_CFLAGS) $(SNDFILE_CFLAGS) $(AM_CFLAGS) $(OS_CFLAGS)
roger_LDFLAGS = $(EXTRA_LIBS) $(GTK_LIBS) $(GMODULE_LIBS) -export-dynamic $(top_builddir)/libroutermanager/libroutermanager.la $(PEAS_GTK_LIBS) $(SNDFILE_LIBS) $(SPANDSP_LIBS) $(OS_LIBS)

roger_core_SOURCES =\
	about.c\
	about.h\
	application.c\
	application.h\
	assistant.c\
	assistant.h\
	contacts.c\
	contacts.h\
	contact_edit.c\
	contact_edit.h\
	fax.c\
	fax.h\
	icons.c\
	icons.h\
	journal.c\
	journal.h\
	llevel.c\
	llevel.h\
	main.h\
	main_ui.c \
	phone.c\
	phone.h\
	pref.c\
	pref.h\
	pref_action.c\
	pref_action.h\
	pref_audio.c\
	pref_audio.h\
	pref_fax.c \
	pref_fax.h \
	pref_filters.c\
	pref_filters.h\
	pref_plugins.c\
	pref_plugins.h\
	pref_prefix.c\
	pref_prefix.h\
	pref_router.c\
	pref_router.h\
	pref_softphone.c\
	pref_softphone.h\
	pref_misc.c\
	pref_misc.h\
	print.c \
	print.h \
	uitools.c \
	uitools.h

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

NULL =

public_icon_themes = \
	hicolor \
	$(NULL)

private_icon_themes = \
	$(NULL)

public_icons = \
	hicolor_apps_scalable_roger.svg \
	$(NULL)

private_icons = \
	$(NULL)

gtk_update_icon_cache = gtk-update-icon-cache -f -t

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating GTK icon cache."; \
		for theme in $(public_icon_themes); do \
			$(gtk_update_icon_cache) $(DESTDIR)$(datadir)/icons/$$theme; \
		done; \
		for theme in $(private_icon_themes); do \
			$(gtk_update_icon_cache) $(DESTDIR)$(pkgdatadir)/icons/$$theme; \
		done; \
	else \
		echo "*** Icon cache not updated. After (un)install, run this:"; \
		for theme in $(public_icon_themes); do \
			echo "*** $(gtk_update_icon_cache) $(DESTDIR)$(datadir)/icons/$$theme"; \
		done; \
		for theme in $(private_icon_themes); do \
			echo "*** $(gtk_update_icon_cache) $(DESTDIR)$(pkgdatadir)/icons/$$theme"; \
		done; \
	fi

install-icons:
	@-for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d _ -f1`; \
		CONTEXT=`echo $$icon | cut -d _ -f2`; \
		SIZE=`echo $$icon | cut -d _ -f3`; \
		ICONFILE=`echo $$icon | cut -d _ -f4`; \
		mkdir -p $(DESTDIR)/$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/images/$$icon $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
		THEME=`echo $$icon | cut -d _ -f1`; \
		CONTEXT=`echo $$icon | cut -d _ -f2`; \
		SIZE=`echo $$icon | cut -d _ -f3`; \
		ICONFILE=`echo $$icon | cut -d _ -f4`; \
		mkdir -p $(DESTDIR)/$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/images/$$icon $(DESTDIR)$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

uninstall-icons:
	for icon in $(public_icons); do \
		THEME=`echo $$icon | cut -d _ -f1`; \
		CONTEXT=`echo $$icon | cut -d _ -f2`; \
		SIZE=`echo $$icon | cut -d _ -f3`; \
		ICONFILE=`echo $$icon | cut -d _ -f4`; \
		rm -f $(DESTDIR)/$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done; \
	for icon in $(private_icons); do \
		THEME=`echo $$icon | cut -d _ -f1`; \
		CONTEXT=`echo $$icon | cut -d _ -f2`; \
		SIZE=`echo $$icon | cut -d _ -f3`; \
		ICONFILE=`echo $$icon | cut -d _ -f4`; \
		rm -f $(DESTDIR)/$(pkgdatadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done

install-data-local: install-icons update-icon-cache

uninstall-local: uninstall-icons update-icon-cache
