
#
# csvprintf - Simple CSV file parser for the UNIX command line
# 
# $Id$
#

bin_PROGRAMS=		csvprintf

noinst_HEADERS=		csvprintf.h

man_MANS=		csvprintf.1

docdir=			$(datadir)/doc/packages/$(PACKAGE)

doc_DATA=		CHANGES COPYING README INSTALL

EXTRA_DIST=		CHANGES csvprintf.1 csvprintf.spec

csvprintf_SOURCES=	main.c \
			svnrev.c

svnrev.c:
			printf 'const int csvprintf_svnrev = %u;\n' `svn info | grep -w Rev | sed -r 's/^.*Rev: //g'` > svnrev.c

