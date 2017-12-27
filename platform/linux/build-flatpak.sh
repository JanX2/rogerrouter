#!/bin/bash

BUILD_DIR=~/.builds
NAME=org.tabos.roger
JSON=$NAME.flatpak.json
REPO=repo
EXE=roger

echo "Removing build dir.."
rm -rf $BUILD_DIR

echo "Building $NAME with flatpak"
flatpak-builder --ccache --repo=$REPO --subject="Nightly build of Roger, `date`" $BUILD_DIR $JSON

flatpak build-sign repo --gpg-sign=2366F73FC963E8482C87A2D7E00E793AD2EE07CF --gpg-homedir=~/.gpg
flatpak build-update-repo repo --generate-static-deltas --gpg-sign=2366F73FC963E8482C87A2D7E00E793AD2EE07CF --gpg-homedir=~/.gpg

#echo "Creating bundle file..."
#flatpak -v build-bundle $REPO $EXE.flatpak $NAME
