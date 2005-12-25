
if APPS
noinst_LIBRARIES = libircp.a

libircp_a_SOURCES = \
	ircp_client.c ircp_client.h \
	ircp_server.c ircp_server.h \
	ircp_io.c ircp_io.h \
	dirtraverse.c dirtraverse.h \
	debug.h ircp.h

bin_PROGRAMS = ircp 

LDADD = $(top_builddir)/lib/src/libopenobex.la libircp.a

INCLUDES = -I$(top_srcdir)/lib/src
endif

MAINTAINERCLEANFILES = Makefile.in
