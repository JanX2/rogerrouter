#!/bin/bash

BUILD_DIR=build-local

echo "Building $NAME"
meson ../.. $BUILD_DIR --prefix=/usr

cd $BUILD_DIR
ninja-build

#ninja-build roger-pot
#ninja-build roger-update-po

cd ..