lib_LTLIBRARIES = libopenobex.la

INCLUDES = @GLIB_CFLAGS@ -I.

libopenobexincludedir = $(includedir)/openobex

libopenobex_la_LDFLAGS = \
        -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
        -release $(LT_RELEASE) \
	-export-symbols obex.sym

libopenobex_la_SOURCES = \
	obex.c \
	obex_main.c obex_main.h \
	obex_connect.c obex_connect.h \
	obex_header.c obex_header.h \
	obex_object.c obex_object.h \
	obex_transport.c obex_transport.h \
	obex_server.c obex_server.h \
	obex_client.c obex_client.h \
	irobex.c irobex.h \
	inobex.c inobex.h \
	netbuf.c netbuf.h \
	irda.h irda_wrap.h

libopenobexinclude_HEADERS = \
	obex.h \
	obex_const.h

EXTRA_DIST = \
	obex.sym \
	obex.def \
	makefile.msc \
	win32compat.c
