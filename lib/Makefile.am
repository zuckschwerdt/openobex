# Targets

lib_LTLIBRARIES = libopenobex.la

INCLUDES = @GLIB_CFLAGS@ -I.

libopenobexincludedir = $(includedir)/obex

libopenobex_la_LDFLAGS = \
        -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
        -release $(LT_RELEASE) \
	-export-symbols obex.sym

libopenobex_la_SOURCES = \
	obex_main.c \
	obex_connect.c \
	obex.c \
	obex_header.c \
	obex_object.c \
	obex_transport.c \
	obex_server.c \
	obex_client.c \
	irobex.c \
	inobex.c \
	netbuf.c

libopenobexinclude_HEADERS = \
	obex.h \
	obex_const.h

EXTRA_DIST = 
	obex_main.h \
	obex_connect.h \
	obex_header.h \
	obex_object.h \
	obex_transport.h \
	obex_server.h \
	obex_client.h \
	irobex.h \
	inobex.h \
	netbuf.h \
	obex.sym
