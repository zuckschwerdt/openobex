INCLUDES = @GLIB_CFLAGS@ @OPENOBEX_CFLAGS@

LDADD = @GLIB_LIBS@ @OPENOBEX_LIBS@ libircp.a

libircp_a_SOURCES = \
		ircp_client.c ircp_client.h \
		ircp_io.c ircp_io.h \
		dirtraverse.c dirtraverse.h \
		debug.h

noinst_LIBRARIES = libircp.a

ircp_SOURCES = ircp.c

bin_PROGRAMS = ircp

