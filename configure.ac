dnl Process this file with autoconf to produce configure.

m4_define([roger_major], [1])
m4_define([roger_minor], [8])
m4_define([roger_micro], [4])

m4_define([roger_version],[roger_major.roger_minor.roger_micro])

AC_INIT([Roger Router],[roger_version],[http://www.tabos.org/roger])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PREFIX_DEFAULT(/usr)
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz])
AC_CONFIG_HEADERS(config.h)

AC_CONFIG_MACRO_DIR(m4)

# Check for compilers.
AC_PROG_CC
AC_CHECK_PROG(found_cc, "$CC", yes, no)
AM_PROG_CC_C_O

# Check for libtool
LT_INIT(win32-dll)

AC_CANONICAL_HOST

GLIB_GSETTINGS

ROGER_VERSION_MAJOR=roger_major()
AC_SUBST(ROGER_VERSION_MAJOR)
ROGER_VERSION_MINOR=roger_minor()
AC_SUBST(ROGER_VERSION_MINOR)
ROGER_VERSION_MICRO=roger_micro()
AC_SUBST(ROGER_VERSION_MICRO)

AC_CHECK_LIB(m, pow)
AC_CHECK_LIB(gthread-2.0, g_thread_init)
AC_PATH_PROG(INTLTOOL_MSGMERGE, msgmerge, msgmerge)
AC_PATH_PROG(INTLTOOL_XGETTEXT, xgettext, xgettext)

dnl Add the languages which your application supports to po/LINGUAS
GETTEXT_PACKAGE=roger
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define to the Gettext package name])
AC_SUBST(GETTEXT_PACKAGE)
IT_PROG_INTLTOOL([0.40.0])

ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT

AC_MSG_CHECKING([if cross-building for win32])
case "$host" in
	*-*-*mingw*)
		WIN32=yes
		macos=no
		OS_CFLAGS="-D_WIN32_WINNT=0x0501 -DROUTERMANAGER_PLUGINS='\"lib/routermanager/plugins/\"' -DAPP_PLUGINS='\"lib/roger/plugins/\"' -DAPP_DATA='\"share/roger/\"' -DAPP_LOCALE='\"share/locale\"'"
		OS_LIBS="-mwindows -mms-bitfields -Wl,-subsystem,windows -lws2_32"
		AC_CHECK_TOOL(WINDRES, windres)
	;;
	*darwin*)
		WIN32=no
		macos=yes
		OS_CFLAGS="-DROUTERMANAGER_PLUGINS='\"lib/routermanager/plugins/\"' -DAPP_PLUGINS='\"lib/roger/plugins/\"' -DAPP_DATA='\"share/roger/\"' -DAPP_LOCALE='\"share/locale\"'"
		OS_LIBS="-rdynamic"
	;;
	*freebsd*)
		WIN32=no
		macos=no
		AC_PREFIX_DEFAULT(/usr/local)
		OS_CFLAGS=""
		OS_LIBS="-rdynamic"
	;;
	*)
		WIN32=no
		macos=no
		OS_CFLAGS="-DROUTERMANAGER_PLUGINS='\"\$(libdir)/routermanager/plugins/\"' -DAPP_PLUGINS='\"\$(libdir)/roger/plugins/\"' -DAPP_DATA='\"\$(datadir)/roger/\"' -DAPP_LOCALE='\"\$(localedir)\"' -Wall"
		OS_LIBS="-rdynamic"
	;;
esac
AC_SUBST(OS_CFLAGS)
AC_SUBST(OS_LIBS)
AC_MSG_RESULT([$WIN32])
AM_CONDITIONAL(WIN32, test "x$WIN32" != "xno")
AM_CONDITIONAL(MACOSX, test "x$macos" != "xno")

PKG_CHECK_MODULES([EXTRA], [glib-2.0 >= 2.32 gio-2.0 >= 2.32 speex >= 1.0 speexdsp >= 1.0 sndfile >= 1.0.0 libxml-2.0])

