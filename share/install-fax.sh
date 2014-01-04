#!/bin/sh
##
 # Roger Router
 # Copyright (c) 2012-2014 Jan-Michael Brummer, Louis Lagendijk
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
set -x

if test $# != 0; then
	echo Create the cups printer required for roger-router fax sending
	echo Usage: $0
	exit 1
fi 
if test `id -u` != 0; then
        echo Not started from user root. Cannot install printer. Use sudo $0 ?
	exit 2
fi

osname=`uname -s`
if test x$osname = xDarwin; then
	# Mac OSX -  test if group fax exists, if not create it

	found_group=$(dscl . -list /Groups | grep fax)
	if  test x$found_group != xfax; then
		# group fax does not yet exist
		# find a free group id, 501 - 1023 are local admin created group ids

		for  gid in {501..1023}; do
			found_gid=$(dscl . -list /Groups PrimaryGroupID | awk '{print $2}'|grep $gid 2>/dev/null)
			if test x$found_gid != x$gid; then
				break;
			fi
		done	

		# check that we really found a free group id

		if test x$found_gid = x$gid; then
			echo Group fax does not exist but no free group id found, cannot continue
			exit 4
		fi

		# found a free gid, now create the group

		echo allocating groupid $gid for group fax
		dscl . -create /groups/fax
		dscl . -append /groups/fax gid $gid
	fi
	backend_uid=root
	backend_gid=wheel
	spooldir_uid=lp
	spooldir_gid=fax
else
	# On linux etc we can do a simple groupadd to add the group
	groupadd -f fax
	backend_uid=root
	backend_gid=root
	spooldir_uid=lp
	spooldir_gid=fax
fi

# find cups serverbin directory for backend installation
# if cups-config can be found we use it, otherwise we try a number of locations for the backenddir

cupsconfig=$(which cups-config 2> /dev/null)

if test x$cupsconfig != x; then
	cupsbackenddir=$(cups-config --serverbin)/backend
else
	for bindir in /usr/lib/cups /usr/local/lib/cups /usr/libexec/cups /usr/local/libexec/cups; do
		if test -d $bindir/backend; then
			cupsbackenddir=$bindir/backend
			break;
		fi
	done
fi
	
if ! test -d $cupsbackenddir; then
	echo cannot find the cups backend directory!
	exit 5
fi

# Create spooler directory
mkdir -p /var/spool/roger/
chown $spooldir_uid:$spooldir_gid /var/spool/roger
chmod 2770 /var/spool/roger/

# install the backend
cp roger-cups $cupsbackenddir
chown $backend_uid:$backend_gid $cupsbackenddir
chmod 0755 $cupsbackenddir/roger-cups

#  Create the printer
lpadmin -p "Roger-Router-Fax" -E -v roger-cups:/ -P roger-fax.ppd
