/*********************************************************************
 *                
 * Filename:      inobex.c
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Sat Apr 17 16:50:35 1999
 * Modified at:   Sun Aug 13 02:10:12 PM CEST 2000
 * Modified by:   Pontus Fuchs <pontus.fuchs@tactel.se>
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
#include "config.h"
#include <stdio.h>

#ifdef _WIN32
#include <winsock.h>
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#endif /*_WIN32*/
#include "obex_main.h"

#define OBEX_PORT 650

/*
 * Function inobex_listen (self)
 *
 *    Wait for incomming connections
 *
 */
gint inobex_listen(obex_t *self, const char *service)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	self->serverfd = obex_create_socket(self, AF_INET, FALSE);
	if(self->serverfd < 0) {
		DEBUG(0, G_GNUC_FUNCTION "() Cannot create server-socket\n");
		return -1;
	}

	/* Bind local service */
	self->trans.self.inet.sin_family = AF_INET;
	self->trans.self.inet.sin_port = htons(OBEX_PORT);
	self->trans.self.inet.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(self->serverfd, (struct sockaddr*) &self->trans.self.inet,
		 sizeof(struct sockaddr_in))) 
	{
		DEBUG(0, G_GNUC_FUNCTION "() bind() Failed\n");
		return -1;
	}

	if (listen(self->serverfd, 2)) {
		DEBUG(0, G_GNUC_FUNCTION "() listen() Failed\n");
		return -1;
	}

	DEBUG(4, G_GNUC_FUNCTION "() Now listening for incomming connections. serverfd = %d\n", self->serverfd);
	return 1;
}

/*
 * Function inobex_accept (self)
 *
 *    Accept incoming connection.
 *
 */
gint inobex_accept(obex_t *self)
{
	int addrlen = sizeof(struct sockaddr_in);

	self->fd = accept(self->serverfd, (struct sockaddr *) 
		&self->trans.peer.inet, &addrlen);

	obex_delete_socket(self, self->serverfd);
	self->serverfd = -1;

	if(self->fd < 0)
		return -1;

	/* Just use the default MTU for now */
	self->trans.mtu = OBEX_DEFAULT_MTU;

	if(self->async)	{
		obex_register_async(self, self->fd);
	}
	return 1;
}
	

/*
 * Function inobex_connect_request (self)
 *
 *    
 *
 */
gint inobex_connect_request(obex_t *self)
{
	guchar *addr;
	gint ret;

	self->fd = obex_create_socket(self, AF_INET, 0);
	if(self->fd < 0)
		return -1;

	/* Set these just in case */
	self->trans.peer.inet.sin_family = AF_INET;
	self->trans.peer.inet.sin_port = htons(OBEX_PORT);

	addr = (char *) &self->trans.peer.inet.sin_addr.s_addr;

	DEBUG(2, G_GNUC_FUNCTION "(), peer addr = %d.%d.%d.%d\n",
		addr[0], addr[1], addr[2], addr[3]);


	ret = connect(self->fd, (struct sockaddr*) &self->trans.peer.inet, 
		      sizeof(struct sockaddr_in));
	if (ret < 0) {
		DEBUG(4, G_GNUC_FUNCTION "() Connect failed\n");
		obex_delete_socket(self, self->fd);
		self->fd = -1;
		return ret;
	}

	self->trans.mtu = OBEX_DEFAULT_MTU;
	DEBUG(3, G_GNUC_FUNCTION "(), transport mtu=%d\n", self->trans.mtu);

	if(self->async)	{
		obex_register_async(self, self->fd);
	}

	return ret;
}

/*
 * Function inobex_transport_disconnect_request (self)
 *
 *    Shutdown the TCP/IP link
 *
 */
gint inobex_disconnect_request(obex_t *self)
{
	gint ret;
	DEBUG(4, G_GNUC_FUNCTION "()\n");
	ret = obex_delete_socket(self, self->fd);
	if(ret < 0)
		return ret;
	self->fd = -1;
	return ret;	
}

