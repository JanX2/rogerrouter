SUBDIRS = libroutermanager roger roger-cli po doc

EXTRA_DIST = \
	roger.desktop.in \
	remove-settings.sh \
	share/roger-cups \
	share/roger-fax.ppd \
	share/install-fax.sh \
	win32/setup-mingw.sh \
	win32/settings.ini \
	win32/printer.nsh \
	win32/roger.nsi.in \
	win32/roger.ico \
	win32/themes/* \
	win32/printer/* \
	win32/patches/*

noinst_HEADERS = config.h

DISTCLEANFILES = intltool-extract intltool-merge intltool-update roger.desktop

appsdir = $(datadir)/applications
apps_in_files = roger.desktop.in
apps_DATA = $(apps_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

install-data-local:
#	xdg-mime default roger.desktop x-scheme-handler/tel
#	xdg-mime install --mode system roger-tel.xml

uninstall-local:
#	xdg-mime uninstall --mode system roger-tel.xml
