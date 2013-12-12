SUBDIRS = libroutermanager roger roger-cli po doc

if !WIN32
SUBDIRS += cups-backend
endif

EXTRA_DIST = \
	autogen.sh \
	roger.desktop.in \
	remove-settings.sh \
	share/roger-cups \
	share/roger-fax.ppd \
	share/install-fax.sh \
	share/roger.appdata.xml \
	win32/setup-mingw.sh \
	win32/settings.ini \
	win32/printer.nsh \
	win32/roger.nsi.in \
	win32/roger.ico \
	win32/themes/Adwaita/gtk-3.0/gtk.css \
	win32/themes/Adwaita/gtk-3.0/gtk.gresource \
	win32/themes/Adwaita/gtk-3.0/settings.ini \
	win32/printer/ghostpdf.inf \
	win32/printer/ghostpdf.ppd \
	win32/patches/portaudio.patch \
	win32/patches/spandsp-0.0.6.patch

sharedir = $(datadir)/roger
share_DATA = \
	share/roger-cups\
	share/roger-fax.ppd\
	share/install-fax.sh

appdatadir = $(datadir)/appdata
appdata_DATA = \
	share/roger.appdata.xml

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
