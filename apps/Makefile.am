#
#  $Id$
#

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

obex_test_LDADD = @OPENOBEX_LIBS@ @BLUETOOTH_LIBS@ libmisc.a

obex_tcp_SOURCES = obex_tcp.c

irxfer_SOURCES = irxfer.c

irobex_palm3_SOURCES = irobex_palm3.c


LDADD = @OPENOBEX_LIBS@ libmisc.a

INCLUDES = @OPENOBEX_CFLAGS@ @BLUETOOTH_CFLAGS@

EXTRA_DIST = makefile.msc