# Check for GTK >= 3.0
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0])

PKG_CHECK_MODULES([GMODULE], [gmodule-2.0])

PKG_CHECK_MODULES([SOUP], [libsoup-2.4])

PKG_CHECK_MODULES([PEAS], [libpeas-1.0])
PKG_CHECK_MODULES([PEAS_GTK], [libpeas-gtk-1.0])

# Introspection
#GOBJECT_INTROSPECTION_CHECK([0.9.3])

#if test "$found_introspection" = "yes"; then
#	enable_introspection=yes
#	AC_DEFINE([ENABLE_INTROSPECTION], [1], [Define to enable GObject Introspection])
#else
#	enable_introspection=no
#fi

dnl *******************************************************************
dnl Decide which fax spooler solution to use
dnl *******************************************************************
# determine prefered default type
case "$host" in
	*-*-*mingw*)
		fax_printer=socket-server
	;;
	*darwin* | *freebsd*)
		PKG_CHECK_MODULES(FAX_SPOOLER, 
			[gio-2.0 >= 2.36], 
			[fax_printer=spoolerdir],
			[fax_printer=socket-server]
		)
	;;
	
	*)
		fax_printer=spoolerdir
	;;
esac

AC_ARG_WITH([fax_spooler-type],
	[AS_HELP_STRING([--with-fax-spooler-type=@<:@ARG@:>@],
		[type of fax printer spooler to be supported (possible values: spoolerdir, socket-server or auto) @<:@default=auto@:>@])],
	[if test x$with_fax_spooler_type = xauto; then 
		with_fax_spooler_type=$fax_printer
	 fi],
	[with_fax_spooler_type=$fax_printer])

AC_MSG_CHECKING(for fax spooler type)
case "$with_fax_spooler_type" in
	spoolerdir)
		AC_DEFINE(USE_PRINTER_SPOOLER, 1, [Expect files in spooler directory /var/spool/roger])
		AC_MSG_RESULT([Outgoing faxes are expected in spooler directory /var/spool/roger])
	;;
	socket-server)
		AC_DEFINE(USE_PRINTER_SERVER, 1, [Use printer server on port 9100])
		AC_MSG_RESULT([Outgoing faxes are received by fax printer server on port 9100])
	;;
	*)
		AC_MSG_ERROR([Invalid option $with_fax_spooler_type])
	;;
esac

AC_ARG_WITH([headerbar],
	[AS_HELP_STRING([--headerbar=@<:@ARG@:>@],
		[whether journal shoudl make use of GtkHeaderBar (possible values: no, yes) @<:@default=no@:>@])],
	[],
	[with_headerbar=no])

AC_MSG_CHECKING(for headerbar)
case "$with_headerbar" in
	yes)
		AC_DEFINE(USE_HEADERBAR, 1, [Use headerbar])
		AC_MSG_RESULT([Using GtkHeaderBar])
	;;
	no)
		AC_MSG_RESULT([Using statusbar])
	;;
	*)
		AC_MSG_ERROR([Invalid option $with_headerbar])
	;;
esac

dnl *******************************************************************
dnl CUPS backend is optional
dnl *******************************************************************
AC_ARG_WITH([cups],
	[AS_HELP_STRING([--with-cups=@<:@ARG@:>@],
		[Use cups (possible values: no, yes or check) @<:@default=no@:>@])],
	[],
	[with_cups=no])

AC_MSG_CHECKING(for cups)
AS_IF([test "x$with_cups" != xsuper],
[
        if test "x$with_cups" = xcheck; then
		if test -n "`cups-config --build 2> /dev/null`"; then
			with_cups=yes
		fi	
        elif test "x$with_cups" = xyes; then
		if ! test -n "`cups-config --build 2> /dev/null`"; then
			AC_MSG_FAILURE([--with-cups=yes was given, but test for cups-config failed])
		fi
        fi
        if test "x$with_cups" = xyes; then
                AC_SUBST([CUPS_CFLAGS], ["`cups-config --cflags`"])
                AC_SUBST([CUPS_LIBS], ["`cups-config --libs`"])
                ROGER_FEATURES_YES="\tcups backend\n$ROGER_FEATURES_YES"
        else
                with_cups=no
                ROGER_FEATURES_NO="\tcups backend\n$ROGER_FEATURES_NO"
        fi

])

