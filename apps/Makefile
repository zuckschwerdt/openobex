######################################################################
##                
## Filename:      Makefile
## Version:       
## Description:   Makefile for GNU OBEX test-programs
## Status:        Experimental.
## Author:        Dag Brattli <dagb@cs.uit.no>
## Created at:    Thu Feb 19 00:10:23 1998
## Modified at:   Tue Sep  7 22:47:46 1999
## Modified by:   Dag Brattli <dagb@cs.uit.no>
## 
## $Id$
##
##     Copyright (c) 1998-1999 Dag Brattli, All Rights Reserved.
##      
##     This program is free software; you can redistribute it and/or 
##     modify it under the terms of the GNU General Public License as 
##     published by the Free Software Foundation; either version 2 of 
##     the License, or (at your option) any later version.
##  
##     Neither Dag Brattli nor University of Tromsø admit liability nor
##     provide warranty for any of this software. This material is 
##     provided "AS-IS" and at no charge.
##     
######################################################################

CC = gcc
LD = ld
RM = rm -f

EXECS = irobex_palm3 irobex_test obex_tcp irxfer
RM_CMD = $(RM) *.BAK *.bak *.o ,* *~ *.a core $(EXECS)

INCLUDES = -I$(ROOT)/usr/include -I/usr/lib/glib/include
LIBRARIES = -lobex -lglib
LIBPATH = -L$(ROOT)/usr/lib

CFLAGS = $(RPM_OPT_FLAGS) -Wall $(INCLUDES)

all: $(EXECS)

irobex_test: irobex_test.o cobex_R320.o
	$(CC) $(CFLAGS) -o irobex_test irobex_test.o cobex_R320.o $(LIBPATH) $(LIBRARIES)

irobex_palm3: irobex_palm3.o obex_put_common.o
	$(CC) $(CFLAGS) -o irobex_palm3 irobex_palm3.o obex_put_common.o $(LIBPATH) $(LIBRARIES)

obex_tcp: obex_tcp.o obex_put_common.o
	$(CC) $(CFLAGS) -o obex_tcp obex_tcp.o obex_put_common.o $(LIBPATH) $(LIBRARIES)

irxfer: irxfer.o obex_put_common.o
	$(CC) $(CFLAGS) -o irxfer irxfer.o obex_put_common.o $(LIBPATH) $(LIBRARIES)


.c.o:
	$(CC) -g $(CFLAGS) $(XTRACFLAGS) -c $<

install: all
	install irobex_palm3   $(ROOT)/usr/bin
	install irobex_test    $(ROOT)/usr/bin
	install irxfer         $(ROOT)/usr/bin
	install obex_tcp       $(ROOT)/usr/bin

clean:
	$(RM_CMD)

distclean: clean

depend::
	makedepend -s "# DO NOT DELETE" -- $(INCLUDES) -- $(SRCS)
# DO NOT DELETE
