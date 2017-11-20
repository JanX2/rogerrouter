export WINDRES=i686-w64-mingw32.shared.posix-windres
meson ../.. build --cross-file=mingw-cross.txt --prefix=/home/jbrummer/Projekte/rogerrouter/platform/windows/mxe/usr/i686-w64-mingw32.shared.posix --libdir=lib -Dfax_spooler=false
cd build
ninja
ninja install
cd ..