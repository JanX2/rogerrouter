dnl Process this file with autoconf to produce configure.
AC_INIT(Roger Router, 1.8.0, http://www.tabos.org/roger)
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
		AC_CHECK_TOOL(WINDRES, windre, windres)
	;;
	*darwin*)
		WIN32=no
		macos=yes
		OS_CFLAGS="-DROUTERMANAGER_PLUGINS='\"\$(libdir)/routermanager/plugins/\"' -DAPP_PLUGINS='\"\$(libdir)/roger/plugins/\"' -DAPP_DATA='\"\$(datadir)/roger/\"' -DAPP_LOCALE='\"\$(localedir)\"' -DBUNDLE_DATADIR_SUFFIX='\"/roger\"' -DBUNDLE_LOCALEDIR_SUFFIX='\"/locale\"' -DBUNDLE_PLUGINDIR_SUFFIX='\"roger/plugins\"'"
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
		OS_CFLAGS="-DROUTERMANAGER_PLUGINS='\"\$(libdir)/routermanager/plugins/\"' -DAPP_PLUGINS='\"\$(libdir)/roger/plugins/\"' -DAPP_DATA='\"\$(datadir)/roger/\"' -DAPP_LOCALE='\"\$(localedir)\"'"
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

		$PKG_CONFIG --atleast-version=3.5.4 libebook-1.2 && with_ebook_registry=yes
		if test "x$with_ebook_registry" = xyes; then
			AC_DEFINE([HAVE_EBOOK_SOURCE_REGISTRY], 1, [Have libebook >= 3.5.4])
			PKG_CHECK_MODULES([EVO_SHELL], evolution-shell-3.0 >= 3.5.4,, AC_MSG_ERROR([libebook >= 3.5.4 requires evolution-shell]))
		fi
		ROGER_FEATURES_YES="\tebook\n$ROGER_FEATURES_YES"
	else
		with_ebook=no
		ROGER_FEATURES_NO="\tebook\n$ROGER_FEATURES_NO"
	fi
])
AC_MSG_RESULT($with_ebook)
AM_CONDITIONAL(WITH_EBOOK, test "x$with_ebook" != "xno")
AM_CONDITIONAL(HAVE_EBOOK_SOURCE_REGISTRY, test "x$with_ebook_registry" = "xyes")

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
AC_MSG_RESULT($with_faxophone)

AM_CONDITIONAL(WITH_FAXOPHONE, test "x$with_faxophone" != "xno")

dnl ********************************************************************
dnl dbus-glib is optional
dnl ********************************************************************
AC_ARG_WITH([dbus],
	[AS_HELP_STRING([--with-dbus=@<:@ARG@:>@],
	[Use dbus (possible values: no, yes, check, or the path to dbus-glib-1) @<:@default=check@:>@])],
	[],
	[with_dbus=check])

AC_MSG_CHECKING(for dbus-glib-1)
AS_IF([test "x$with_dbus" != xsuper],
	[if test "x$with_dbus" = xcheck; then
		$PKG_CONFIG --exists dbus-glib-1 && with_dbus=yes
	elif test "x$with_dbus" = xyes; then
		( $PKG_CONFIG --exists dbus-glib-1 ) || AC_MSG_FAILURE(
			[--with-dbus=yes was given, but test for dbus-glib-1 failed])
	fi
	if test "x$with_dbus" = xyes; then
		AC_SUBST([DBUS_CFLAGS], ["`$PKG_CONFIG --cflags dbus-glib-1`"])
		AC_SUBST([DBUS_LIBS], ["`$PKG_CONFIG --libs dbus-glib-1`"])
		AC_DEFINE([HAVE_DBUS], 1, [Have dbus-glib-1])
		ROGER_FEATURES_YES="\tdbus\n$ROGER_FEATURES_YES"
	else
		with_dbus=no
		ROGER_FEATURES_NO="\tdbus\n$ROGER_FEATURES_NO"
	fi
])

AC_MSG_RESULT($with_dbus)

AM_CONDITIONAL(WITH_DBUS, test "x$with_dbus" != "xno")


AC_ARG_ENABLE(werror,
	AS_HELP_STRING([--disable-werror], [don't use gcc's -Werror option when building]))

if test x"$enable_Werror" != xno
then
	EXTRA_CFLAGS+="-Werror"
fi

AC_OUTPUT([
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
    libroutermanager/plugins/secret/Makefile
    roger/Makefile
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
])


printf "\n"
printf "configure: *** Extra features that will be built:"
printf "$ROGER_FEATURES_YES\n" | sort
printf "\n"
printf "configure: *** Extra features that will NOT be built:"
printf "$ROGER_FEATURES_NO\n" | sort
printf "\n"
