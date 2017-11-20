MXE=$(pwd)/mxe
MXE_SYSROOT=$MXE/usr/i686-w64-mingw32.shared.posix

echo Creating setup with files from $MXE_SYSROOT .
cp build/platform/windows/roger.nsi $MXE_SYSROOT
cp printer.nsh $MXE_SYSROOT
cp settings.ini $MXE_SYSROOT

makensis $MXE_SYSROOT/roger.nsi
