/*************************************<********************************
 *                
 * Filename:      irobex.c
 * Version:       0.5
 * Description:   IrOBEX, IrDA transport for OBEX
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Fri Apr 23 14:28:13 1999
 * Modified at:   Tue Nov 23 21:34:42 1999
 * Modified by:   Pontus Fuchs <pontus@tactel.se>
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

#ifdef HAVE_IRDA

#include <fcntl.h>
#include <signal.h>
#include <string.h>

/* for getpid */
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include <irda.h>

#include <obex_main.h>
#include <irobex.h>

#ifndef AF_IRDA
#define AF_IRDA 23
#endif /* AF_IRDA */


/*
 * Function irobex_listen (self)
 *
 *    Wait for incomming connections
 *
 */
gint irobex_listen(obex_t *self, char *service)
{
	int addrlen = sizeof(struct sockaddr_irda);
	int mtu;
	int len = sizeof(int);

	DEBUG(3, __FUNCTION__ "()\n");

	if(obex_create_socket(self, AF_IRDA, self->async) < 0)
		return -1;

	/* Bind local service */
	self->trans.self.irda.sir_family = AF_IRDA;

	if (service == NULL)
		strncpy(self->trans.self.irda.sir_name, "OBEX", 25);
	else
		strncpy(self->trans.self.irda.sir_name, service, 25);

	self->trans.self.irda.sir_lsap_sel = LSAP_ANY;
	
	if (bind(self->fd, (struct sockaddr*) &self->trans.self.irda, 
		 sizeof(struct sockaddr_irda))) 
	{
		return -1;
	}

	if (listen(self->fd, 2)) {
		return -1;
	}
	self->fd = accept(self->fd, (struct sockaddr *) &self->trans.peer.irda,
 			  &addrlen);
	if (self->fd < 0) {
		return -1;
	}

	if(self->async) {
		if(obex_register_async(self, self->fd) < 0)	{
			close(self->fd);
			self->fd = -1;
			return -1;
		}
	}

	/* Check what the IrLAP data size is */
	if (getsockopt(self->fd, SOL_IRLMP, IRTTP_MAX_SDU_SIZE, (void *) &mtu, 
		       &len)) 
	{
		return -1;
	}
	self->trans.mtu = mtu;
	DEBUG(3, __FUNCTION__ "(), transport mtu=%d\n", mtu);

	return 0;
}

/*
 * Function irobex_discover_devices (fd)
 *
 *    Try to discover some remote device(s) that we can connect to
 *
 */
gint irobex_discover_devices(obex_t *self)
{
	struct irda_device_list *list;
	unsigned char *buf;
	int len;
	int i;

	len = sizeof(struct irda_device_list) -
		sizeof(struct irda_device_info) +
		sizeof(struct irda_device_info) * MAX_DEVICES;

	buf = g_malloc(len);
	list = (struct irda_device_list *) buf;
	
	if (getsockopt(self->fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &len)) {
		return -1;
	}

	if (len > 0) {
		DEBUG(1, "Discovered: (list len=%d)\n", list->len);

		for (i=0;i<list->len;i++) {
			DEBUG(1, "  name:  %s\n", list->dev[i].info);
			DEBUG(1, "  daddr: %08x\n", list->dev[i].daddr);
			DEBUG(1, "  saddr: %08x\n", list->dev[i].saddr);
			DEBUG(1, "\n");
			
			self->trans.peer.irda.sir_addr = list->dev[i].daddr;
			self->trans.self.irda.sir_addr = list->dev[i].saddr;

#if 1
			/* 
			   It is not required by the standard to have the 
			   hint bit set, only recommended. Win95 does not.
			*/
			return 0;
#else
			/* Make sure we discovered an OBEX device */
			if (list->dev[i].hints[1] & HINT_OBEX) {
				DEBUG(1, __FUNCTION__ "(), this one looks good\n");
				return 0;
			}
#endif
		}
	}
	DEBUG(1, __FUNCTION__ "(), didn't find any OBEX devices!\n");
	return -1;
}

/*
 * Function irobex_irda_connect_request (self)
 *
 *    Open the TTP connection
 *
 */
gint irobex_connect_request(obex_t *self)
{
	int mtu = 0;
	int len = sizeof(int);
	int ret;

	DEBUG(4, __FUNCTION__ "()\n");

	if(obex_create_socket(self, AF_IRDA, self->async) < 0)
		return -1;

	ret = irobex_discover_devices(self);
	if (ret == -1)	{
		return -1;
	}

	ret = connect(self->fd, (struct sockaddr*) &self->trans.peer.irda,
		      sizeof(struct sockaddr_irda));
	if (ret < 0) {
		g_print(__FUNCTION__ "(), ret=%d\n", ret);
		return ret;
	}

	/* Check what the IrLAP data size is */
	ret = getsockopt(self->fd, SOL_IRLMP, IRTTP_MAX_SDU_SIZE, 
			 (void *) &mtu, &len);
	if (ret < 0) {
		return ret;
	}
	self->trans.mtu = mtu;
	DEBUG(2, __FUNCTION__ "(), transport mtu=%d\n", mtu);
	
	return 0;
}

/*
 * Function irobex_link_disconnect_request (self)
 *
 *    Shutdown the IrTTP link
 *
 */
gint irobex_disconnect_request(obex_t *self)
{
	DEBUG(4, __FUNCTION__ "()\n");
	return obex_delete_socket(self);
}

#endif /* HAVE_IRDA */
