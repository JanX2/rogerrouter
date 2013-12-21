#!/bin/sh
##
 # Roger Router
 # Copyright (c) 2012-2013 Jan-Michael Brummer, Louis Lagendijk
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
# set -x
if test $# != 1; then
	echo Add user to group fax
	echo Usage: $0 \<user\>
	echo where \<user\> is the user to be added to the group fax
	exit 1
fi
if test `id -u` != 0; then
	echo Not started from user root. Cannot add user to the fax group. Use sudo $0?
	exit 2
fi 
echo Adding user $1 to group fax

osname=`uname -s`
if test x$osname = xDarwin; then
	dseditgroup -o edit -a $1 -t user fax
else
	usermod -a -G fax $1
fi
