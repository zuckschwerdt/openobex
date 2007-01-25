/*********************************************************************
 *                
 * Filename:      inobex.c
 * Version:       
 * Description:   InOBEX, Inet transport for OBEX.
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Sat Apr 17 16:50:35 1999
 * CVS ID:	  $Id$
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif /*_WIN32*/

#include "obex_main.h"

#define OBEX_PORT 650

static void map_ip4to6(struct sockaddr_in *in, struct sockaddr_in6 *out)
{
	out->sin6_family = AF_INET6;
	if (in->sin_port == 0)
		out->sin6_port = htons(OBEX_PORT);
	else
		out->sin6_port = in->sin_port;
	out->sin6_flowinfo = 0;

	/* default, matches IN6ADDR_ANY */
	memset(out->sin6_addr.s6_addr, 0, sizeof(out->sin6_addr.s6_addr));
	switch (in->sin_addr.s_addr) {
	case INADDR_ANY:
		/* does not work, so use IN6ADDR_ANY
		 * which includes INADDR_ANY
		 */
		break;
	default:
		/* map the IPv4 address to [::FFFF:<ipv4>]
		 * see RFC2373 and RFC2553 for details
		 */
		out->sin6_addr.s6_addr16[10/2] = 0xFFFF;
		out->sin6_addr.s6_addr[12] = (in->sin_addr.s_addr >> 24) & 0xFF;
		out->sin6_addr.s6_addr[13] = (in->sin_addr.s_addr >> 16) & 0xFF;
		out->sin6_addr.s6_addr[14] = (in->sin_addr.s_addr >>  8) & 0xFF;
		out->sin6_addr.s6_addr[15] = (in->sin_addr.s_addr >>  0) & 0xFF;
		break;
	}
}

/*
 * Function inobex_prepare_connect (self, service)
 *
 *    Prepare for INET-connect
 *
 */
void inobex_prepare_connect(obex_t *self, struct sockaddr *saddr, int addrlen)
{
	struct sockaddr_in6 addr = {
		.sin6_family       = AF_INET6,
		.sin6_port         = htons(OBEX_PORT),
#ifdef _WIN32
		.sin6_addr.s6_addr = IN6ADDR_LOOPBACK_INIT,
#else
		.sin6_addr         = IN6ADDR_LOOPBACK_INIT,
#endif
		.sin6_flowinfo     = 0
	};
	if (saddr == NULL)
		saddr = (struct sockaddr*)(&addr);
	else
		switch (saddr->sa_family){
		case AF_INET6:
			break;
		case AF_INET:
			map_ip4to6((struct sockaddr_in*)saddr,&addr);
			/* no break */
		default:
			saddr = (struct sockaddr*)(&addr);
			break;
	}
	memcpy(&self->trans.peer.inet6, saddr, sizeof(self->trans.self.inet6));
}

/*
 * Function inobex_prepare_listen (self)
 *
 *    Prepare for INET-listen
 *
 */
void inobex_prepare_listen(obex_t *self, struct sockaddr *saddr, int addrlen)
{
	struct sockaddr_in6 addr = {
		.sin6_family       = AF_INET6,
		.sin6_port         = htons(OBEX_PORT),
#ifdef _WIN32
		.sin6_addr.s6_addr = IN6ADDR_ANY_INIT,
#else
		.sin6_addr         = IN6ADDR_ANY_INIT,
#endif
		.sin6_flowinfo     = 0
	};
	/* Bind local service */
	if (saddr == NULL)
		saddr = (struct sockaddr *) &addr;
	else
		switch (saddr->sa_family) {
		case AF_INET6:
			break;
		case AF_INET:
			map_ip4to6((struct sockaddr_in *) saddr, &addr);
			/* no break */
		default:
			saddr = (struct sockaddr *) &addr;
			break;
		}
	memcpy(&self->trans.self.inet6, saddr, sizeof(self->trans.self.inet6));
}

/*
 * Function inobex_listen (self)
 *
 *    Wait for incomming connections
 *
 */
int inobex_listen(obex_t *self)
{
	DEBUG(4, "\n");

	self->serverfd = obex_create_socket(self, AF_INET6);
	if(self->serverfd < 0) {
		DEBUG(0, "Cannot create server-socket\n");
		return -1;
	}

	//printf("TCP/IP listen %d %X\n", self->trans.self.inet.sin_port,
	//       self->trans.self.inet.sin_addr.s_addr);
	if (bind(self->serverfd, (struct sockaddr *) &self->trans.self.inet6,
						sizeof(struct sockaddr_in6))) {
		DEBUG(0, "bind() Failed\n");
		return -1;
	}

	if (listen(self->serverfd, 2)) {
		DEBUG(0, "listen() Failed\n");
		return -1;
	}

	DEBUG(4, "Now listening for incomming connections. serverfd = %d\n", self->serverfd);
	return 1;
}

/*
 * Function inobex_accept (self)
 *
 *    Accept incoming connection.
 *
 * Note : don't close the server socket here, so apps may want to continue
 * using it...
 */
int inobex_accept(obex_t *self)
{
	socklen_t addrlen = sizeof(struct sockaddr_in6);

	self->fd = accept(self->serverfd,
			  (struct sockaddr *) &self->trans.peer.inet6,
			  &addrlen);

	if(self->fd < 0)
		return -1;

	/* Just use the default MTU for now */
	self->trans.mtu = OBEX_DEFAULT_MTU;
	return 1;
}
	

/*
 * Function inobex_connect_request (self)
 *
 *    
 *
 */
int inobex_connect_request(obex_t *self)
{
	char addr[INET6_ADDRSTRLEN];
	int ret;

	self->fd = obex_create_socket(self, AF_INET6);
	if(self->fd < 0)
		return -1;

	/* Set these just in case */
	if (self->trans.peer.inet6.sin6_port == 0)
		self->trans.peer.inet6.sin6_port = htons(OBEX_PORT);

#ifndef _WIN32
	if (!inet_ntop(AF_INET6,&self->trans.peer.inet6.sin6_addr,
		       addr,sizeof(addr))) {
		DEBUG(4, "Adress problem\n");
		obex_delete_socket(self, self->fd);
		self->fd = -1;
		return -1;
	}
	DEBUG(2, "peer addr = [%s]:%u\n",addr,ntohs(self->trans.peer.inet6.sin6_port));
#endif

	ret = connect(self->fd, (struct sockaddr *) &self->trans.peer.inet6,
		      sizeof(struct sockaddr_in6));
	if (ret < 0) {
		DEBUG(4, "Connect failed\n");
		obex_delete_socket(self, self->fd);
		self->fd = -1;
		return ret;
	}

	self->trans.mtu = OBEX_DEFAULT_MTU;
	DEBUG(3, "transport mtu=%d\n", self->trans.mtu);

	return ret;
}

/*
 * Function inobex_transport_disconnect_request (self)
 *
 *    Shutdown the TCP/IP link
 *
 */
int inobex_disconnect_request(obex_t *self)
{
	int ret;
	DEBUG(4, "\n");
	ret = obex_delete_socket(self, self->fd);
	if(ret < 0)
		return ret;
	self->fd = -1;
	return ret;	
}

/*
 * Function inobex_transport_disconnect_server (self)
 *
 *    Close the server socket
 *
 * Used when we start handling a incomming request, or when the
 * client just want to quit...
 */
int inobex_disconnect_server(obex_t *self)
{
	int ret;
	DEBUG(4, "\n");
	ret = obex_delete_socket(self, self->serverfd);
	self->serverfd = -1;
	return ret;	
}
