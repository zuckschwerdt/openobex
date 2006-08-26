
includedir = @includedir@/openobex

if GLIB
include_HEADERS = obex-client.h

lib_LTLIBRARIES = libopenobex-glib.la

libopenobex_glib_la_SOURCES = obex-client.c

libopenobex_glib_la_LDFLAGS = -version-info 1:0:0

libopenobex_glib_la_LIBADD = @GLIB_LIBS@ $(top_builddir)/lib/libopenobex.la

noinst_PROGRAMS = test-client

test_client_LDADD = @GLIB_LIBS@ libopenobex-glib.la
endif

AM_CFLAGS = @GLIB_CFLAGS@

INCLUDES = -I$(top_builddir)/include

MAINTAINERCLEANFILES = Makefile.in
