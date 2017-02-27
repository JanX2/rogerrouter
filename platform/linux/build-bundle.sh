#!/bin/bash

BUILD_DIR=build
NAME=org.tabos.roger
JSON=$NAME.flatpak.json
REPO=repo
EXE=roger

echo "Removing build dir.."
rm -rf $BUILD_DIR

echo "Building $NAME with flatpak"
flatpak-builder --ccache --repo=$REPO --subject="Nightly build of Roger, `date`" $BUILD_DIR $JSON

echo "Creating bundle file..."
flatpak -v build-bundle $REPO $EXE.flatpak $NAME
