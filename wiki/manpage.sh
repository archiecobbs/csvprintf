#!/bin/sh

cat << "xxEOFxx"
#summary Wikified version of the csvprintf man page
#labels Featured

{{{
xxEOFxx

groff -r LL=131n -r LT=131n -Tlatin1 -man ../trunk/csvprintf.1 | sed -r -e 's/.\x08(.)/\1/g' -e 's/[[0-9]+m//g' 

cat << "xxEOFxx"
}}}
xxEOFxx
