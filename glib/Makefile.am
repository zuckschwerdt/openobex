
includedir = @includedir@/openobex

if GLIB
include_HEADERS = obex-client.h

lib_LTLIBRARIES = libopenobex-glib.la

libopenobex_glib_la_SOURCES = obex-lowlevel.h obex-lowlevel.c obex-client.c

libopenobex_glib_la_LDFLAGS = -version-info 1:0:0

libopenobex_glib_la_LIBADD = @GLIB_LIBS@ $(top_builddir)/lib/libopenobex.la

BUILT_SOURCES = obex-marshal.h obex-marshal.c

nodist_libopenobex_glib_la_SOURCES = $(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES)

noinst_PROGRAMS = test-client

test_client_LDADD = @GLIB_LIBS@ libopenobex-glib.la
endif

AM_CFLAGS = @GLIB_CFLAGS@

INCLUDES = -I$(top_builddir)/include

EXTRA_DIST = obex-marshal.list

MAINTAINERCLEANFILES = Makefile.in

if GLIB
obex-marshal.h: obex-marshal.list
	$(GLIB_GENMARSHAL) --prefix=obex_marshal $< --header > $@

obex-marshal.c: obex-marshal.list
	$(GLIB_GENMARSHAL) --prefix=obex_marshal $< --body > $@
endif
