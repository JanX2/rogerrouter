##
 # Roger Router
 # Copyright (c) 2012-2013 Jan-Michael Brummer
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

#!/bin/sh
groupadd -f fax
mkdir -p /var/spool/roger/
chown lp:fax /var/spool/roger
chmod 2770 /var/spool/roger/
cp roger-cups /usr/lib/cups/backend/
chown root:root /usr/lib/cups/backend/roger-cups
chmod 0755 /usr/lib/cups/backend/roger-cups
lpadmin -p "Roger-Router-Fax" -E -v roger-cups:/ -P roger-fax.ppd
