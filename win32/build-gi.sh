#!/bin/bash

export PYTHON=/usr/bin/python2

export topdir=`pwd`
export PATCH_DIR=$topdir/patches/
export srcdir=$topdir/packages/gobject-introspection

function prepare() {
cd $topdir/packages/
if [ -d gobject-introspection ]; then
echo "Existing directory"
else
git clone https://git.gnome.org/browse/gobject-introspection

cd $srcdir

# https://bugzilla.gnome.org/show_bug.cgi?id=761985
patch -Np1 < $PATCH_DIR/0003-Allow-overriding-of-the-host-os-name.patch

patch -Np1 < $PATCH_DIR/0007-Allow-to-skip-the-check-for-Python-headers.patch
patch -Np1 < $PATCH_DIR/0008-Overcome-File-name-too-long-error.patch
patch -Np1 < $PATCH_DIR/0009-Customize-CCompiler-on-MinGW-cross-compilation.patch

NOCONFIGURE=1 ./autogen.sh
fi
cd $srcdir
NOCONFIGURE=1 ./autogen.sh

# Clear environment trees created by wine
rm -fr "$srcdir/win32" "$srcdir/win64"
}

echo "Cross compiling gobject-introspection"
prepare
cd $srcdir

# First pass: build the giscanner Python module with build == host
./configure --disable-silent-rules --enable-shared --enable-static
make scannerparser.c _giscanner.la

export GI_HOST_OS=nt
export bits=32
export GI_CROSS_LAUNCHER="env WINEARCH=win$bits WINEPREFIX=$srcdir/win$bits DISPLAY= /usr/bin/wine"

# Second pass: build everything else with build != host (i.e. cross-compile)
mingw32-configure --disable-silent-rules --enable-shared --enable-static --disable-doctool --disable-giscanner

# Avoid overriding what built in the first pass
make -t scannerparser.c _giscanner.la

make
#sudo make install
