/*********************************************************************
 *                
 * Filename:      obex_transport.h
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Sat May  1 20:16:45 1999
 * Modified at:   Mon May 24 19:44:41 1999
 * Modified by:   Dag Brattli <dagb@cs.uit.no>
 * 
 *     Copyright (c) 1999 Dag Brattli, All Rights Reserved.
 *     
 *     This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU Lesser General Public
 *     License as published by the Free Software Foundation; either
 *     version 2 of the License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *     Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public
 *     License along with this library; if not, write to the Free Software
 *     Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *     MA  02111-1307  USA
 *     
 ********************************************************************/

#ifndef OBEX_TRANSPORT_H
#define OBEX_TRANSPORT_H

#ifdef _WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#ifdef HAVE_IRDA
#include "irda_wrap.h"
#endif /*HAVE_IRDA*/

#include "obex_main.h"

typedef union {
#ifdef HAVE_IRDA
	struct sockaddr_irda irda;
#endif /*HAVE_IRDA*/
	struct sockaddr_in   inet;
} saddr_t;

typedef struct obex_transport {
	gint	type;
     	gboolean connected;	/* Link connection state */
     	guint	mtu;		/* Tx MTU of the link */
	saddr_t	self;		/* Source address */
	saddr_t	peer;		/* Destination address */

} obex_transport_t;

struct obex_t;

/* function declarations are in obex_main.h to avoid circular dependencies */

#endif OBEX_TRANSPORT_H



