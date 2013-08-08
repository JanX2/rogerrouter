#!/bin/bash

function build()
{
	echo "Configuring $1"
	autoconf
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

topdir=`pwd`/..
windir=`pwd`


# ********************* INFO *****************************
echo "Cross-Compiling Roger Router for Windows 32-bit"
read -p "Press any key to continue... " -n1 -s
echo ""

# ********************* MINGW32 PACKAGES **************************

# Install mingw32 tools
#sudo yum install mingw32-atk mingw32-gettext mingw32-pkg-config mingw32-binutils mingw32-glib2 mingw32-termcap mingw32-bzip2 mingw32-gtk3 \
#	mingw32-wine-gecko mingw32-cairo mingw32-harfbuzz mingw32-win-iconv mingw32-cpp mingw32-headers mingw32-zlib mingw32-crt \
#	mingw32-icu mingw32-expat mingw32-jasper mingw32-filesystem mingw32-libffi mingw32-fontconfig mingw32-libjpeg-turbo \
#	mingw32-freetype mingw32-libpng mingw-binutils-generic mingw32-gcc-c++ mingw32-libxml2 mingw-filesystem-base \
#	mingw32-gcc mingw32-pango mingw32-gdk-pixbuf mingw32-pixman mingw32-libsoup mingw32-dlfcn mingw32-libtiff mingw32-pthreads \
#	mingw32-nsis flex bison python-devel wine

mkdir -p packages
cd packages


# ********************* 3rd PARTY PACKAGES ************************

# Download and compile libogg
make_pkg "http://downloads.xiph.org/releases/ogg/" "libogg-1.3.1.tar.xz" "libogg-1.3.1"

# Download and compile speex (requires libogg)
make_pkg "http://downloads.xiph.org/releases/speex/" "speex-1.2rc1.tar.gz" "speex-1.2rc1"

# Download and compile sndfile
make_pkg "http://www.mega-nerd.com/libsndfile/files/" "libsndfile-1.0.25.tar.gz" "libsndfile-1.0.25"

# Download and compile libcapi20
make_pkg "http://tabos.org/roger/download/" "libcapi20-3.0.7.tar.bz2" "capi20"

# Download and compile portaudio
make_pkg "http://www.portaudio.com/archives/" "pa_stable_v19_20111121.tgz" "portaudio"

# Download and compile spandsp
make_pkg "http://soft-switch.org/downloads/spandsp/" "spandsp-0.0.6pre21.tgz" "spandsp-0.0.6"

# ************ PATCHED PACKAGES ********************

# Download and compile spandsp
#wget http://soft-switch.org/downloads/spandsp/spandsp-0.0.6pre21.tgz
#tar xf spandsp-0.0.6pre21.tgz
#cd spandsp-0.0.6
#export ac_cv_func_malloc_0_nonnull=yes
#export ac_cv_func_realloc_0_nonnull=yes
#mingw32-configure CFLAGS="-DLIBSPANDSP_EXPORTS -D_WIN32_WINNT=0x0501 -mwindows -mms-bitfields -Wl,-subsystem,windows -lws2_32" LDFLAGS="-D_WIN32_WINNT=0x0501"
#make -j4
#sudo make install
#cd ..

# Download and compile patched (without python) gobject introspection
#wget http://ftp.gnome.org/pub/GNOME/sources/gobject-introspection/1.36/gobject-introspection-1.36.0.tar.xz
#tar xf gobject-introspection-1.36.0.tar.xz
#cd gobject-introspection-1.36.0
#mingw32-configure
#make -j4
#sudo make install
#cd ..

# Download and compile libpeas
#wget http://ftp.gnome.org/pub/GNOME/sources/libpeas/1.8/libpeas-1.8.1.tar.xz
#tar xf libpeas-1.8.1.tar.xz
#cd libpeas-1.8.1
#mingw32-configure
#make -j4
#sudo make install
#cd ..

# ******************** Roger Router *******************

# Compile Roger Router
cd $topdir
build .

# Build windows executable
makensis win32/roger.nsi
