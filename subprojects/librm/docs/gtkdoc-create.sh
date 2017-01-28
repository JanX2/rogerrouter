#!/bin/bash

DOC_MODULE=rm
SRC=`pwd`/..
DST=`pwd`/reference

gtkdoc-scan --module=$DOC_MODULE --source-dir=$SRC --output-dir=$DST --ignore-headers="plugins"
cd $DST
gtkdoc-mkdb --module=$DOC_MODULE --output=$DST --output-format=xml --source-dir=$SRC --name-space=Rm

gtkdoc-mkhtml $DOC_MODULE $DOC_MODULE-docs.xml
gtkdoc-fixxref --module=$DOC_MODULE --module-dir=$DST
