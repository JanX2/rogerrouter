#!/bin/bash
##
 # Roger Router
 # Copyright (c) 2012-2014 Jan-Michael Brummer
 #
 # This file is part of Roger Router.
 #
 # This program is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation; version 2 only.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

function build()
{
	echo "Configuring $1"
	if [ -f ./autogen.sh ]; then
		./autogen.sh
	else
		autoconf
	fi
	mingw32-configure
	if [ $? -ne 0 ]; then
		exit
	fi
	echo "Compiling $1"
	make -j4
	if [ $? -ne 0 ]; then
		exit
	fi
	echo "Installing $1 - please provide password"
	sudo make install-strip
	if [ $? -ne 0 ]; then
		sudo make install
	fi
	ADD_CFLAGS=""
}

function make_pkg
{
	if [ -f $2.done ]; then
		return
	fi

	if [ ! -f $2 ]; then
		echo "Downloading $1/$2"
		wget $1/$2
		if [ $? -ne 0 ]; then
			exit
		fi
	fi
	echo "Extracting $2"
	tar xf $2
	if [ $? -ne 0 ]; then
		exit
	fi
	if [ -f $windir/patches/$3.patch ]; then
		echo "Patching source with $windir/patches/$3.patch"
		patch -d $3 -p1 < $windir/patches/$3.patch
	fi
	echo "Entering directory $3"
	cd $3
	if [ $? -ne 0 ]; then
		exit
	fi
	build $3
	cd ..

	touch $2.done
}

function download
{
	if [ -f $2.done ]; then
		return
	fi

	if [ ! -f $2 ]; then
		echo "Downloading $1/$2"
		wget $1/$2
		if [ $? -ne 0 ]; then
			exit
		fi
	fi

	touch $2.done
}

topdir=`pwd`/..
windir=`pwd`

# ********************* INFO *****************************
echo "Cross-Compiling Roger Router for Windows 32-bit"

# ********************* MINGW32 PACKAGES **************************

# Install mingw32 tools

if [ $# -eq 1 ]; then
	if [ $1 == "setup" ]; then
		sudo dnf install mingw32-atk mingw32-gettext mingw32-pkg-config mingw32-binutils mingw32-glib2 mingw32-termcap mingw32-bzip2 mingw32-gtk3 \
			mingw32-wine-gecko mingw32-cairo mingw32-harfbuzz mingw32-win-iconv mingw32-cpp mingw32-headers mingw32-zlib mingw32-crt \
			mingw32-icu mingw32-expat mingw32-jasper mingw32-filesystem mingw32-libffi mingw32-fontconfig mingw32-libjpeg-turbo \
			mingw32-freetype mingw32-libpng mingw-binutils-generic mingw32-gcc-c++ mingw32-libxml2 mingw-filesystem-base \
			mingw32-gcc mingw32-pango mingw32-gdk-pixbuf mingw32-pixman mingw32-libsoup mingw32-dlfcn mingw32-libtiff mingw32-pthreads \
			mingw32-libogg mingw32-speex mingw32-librsvg2 mingw32-nsis mingw32-hicolor-icon-theme mingw32-gstreamer1-plugins-good \
			flex bison python-devel wget gcc patch autoconf automake intltool libtool glib2-devel
		sudo dnf install wine
	fi
fi

mkdir -p packages
cd packages


# ********************* 3rd PARTY PACKAGES ************************

# Download and compile sndfile
make_pkg "http://www.mega-nerd.com/libsndfile/files/" "libsndfile-1.0.25.tar.gz" "libsndfile-1.0.25"

# Download and compile libcapi20
make_pkg "http://tabos.org/downloads/" "libcapi20-3.0.7.tar.bz2" "capi20"

# ************ PATCHED PACKAGES ********************

# Download and compile spandsp (configure needs to be patched, gethostname not resolved)
make_pkg "http://soft-switch.org/downloads/spandsp/" "spandsp-0.0.6pre21.tgz" "spandsp-0.0.6"

# Download and compile gobject-introspection (default compilation broken due to python)
export python_dir=$HOME/.wine/drive_c/Python27
make_pkg "http://ftp.gnome.org/pub/GNOME/sources/gobject-introspection/1.42/" "gobject-introspection-1.42.0.tar.xz" "gobject-introspection-1.42.0"

# Download and compile libpeas (depends on gobject-introspection, g-ir-scanner)
#make_pkg "http://ftp.gnome.org/pub/GNOME/sources/libpeas/1.12/" "libpeas-1.12.1.tar.xz" "libpeas-1.12.1"
make_pkg "http://ftp.gnome.org/pub/GNOME/sources/libpeas/1.8/" "libpeas-1.8.1.tar.xz" "libpeas-1.8.1"

make_pkg "http://ftp.gnome.org/pub/GNOME/sources/gssdp/0.14/" "gssdp-0.14.15.tar.xz" "gssdp-0.14.15"

make_pkg "http://ftp.gnome.org/pub/GNOME/sources/gupnp/0.20/" "gupnp-0.20.17.tar.xz" "gupnp-0.20.17"

make_pkg "http://ftp.gnome.org/pub/GNOME/sources/json-glib/1.2/" "json-glib-1.2.0.tar.xz" "json-glib-1.2.0"

#make_pkg "http://www.pjsip.org/release/2.5.1/" "pjproject-2.5.1.tar.bz2" "pjproject-2.5.1"

# ******************** Roger Router *******************
download https://github.com/ArtifexSoftware/ghostpdl-downloads/releases/download/gs919/ gs919w32.exe

# Compile Roger Router
cd $topdir
build .

# Build windows executable
makensis win32/roger.nsi
