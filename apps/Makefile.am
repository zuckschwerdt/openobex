EXTRA_DIST = \
	makefile.msc

INCLUDES = @GLIB_CFLAGS@ @OPENOBEX_CFLAGS@

LDADD = @GLIB_LIBS@ @OPENOBEX_LIBS@ libmisc.a

libmisc_a_SOURCES = \
		obex_io.c obex_io.h \
		obex_put_common.c obex_put_common.h

noinst_LIBRARIES = libmisc.a

obex_test_SOURCES = \
	obex_test.c obex_test.h \
	obex_test_client.c obex_test_client.h \
	obex_test_server.c obex_test_server.h \
	obex_test_cable.c obex_test_cable.h

obex_tcp_SOURCES = obex_tcp.c

irxfer_SOURCES = irxfer.c

irobex_palm3_SOURCES = irobex_palm3.c

bin_PROGRAMS = irxfer obex_tcp irobex_palm3
noinst_PROGRAMS = obex_test
