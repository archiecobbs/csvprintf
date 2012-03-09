#!/bin/sh

cat << "xxEOFxx"
#summary Wikified version of the csvprintf man page
#labels Featured

{{{
xxEOFxx

SEDFLAG='-r'
if [ `uname -s` = 'Darwin' ]; then
    echo This doesn\'t work on Mac OS 1>&2
    exit 1
fi
groff -r LL=100n -r LT=100n -Tlatin1 -man ../trunk/csvprintf.1 | sed "${SEDFLAG}" -e 's/.\x08(.)/\1/g' -e 's/[[0-9]+m//g' 

cat << "xxEOFxx"
}}}
xxEOFxx
