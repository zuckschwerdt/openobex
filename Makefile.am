
SUBDIRS = include lib apps ircp doc

aclocaldir = $(datadir)/aclocal

aclocal_DATA = openobex.m4

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = openobex.pc

EXTRA_DIST = $(aclocal_DATA)

DISTCLEANFILES = $(pkgconfig_DATA)

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp missing install-sh mkinstalldirs
