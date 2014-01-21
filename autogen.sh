#!/bin/sh
##
 # Roger Router
 # Copyright (c) 2012-2014 Jan-Michael Brummer
 #
 # This file is part of Roger Router.
 #
 # This program is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation; version 2 only.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

# Generate the Makefiles and configure files
if !( autoreconf --version ) </dev/null > /dev/null 2>&1; then
    echo "autoreconf not found -- aborting"
    exit 1
fi

echo "Updating generated configuration files with autoreconf..." && mkdir -p ./m4 && autoreconf --force --install --verbose
RES=$?
if [ $RES != 0 ]; then
    echo "Autogeneration failed (exit code $RES)"
    exit $RES
fi
rm -rf autom4te*.cache
echo "copy intltool related files" && intltoolize --automake --force --copy
echo 'run "./configure && make"'
