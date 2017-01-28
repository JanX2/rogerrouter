#!/bin/bash

DOC_MODULE=rm
SRC=.
DST=doc

gtkdoc-scan --module=$DOC_MODULE --source-dir=$SRC --output-dir=. --ignore-headers="plugins"
gtkdoc-mkdb --module=$DOC_MODULE --output=$DST --output-format=xml --source-dir=$SRC --name-space=Rm
mkdir -p html && cd html
gtkdoc-mkhtml $DOC_MODULE ../$DOC_MODULE-docs.xml
gtkdoc-fixxref --module=$DOC_MODULE --module-dir=html
