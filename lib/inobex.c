/*********************************************************************
 *                
 * Filename:      inobex.c
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Sat Apr 17 16:50:35 1999
 * Modified at:   Fri Oct  8 21:23:53 1999
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

#include <config.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
//#include <signal.h>
#include <sys/socket.h>

#include <obex_main.h>

#define OBEX_PORT 650

/*
 * Function inobex_listen (self)
 *
 *    Wait for incomming connections
 *
 */
gint inobex_listen(obex_t *self, char *service)
{
//	int mtu;

	int addrlen = sizeof(struct sockaddr_in);

	DEBUG(3, __FUNCTION__ "()\n");

	if(obex_create_socket(self, AF_INET, FALSE) < 0)
		return -1;

	/* Bind local service */
	self->trans.self.inet.sin_family = AF_INET;
	self->trans.self.inet.sin_port = htons(OBEX_PORT);
	self->trans.self.inet.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(self->fd, (struct sockaddr*) &self->trans.self.inet,
		 sizeof(struct sockaddr_in))) 
	{
		DEBUG(4, __FUNCTION__ "() bind() Failed\n");
		return -1;
	}


	if (listen(self->fd, 2)) {
		DEBUG(4, __FUNCTION__ "() listen() Failed\n");
		return -1;
	}

	self->fd = accept(self->fd, (struct sockaddr *) &self->trans.peer.inet,
			  &addrlen);


	/* Just use the default MTU for now */
	self->trans.mtu = OBEX_DEFAULT_MTU;

	if(self->async)	{
		obex_register_async(self, self->fd);
	}
	

//	DEBUG(3, __FUNCTION__ "(), transport mtu=%d\n", mtu);

	return 0;
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

	if(obex_create_socket(self, AF_INET, 0) < 0)
		return -1;

	/* Set these just in case */
	self->trans.peer.inet.sin_family = AF_INET;
	self->trans.peer.inet.sin_port = htons(OBEX_PORT);

	addr = (char *) &self->trans.peer.inet.sin_addr.s_addr;

	g_print(__FUNCTION__ "(), peer addr = %d.%d.%d.%d\n",
		addr[0], addr[1], addr[2], addr[3]);


	ret = connect(self->fd, (struct sockaddr*) &self->trans.peer.inet, 
		      sizeof(struct sockaddr_in));
	if (ret < 0) {
		DEBUG(4, __FUNCTION__ "() Connect failed\n");
		return ret;
	}

	self->trans.mtu = OBEX_DEFAULT_MTU;
	DEBUG(3, __FUNCTION__ "(), transport mtu=%d\n", self->trans.mtu);

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
	return obex_delete_socket(self);
}

