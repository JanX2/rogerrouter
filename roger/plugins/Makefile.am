SUBDIRS = statusicon fritzfon gtknotify vcard thunderbird

plugindir = ${libdir}/roger/

if WITH_EBOOK
SUBDIRS += evolution
endif

if WITH_LIBNOTIFY
SUBDIRS += notification
endif

if WITH_APPINDICATOR3
SUBDIRS += indicator
endif

if WITH_GDATA
SUBDIRS += google
endif

if WITH_KWALLET
SUBDIRS += kwallet
endif

if WITH_MACOS_KEYCHAIN
SUBDIRS += keychain
endif
