
SUBDIRS = include lib glib apps ircp doc

aclocaldir = $(datadir)/aclocal

aclocal_DATA = openobex.m4

pkgconfigdir = $(libdir)/pkgconfig

if GLIB
pkgconfig_DATA = openobex.pc openobex-glib.pc
else
pkgconfig_DATA = openobex.pc
endif

EXTRA_DIST = $(aclocal_DATA) CMakeLists.txt \
			CMakeModules/FindBluetooth.cmake CMakeModules/FindGlib.cmake \
			CMakeModules/FindIrda.cmake CMakeModules/FindLibUSB.cmake

DISTCLEANFILES = $(pkgconfig_DATA)

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp missing install-sh mkinstalldirs
