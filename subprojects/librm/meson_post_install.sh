#!/bin/sh

# Packagers set DESTDIR so we don't want to try writing to root
if [ -z $DESTDIR ]; then
	PREFIX=${MESON_INSTALL_PREFIX:-/usr}

	echo 'Compiling GSchema'
	glib-compile-schemas "$PREFIX/share/glib-2.0/schemas"
fi
