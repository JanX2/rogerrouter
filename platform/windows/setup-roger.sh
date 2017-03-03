export WINDRES=/usr/bin/i686-w64-mingw32-windres
meson ../.. build --cross-file=mingw-cross.txt --prefix=/usr/i686-w64-mingw32/sys-root/mingw --libdir=lib