AC_MSG_RESULT($with_cups)
AM_CONDITIONAL(WITH_CUPS, test "x$with_cups" != "xno")

dnl *******************************************************************
dnl Check for cups backend directory if cups is found
dnl *******************************************************************

if test "x$with_cups" = xyes; then
	AC_MSG_CHECKING(for cups backend  directory)
	AC_ARG_WITH(cupsbackenddir,
		AC_HELP_STRING([--with-cupsbackenddir=DIR],
			[ cups-backends directory (auto)]),
		[ cupsbackenddir="${withval}" ],
		[ cupsbackenddir=`cups-config --serverbin`/backend ]
	)
	if test -d "$cupsbackenddir"; then
		AC_SUBST([cupsbackenddir])
		AC_MSG_RESULT([$cupsbackenddir])
		AC_DEFINE([HAVE_CUPS_BACKEND], 1, [Use new CUPS backend])
	else
		AC_MSG_FAILURE([Cups backend directory not found!])
	fi
fi

# Check for unistd.h
AC_CHECK_HEADERS([unistd.h])

#  Check for sigaction and sigset
AC_CHECK_FUNCS(sigaction, [ have_sigaction="1" ], [ have_sigaction="0" ])

AC_CACHE_CHECK([for sigset()],[ac_cv_sigset],[
	AC_TRY_COMPILE([#include <signal.h>],[sigset(SIGTERM,SIG_IGN);],
	[ac_cv_sigset=yes], [ac_cv_sigset=no])
])
if test x"ac_cv_sigset" = xyes; then
	AC_DEFINE([HAVE_SIGSET],1,[Define to 1 if you have the `sigset' function.])
fi

	

dnl ********************************************************************
dnl libnotify is optional
dnl ********************************************************************
AC_ARG_WITH([libnotify],
        [AS_HELP_STRING([--with-libnotify=@<:@ARG@:>@],
        [Use libnotify (possible values: no, yes, check, or the path to libnotify) @<:@default=check@:>@])],
        [],
        [with_libnotify=check])

AC_MSG_CHECKING(for libnotify)
AS_IF([test "x$with_libnotify" != xsuper],
        [if test "x$with_libnotify" = xcheck; then
                $PKG_CONFIG --exists libnotify && with_libnotify=yes
        elif test "x$with_libnotify" = xyes; then
                ( $PKG_CONFIG --exists libnotify ) || AC_MSG_FAILURE(
                        [--with-libnotify=yes was given, but test for libnotify failed])
        fi
        if test "x$with_libnotify" = xyes; then
                AC_SUBST([LIBNOTIFY_CFLAGS], ["`$PKG_CONFIG --cflags libnotify`"])
                AC_SUBST([LIBNOTIFY_LIBS], ["`$PKG_CONFIG --libs libnotify`"])
                AC_DEFINE([HAVE_LIBNOTIFY], 1, [Have libnotify])
                ROGER_FEATURES_YES="\tlibnotify\n$ROGER_FEATURES_YES"
        else
                with_libnotify=no
                ROGER_FEATURES_NO="\tlibnotify\n$ROGER_FEATURES_NO"
        fi
])

AC_MSG_RESULT($with_libnotify)

AM_CONDITIONAL(WITH_LIBNOTIFY, test "x$with_libnotify" != "xno")

dnl ********************************************************************
dnl secret is optional
dnl ********************************************************************
AC_ARG_WITH([secret],
	[AS_HELP_STRING([--with-secret=@<:@ARG@:>@],
	[Use secret (possible values: no, yes, check, or the path to secret) @<:@default=check@:>@])],
	[],
	[with_secret=check])

AC_MSG_CHECKING(for libsecret-1 >= 0.11)
AS_IF([test "x$with_secret" != xno],
	[if test "x$with_secret" = xcheck; then
		$PKG_CONFIG --atleast-version=0.11 libsecret-1 && with_secret=yes
	elif test "x$with_secret" = xyes; then
		$PKG_CONFIG --atleast-version=0.11 libsecret-1 || AC_MSG_FAILURE(
		[--with-secret=yes was given, but test for secret failed])
	fi
	if test "x$with_secret" = xyes; then
		AC_SUBST([SECRET_CFLAGS], ["`$PKG_CONFIG --cflags libsecret-1`"])
		AC_SUBST([SECRET_LIBS], ["`$PKG_CONFIG --libs libsecret-1`"])

		ROGER_FEATURES_YES="\tsecret\n$ROGER_FEATURES_YES"
	else
		with_secret=no
		ROGER_FEATURES_NO="\tsecret\n$ROGER_FEATURES_NO"
	fi
])
AC_MSG_RESULT($with_secret)
AM_CONDITIONAL(WITH_SECRET, test "x$with_secret" != "xno")
AM_CONDITIONAL(HAVE_SECRET_SOURCE_REGISTRY, test "x$with_secret_registry" = "xyes")

dnl ********************************************************************
dnl gdata is optional
dnl ********************************************************************
AC_ARG_WITH([gdata],
	[AS_HELP_STRING([--with-gdata=@<:@ARG@:>@],
	[Use gdata (possible values: no, yes, check, or the path to secret) @<:@default=check@:>@])],
	[],
	[with_gdata=check])

AC_MSG_CHECKING(for libgdata)
AS_IF([test "x$with_gdata" != xno],
	[if test "x$with_gdata" = xcheck; then
		$PKG_CONFIG libgdata && with_gdata=yes
	elif test "x$with_gdata" = xyes; then
		$PKG_CONFIG libgdata || AC_MSG_FAILURE(
		[--with-gdata=yes was given, but test for gdata failed])
	fi
	if test "x$with_gdata" = xyes; then
		AC_SUBST([GDATA_CFLAGS], ["`$PKG_CONFIG --cflags libgdata`"])
		AC_SUBST([GDATA_LIBS], ["`$PKG_CONFIG --libs libgdata`"])

		ROGER_FEATURES_YES="\tgdata\n$ROGER_FEATURES_YES"
	else
		with_gdata=no
		ROGER_FEATURES_NO="\tgdata\n$ROGER_FEATURES_NO"
	fi
])
AC_MSG_RESULT($with_gdata)
AM_CONDITIONAL(WITH_GDATA, test "x$with_gdata" != "xno")

dnl ********************************************************************
dnl ebook is optional
dnl ********************************************************************
AC_ARG_WITH([ebook],
	[AS_HELP_STRING([--with-ebook=@<:@ARG@:>@],
	[Use ebook (possible values: no, yes, check, or the path to ebook) @<:@default=check@:>@])],
	[],
	[with_ebook=check])

AC_MSG_CHECKING(for libebook-1.2 >= 3.2.0)
AS_IF([test "x$with_ebook" != xno],
	[if test "x$with_ebook" = xcheck; then
		$PKG_CONFIG --atleast-version=3.2.0 libebook-1.2 && with_ebook=yes
	elif test "x$with_ebook" = xyes; then
		$PKG_CONFIG --atleast-version=3.2.0 libebook-1.2 || AC_MSG_FAILURE(
		[--with-ebook=yes was given, but test for ebook failed])
	fi
	if test "x$with_ebook" = xyes; then
		AC_SUBST([EBOOK_CFLAGS], ["`$PKG_CONFIG --cflags libebook-1.2`"])
		AC_SUBST([EBOOK_LIBS], ["`$PKG_CONFIG --libs libebook-1.2`"])

		ROGER_FEATURES_YES="\tebook\n$ROGER_FEATURES_YES"
	else
		with_ebook=no
		ROGER_FEATURES_NO="\tebook\n$ROGER_FEATURES_NO"
	fi
])
AC_MSG_RESULT($with_ebook)
AM_CONDITIONAL(WITH_EBOOK, test "x$with_ebook" != "xno")

AS_IF([test "x$with_ebook" != xno],
	[$PKG_CONFIG --atleast-version=3.5.4 libebook-1.2 && with_ebook_registry=yes
	if test "x$with_ebook_registry" = xyes; then
		AC_DEFINE([HAVE_EBOOK_SOURCE_REGISTRY], 1, [Have libebook >= 3.5.4])
		PKG_CHECK_MODULES([EVO_SHELL], evolution-shell-3.0 >= 3.5.4,, AC_MSG_ERROR([libebook >= 3.5.4 requires evolution-shell]))
	fi
])
AM_CONDITIONAL(HAVE_EBOOK_SOURCE_REGISTRY, test "x$with_ebook_registry" = "xyes")


dnl ********************************************************************
dnl gstreamer-0.10 is optional
dnl ********************************************************************
AC_ARG_WITH([gstreamer0],
	[AS_HELP_STRING([--with-gstreamer0=@<:@ARG@:>@],
	[Use gstreamer-0.10 (possible values: no, yes, check, or the path to gstreamer) @<:@default=check@:>@])],
	[],
	[with_gstreamer0=no])

AC_MSG_CHECKING(for gstreamer-0.10 and gstreamer-app-0.10)
AS_IF([test "x$with_gstreamer0" != xno],
	[if test "x$with_gstreamer0" = xcheck; then
		pkg-config --exists gstreamer-0.10 && pkg-config --exists gstreamer-app-0.10 && with_gstreamer0=yes
	elif test "x$with_gstreamer0" = xyes; then
		( pkg-config --exists gstreamer-0.10 && pkg-config --exists gstreamer-app-0.10 gstreamer-interfaces-0.10 ) || AC_MSG_FAILURE(
			[--with-gstreamer-0.10=yes was given, but test for gstreamer-0.10 and gstreamer-app-0.10 failed])
	fi
	if test "x$with_gstreamer0" = xyes; then
		AC_SUBST([GSTREAMER0_CFLAGS], ["`pkg-config --cflags gstreamer-0.10 gstreamer-app-0.10 gstreamer-interfaces-0.10`"])
		AC_SUBST([GSTREAMER0_LIBS], ["`pkg-config --libs gstreamer-0.10 gstreamer-app-0.10 gstreamer-interfaces-0.10`"])
		AC_DEFINE([HAVE_GSTREAMER0], 1, [Have gstreamer 0.10])
		ROGER_FEATURES_YES="\tgstreamer 0.10\n$ROGER_FEATURES_YES"
	else
		with_gstreamer0=no
		ROGER_FEATURES_NO="\tgstreamer 0.10\n$ROGER_FEATURES_NO"
	fi
])

AC_MSG_RESULT($with_gstreamer0)

AM_CONDITIONAL(WITH_GSTREAMER0, test "x$with_gstreamer0" != "xno")

dnl ********************************************************************
dnl gstreamer-1.x is optional
dnl ********************************************************************
AC_ARG_WITH([gstreamer1],
	[AS_HELP_STRING([--with-gstreamer1=@<:@ARG@:>@],
	[Use gstreamer-1.0 (possible values: no, yes, check, or the path to gstreamer) @<:@default=check@:>@])],
	[],
	[with_gstreamer1=no])

AC_MSG_CHECKING(for gstreamer-1.0 and gstreamer-app-1.0)
AS_IF([test "x$with_gstreamer1" != xno],
	[if test "x$with_gstreamer1" = xcheck; then
		pkg-config --exists gstreamer-1.0 && pkg-config --exists gstreamer-app-1.0 && with_gstreamer1=yes
	elif test "x$with_gstreamer1" = xyes; then
		( pkg-config --exists gstreamer-1.0 && pkg-config --exists gstreamer-app-1.0) || AC_MSG_FAILURE(
			[--with-gstreamer-1.0=yes was given, but test for gstreamer-1.0 and gstreamer-app-1.0 failed])
	fi
	if test "x$with_gstreamer1" = xyes; then
		AC_SUBST([GSTREAMER1_CFLAGS], ["`pkg-config --cflags gstreamer-1.0 gstreamer-app-1.0`"])
		AC_SUBST([GSTREAMER1_LIBS], ["`pkg-config --libs gstreamer-1.0 gstreamer-app-1.0`"])
		AC_DEFINE([HAVE_GSTREAMER1], 1, [Have gstreamer 1.0])
		ROGER_FEATURES_YES="\tgstreamer 1.0\n$ROGER_FEATURES_YES"
	else
		with_gstreamer1=no
		ROGER_FEATURES_NO="\tgstreamer 1.0\n$ROGER_FEATURES_NO"
	fi
])

AC_MSG_RESULT($with_gstreamer1)

AM_CONDITIONAL(WITH_GSTREAMER1, test "x$with_gstreamer1" != "xno")

dnl ********************************************************************
dnl pulseaudio is optional
dnl ********************************************************************
AC_ARG_WITH([pulseaudio],
	[AS_HELP_STRING([--with-pulseaudio=@<:@ARG@:>@],
	[Use pulseaudio (possible values: no, yes, check, or the path to pulseaudio) @<:@default=check@:>@])],
	[],
	[with_pulseaudio=check])

AC_MSG_CHECKING(for libpulse-simple)
AS_IF([test "x$with_pulseaudio" != xsuper],
	[if test "x$with_pulseaudio" = xcheck; then
		$PKG_CONFIG --exists libpulse-simple && with_pulseaudio=yes
	elif test "x$with_pulseaudio" = xyes; then
		( $PKG_CONFIG --exists libpulse-simple ) || AC_MSG_FAILURE(
			[--with-pulseaudio=yes was given, but test for libpulse-simple failed])
	fi
	if test "x$with_pulseaudio" = xyes; then
		AC_SUBST([PULSEAUDIO_CFLAGS], ["`$PKG_CONFIG --cflags libpulse-simple`"])
		AC_SUBST([PULSEAUDIO_LIBS], ["`$PKG_CONFIG --libs libpulse-simple`"])
		AC_DEFINE([HAVE_PULSEAUDIO], 1, [Have pulseaudio])
		ROGER_FEATURES_YES="\tpulseaudio\n$ROGER_FEATURES_YES"
	else
		with_pulseaudio=no
		ROGER_FEATURES_NO="\tpulseaudio\n$ROGER_FEATURES_NO"
	fi
])

AC_MSG_RESULT($with_pulseaudio)

AM_CONDITIONAL(WITH_PULSEAUDIO, test "x$with_pulseaudio" != "xno")

dnl ********************************************************************
dnl portaudio is optional
dnl ********************************************************************
AC_ARG_WITH([portaudio],
	[AS_HELP_STRING([--with-portaudio=@<:@ARG@:>@],
	[Use portaudio (possible values: no, yes, check, or the path to portaudio) @<:@default=check@:>@])],
	[],
	[with_portaudio=check])

AC_MSG_CHECKING(for portaudio-2.0 version 19)
AS_IF([test "x$with_portaudio" != xsuper],
	[if test "x$with_portaudio" = xcheck; then
		$PKG_CONFIG --atleast-version=19 portaudio-2.0 && with_portaudio=yes
	elif test "x$with_portaudio" = xyes; then
		( $PKG_CONFIG --atleast-version=19 portaudio-2.0 ) || AC_MSG_FAILURE(
			[--with-portaudio=yes was given, but test for portaudio-2.0 failed])
	fi
	if test "x$with_portaudio" = xyes; then
		AC_SUBST([PORTAUDIO_CFLAGS], ["`$PKG_CONFIG --cflags portaudio-2.0`"])
		AC_SUBST([PORTAUDIO_LIBS], ["`$PKG_CONFIG --libs portaudio-2.0`"])
		AC_DEFINE([HAVE_PORTAUDIO], 1, [Have portaudio])
		ROGER_FEATURES_YES="\tportaudio\n$ROGER_FEATURES_YES"
	else
		with_portaudio=no
		ROGER_FEATURES_NO="\tportaudio\n$ROGER_FEATURES_NO"
	fi
])

AC_MSG_RESULT($with_portaudio)

AM_CONDITIONAL(WITH_PORTAUDIO, test "x$with_portaudio" != "xno")


dnl ********************************************************************
dnl faxophone
dnl ********************************************************************
AC_MSG_CHECKING(for faxophone)
AC_ARG_WITH([faxophone],
	[AS_HELP_STRING([--with-faxophone=@<:@ARG@:>@],
	[Use faxophone (possible values: yes or no) @<:@default=yes@:>@])],
	[],
	[with_faxophone=yes])
AC_MSG_RESULT($with_faxophone)

# Faxophone test
if test "x$with_faxophone" = xyes; then
	AC_DEFINE([HAVE_FAXOPHONE], 1, [Have faxophone])
	ROGER_FEATURES_YES="\tfaxophone\n$ROGER_FEATURES_YES"

	PKG_CHECK_MODULES([CAPI], [capi20 >= 3.6],
		[AC_DEFINE([HAVE_CAPI_36], 1, [Use new CAPI])],
		[PKG_CHECK_MODULES([CAPI], [capi20 >= 3.5])])

	dnl ********************************************************************
	dnl spandsp
	dnl ********************************************************************
	AC_ARG_WITH([spandsp],
		[AS_HELP_STRING([--with-spandsp=@<:@ARG@:>@],
		[Use spandsp (possible values: yes, check, or the path to spandsp) @<:@default=check@:>@])],
		[],
		[with_spandsp=check])

	AC_MSG_CHECKING(for spandsp)
	AS_IF([test "x$with_spandsp" != xsuper],
		[if test "x$with_spandsp" = xcheck; then
			$PKG_CONFIG --exists spandsp && with_spandsp=yes
		elif test "x$with_spandsp" = xyes; then
			( $PKG_CONFIG --exists spandsp ) || AC_MSG_FAILURE(
				[--with-spandsp=yes was given, but test for spandsp])
		fi
		if test "x$with_spandsp" = xyes; then
			AC_SUBST([SPANDSP_CFLAGS], ["`$PKG_CONFIG --cflags spandsp`"])
			AC_SUBST([SPANDSP_LIBS], ["`$PKG_CONFIG --libs spandsp`"])
			AC_MSG_RESULT($with_spandsp)
		else
			AC_MSG_RESULT($with_spandsp)
			AC_CHECK_LIB(spandsp, fax_init, [with_spandsp=yes], AC_MSG_ERROR([spandsp is required by this package]))
			AC_SUBST([SPANDSP_CFLAGS], [""])
			AC_SUBST([SPANDSP_LIBS], ["-lspandsp"])
		fi
	])
else
	ROGER_FEATURES_NO="\tfaxophone\n$ROGER_FEATURES_NO"
fi

AM_CONDITIONAL(WITH_FAXOPHONE, test "x$with_faxophone" != "xno")

dnl ********************************************************************
dnl appindicator is optional
dnl ********************************************************************
AC_ARG_WITH([appindicator3],
	[AS_HELP_STRING([--with-appindicator3=@<:@ARG@:>@],
	[Use appindicator3 (possible values: no, yes, check, or the path to appindicator3) @<:@default=check@:>@])],
	[],
	[with_appindicator3=check])

AC_MSG_CHECKING(for appindicator3-0.1)
AS_IF([test "x$with_appindicator3" != xno],
	[if test "x$with_appindicator3" = xcheck; then
		pkg-config --exists appindicator3-0.1 && with_appindicator3=yes
	elif test "x$with_appindicator3" = xyes; then
		pkg-config --exists appindicator3-0.1 || AC_MSG_FAILURE(
			[--with-appindicator3=yes was given, but test for appindicator3 failed])
	fi
	if test "x$with_appindicator3" = xyes; then
		AC_SUBST([APPINDICATOR_CFLAGS], ["`pkg-config --cflags appindicator3-0.1`"])
		AC_SUBST([APPINDICATOR_LIBS], ["`pkg-config --libs appindicator3-0.1`"])
		AC_DEFINE([HAVE_APPINDICATOR], 1, [Have appindicator])
		ROGER_FEATURES_YES="\tappindicator3\n$ROGER_FEATURES_YES"
	else
		with_appindicator3=no
		ROGER_FEATURES_NO="\tappindicator3\n$ROGER_FEATURES_NO"
	fi
])

AC_MSG_RESULT($with_appindicator3)

AM_CONDITIONAL(WITH_APPINDICATOR3, test "x$with_appindicator3" != "xno")

AC_ARG_ENABLE(werror,
	AS_HELP_STRING([--disable-werror], [don't use gcc's -Werror option when building]))

if test x"$enable_Werror" != xno
then
	EXTRA_CFLAGS+="-Werror"
fi

AC_CONFIG_FILES([
    Makefile
    win32/roger.nsi
    doc/Makefile
    doc/roger.1
    doc/roger_cli.1
    po/Makefile.in
    libroutermanager/routermanager.pc
    libroutermanager/Makefile
    libroutermanager/data/Makefile
    libroutermanager/plugins/Makefile
    libroutermanager/plugins/fritzbox/Makefile
    libroutermanager/plugins/callmonitor/Makefile
    libroutermanager/plugins/areacodes_global/Makefile
    libroutermanager/plugins/pulseaudio/Makefile
    libroutermanager/plugins/reverselookup/Makefile
    libroutermanager/plugins/wincred/Makefile
    libroutermanager/plugins/portaudio/Makefile
    libroutermanager/plugins/gstreamer/Makefile
    libroutermanager/plugins/gstreamer1/Makefile
    libroutermanager/plugins/secret/Makefile
    roger/Makefile
    roger/roger_winres.rc
    roger/data/Makefile
    roger-cli/Makefile
    roger/plugins/Makefile
    roger/plugins/statusicon/Makefile
    roger/plugins/statusicon/data/Makefile
    roger/plugins/notification/Makefile
    roger/plugins/notification/data/Makefile
    roger/plugins/fritzfon/Makefile
    roger/plugins/fritzfon/data/Makefile
    roger/plugins/evolution/Makefile
    roger/plugins/evolution/data/Makefile
    roger/plugins/gtknotify/Makefile
    roger/plugins/gtknotify/data/Makefile
    roger/plugins/indicator/Makefile
    roger/plugins/indicator/data/Makefile
    roger/plugins/vcard/Makefile
    roger/plugins/vcard/data/Makefile
    roger/plugins/google/Makefile
    roger/plugins/google/data/Makefile
    roger/plugins/thunderbird/Makefile
    roger/plugins/thunderbird/data/Makefile
    cups-backend/Makefile
    macosx/roger.plist
    macosx/package
])
AC_OUTPUT


printf "\n"
printf "configure: *** Extra features that will be built:"
printf "$ROGER_FEATURES_YES\n" | sort
printf "\n"
printf "configure: *** Extra features that will NOT be built:"
printf "$ROGER_FEATURES_NO\n" | sort
printf "\n"
