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
	mingw64-configure
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
echo "Cross-Compiling Roger Router for Windows 64-bit"

# ********************* MINgw64 PACKAGES **************************

# Install mingw64 tools

if [ $# -eq 1 ]; then
	if [ $1 == "setup" ]; then
		sudo dnf install mingw64-atk mingw64-gettext mingw64-pkg-config mingw64-binutils mingw64-glib2 mingw64-termcap mingw64-bzip2 mingw64-gtk3 \
			mingw64-wine-gecko mingw64-cairo mingw64-harfbuzz mingw64-win-iconv mingw64-cpp mingw64-headers mingw64-zlib mingw64-crt \
			mingw64-icu mingw64-expat mingw64-jasper mingw64-filesystem mingw64-libffi mingw64-fontconfig mingw64-libjpeg-turbo \
			mingw64-freetype mingw64-libpng mingw-binutils-generic mingw64-gcc-c++ mingw64-libxml2 mingw-filesystem-base \
			mingw64-gcc mingw64-pango mingw64-gdk-pixbuf mingw64-pixman mingw64-libsoup mingw64-dlfcn mingw64-libtiff mingw64-pthreads \
			mingw64-libogg mingw64-speex mingw64-librsvg2 mingw32-nsis mingw64-hicolor-icon-theme mingw64-gstreamer1-plugins-good mingw64-dlfcn mingw64-librsvg2\
			flex bison python-devel wget gcc patch autoconf automake intltool libtool glib2-devel mingw64-filesystem mingw64-gcc mingw64-dlfcn mingw64-libtiff mingw64-libsoup mingw64-speex patch
		sudo dnf install wine
	fi
fi

mkdir -p packages
cd packages


# ********************* 3rd PARTY PACKAGES ************************

# Download and compile sndfile
make_pkg "http://www.mega-nerd.com/libsndfile/files/" "libsndfile-1.0.27.tar.gz" "libsndfile-1.0.27"

# Download and compile libcapi20
make_pkg "http://tabos.org/downloads/" "libcapi20-3.0.7.tar.bz2" "capi20"

make_pkg "http://www.portaudio.com/archives/" "pa_stable_v190600_20161030.tgz" "portaudio"

# ************ PATCHED PACKAGES ********************

# Download and compile spandsp (configure needs to be patched, gethostname not resolved)
make_pkg "http://soft-switch.org/downloads/spandsp/" "spandsp-0.0.6pre21.tgz" "spandsp-0.0.6"

#make_pkg "https://downloads.sourceforge.net/project/libuuid/" "libuuid-1.0.3.tar.gz" "libuuid-1.0.3"

make_pkg "http://ftp.gnome.org/pub/GNOME/sources/gssdp/0.14/" "gssdp-0.14.15.tar.xz" "gssdp-0.14.15"

make_pkg "http://ftp.gnome.org/pub/GNOME/sources/gupnp/0.20/" "gupnp-0.20.17.tar.xz" "gupnp-0.20.17"

make_pkg "http://ftp.gnome.org/pub/GNOME/sources/json-glib/1.2/" "json-glib-1.2.0.tar.xz" "json-glib-1.2.0"

##make_pkg "http://www.pjsip.org/release/2.5.1/" "pjproject-2.5.1.tar.bz2" "pjproject-2.5.1"

# ******************** Roger Router *******************
download https://github.com/ArtifexSoftware/ghostpdl-downloads/releases/download/gs919/ gs919w32.exe
