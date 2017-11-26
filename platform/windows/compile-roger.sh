MXE=$(pwd)/mxe
export PATH=$MXE/usr/bin:$PATH
meson ../.. build --cross-file=mingw-cross.txt --prefix=$MXE/i686-w64-mingw32.shared.posix --libdir=lib -Dfax_spooler=false
cd build
ninja
ninja install
cd ..