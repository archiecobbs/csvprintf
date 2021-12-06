
#
# csvprintf - Simple CSV file parser for the UNIX command line
# 

bin_PROGRAMS=		csvprintf

bin_SCRIPTS=		xml2csv

noinst_HEADERS=		csvprintf.h

man_MANS=		csvprintf.1

pkgdata_DATA=		csv.xsl

docdir=			$(datadir)/doc/packages/$(PACKAGE)

doc_DATA=		CHANGES COPYING README

EXTRA_DIST=		CHANGES INSTALL csvprintf.1.in xml2csv.in csv.xsl

csvprintf_SOURCES=	main.c \
			gitrev.c

DISTCLEANFILES=		csvprintf.1 xml2csv

SUFFIXES=		.in
.in:
			rm -f $@; $(subst) < $< >$@

.PHONY:			tests
tests:			csvprintf
			cd tests && ./run.sh

subst=			sed \
			    -e 's|@PACKAGE[@]|$(PACKAGE)|g' \
			    -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g' \
			    -e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
			    -e 's|@XSLTPROC[@]|$(XSLTPROC)|g'

install-data-hook:
			ln "$(DESTDIR)$(man1dir)"/csvprintf.1 "$(DESTDIR)$(man1dir)"/xml2csv.1

uninstall-hook:
			rm -f "$(DESTDIR)$(man1dir)"/xml2csv.1

gitrev.c:
			printf 'const char *const csvprintf_version = "%s";\n' "`git describe`" > gitrev.c

