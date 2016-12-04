#!/bin/sh

PREFIX=${MESON_INSTALL_PREFIX:-/usr}

glib-compile-schemas "$PREFIX/share/glib-2.0/schemas"
update-desktop-database -q
mkdir -p "$PREFIX/share/icons/hicolor"
gtk-update-icon-cache -q -t -f "$PREFIX/share/icons/hicolor"
