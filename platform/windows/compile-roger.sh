MXE=$(pwd)/mxe
export PATH=$MXE/usr/bin:$PATH
meson ../.. build --cross-file=mingw-cross.txt --prefix=$MXE/usr/i686-w64-mingw32.shared.posix --libdir=lib
cd build
ninja
ninja install
cd ..