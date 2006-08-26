
includedir = @includedir@/openobex

if GLIB
include_HEADERS = obex-client.h

lib_LTLIBRARIES = libopenobex-glib.la

libopenobex_glib_la_SOURCES =

libopenobex_glib_la_LDFLAGS = -version-info 1:0:0

libopenobex_glib_la_LIBADD = $(top_builddir)/lib/libopenobex.la
endif

INCLUDES = -I$(top_builddir)/include

MAINTAINERCLEANFILES = Makefile.in
