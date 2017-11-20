MXEDIR=$(pwd)/mxe

if [ ! -d "$MXEDIR" ]; then
    git clone https://github.com/mxe/mxe.git
    cp mxe-pkgs/* $MXEDIR/src
fi

cd $MXEDIR
make capi gtk3 speex libsndfile speexdsp spandsp gst-plugins-good gst-plugins-bad librsvg libsoup json-glib gssdp gupnp ghostscript adwaita-icon-theme MXE_TARGETS=i686-w64-mingw32.shared.posix
