#!/bin/bash

export PYTHON=/usr/bin/python2

export topdir=`pwd`
export PATCH_DIR=$topdir/patches/
export SRCDIR=$topdir/packages/gobject-introspection-1.48.0

function prepare() {
cd $topdir/packages/
tar xf gobject-introspection-1.48.0.tar.xz
cd $SRCDIR

# https://bugzilla.gnome.org/show_bug.cgi?id=761985
patch -Np1 < $PATCH_DIR/0003-Allow-overriding-of-the-host-os-name.patch

patch -Np1 < $PATCH_DIR/0007-Allow-to-skip-the-check-for-Python-headers.patch
patch -Np1 < $PATCH_DIR/0008-Overcome-File-name-too-long-error.patch
patch -Np1 < $PATCH_DIR/0009-Customize-CCompiler-on-MinGW-cross-compilation.patch

# Clear environment trees created by wine
rm -fr "$srcdir/win32" "$srcdir/win64"
}

echo "Cross compiling gobject-introspection"
prepare

cd $SRCDIR

# First pass: build the giscanner Python module with build == host
setarch i686 ./configure --disable-silent-rules --enable-shared --enable-static
make scannerparser.c _giscanner.la

# Second pass: build everything else with build != host (i.e. cross-compile)
(

source mingw32-configure --disable-silent-rules --enable-shared --enable-static --disable-doctool --disable-giscanner

# Avoid overriding what built in the first pass
make -t scannerparser.c _giscanner.la

make
sudo make DESTDIR=/usr/i686-w64-mingw32/sys-root/mingw/ install
)