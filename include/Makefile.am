
MAINTAINERCLEANFILES = Makefile.in

all-local:
	@if [ ! -e openobex ] ; then $(LN_S) $(top_srcdir)/include openobex ; fi

clean-local:
	@rm -f openobex
