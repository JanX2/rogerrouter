#!/bin/sh

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
