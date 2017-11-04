#!/bin/bash
NAME=`grep -Po "project\('\K.*?(?=')" meson.build`
VERSION=`grep -Po "version : '\K.*?(?=')" meson.build`

git archive --prefix=${NAME}-${VERSION}/ -o ${NAME}_${VERSION}.orig.tar.xz HEAD
