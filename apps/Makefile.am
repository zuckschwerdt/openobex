
if APPS
noinst_LIBRARIES = libmisc.a

libmisc_a_SOURCES = \
	obex_io.c obex_io.h \
	obex_put_common.c obex_put_common.h

bin_PROGRAMS = irxfer obex_tcp irobex_palm3 obex_test

obex_test_SOURCES = \
	obex_test.c obex_test.h \
	obex_test_client.c obex_test_client.h \
	obex_test_server.c obex_test_server.h \
	obex_test_cable.c obex_test_cable.h

obex_test_LDADD = $(top_builddir)/lib/libopenobex.la @BLUEZ_LIBS@ @USB_LIBS@ libmisc.a

LDADD = $(top_builddir)/lib/libopenobex.la libmisc.a

INCLUDES = @USB_CFLAGS@ @BLUEZ_CFLAGS@ -I$(top_builddir)/include
endif

MAINTAINERCLEANFILES = Makefile.in
