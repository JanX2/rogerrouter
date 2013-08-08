#!/bin/sh
groupadd -f fax
mkdir -p /var/spool/roger/
chown lp:fax /var/spool/roger
chmod 2770 /var/spool/roger/
cp roger-cups /usr/lib/cups/backend/
chown root:root /usr/lib/cups/backend/roger-cups
chmod 0755 /usr/lib/cups/backend/roger-cups
lpadmin -p "Roger-Router-Fax" -E -v roger-cups:/ -P roger-fax.ppd
