#!/bin/bash

#
# Script to regenerate all the GNU auto* gunk.
# Run this from the top directory of the source tree.
#
# If it looks like I don't know what I'm doing here, you're right.
#

set -e

. ./cleanup.sh
if [ "${1}" = '-C' ]; then
    exit 0
fi
mkdir -p scripts

ACLOCAL="aclocal"
AUTOHEADER="autoheader"
AUTOMAKE="automake"
AUTOCONF="autoconf"

echo "running aclocal"
${ACLOCAL} ${ACLOCAL_ARGS} -I scripts

echo "running autoheader"
${AUTOHEADER}

echo "running automake"
${AUTOMAKE} --add-missing -c --foreign

echo "running autoconf"
${AUTOCONF} -f -i

if [ "${1}" = '-c' ]; then
    echo "running configure"
    ./configure
fi

