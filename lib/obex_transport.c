/*********************************************************************
 *                
 * Filename:      obex_transport.c
 * Version:       
 * Description:   Code to handle different types of transports
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Sat May  1 20:15:04 1999
 * Modified at:   Fri Oct  8 21:22:04 1999
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

#include <stdio.h>

#include <obex_main.h>
#ifdef HAVE_IRDA
#include <irobex.h>
#endif /*HAVE_IRDA*/
#include <inobex.h>

#include <obex_transport.h>

#ifndef AF_IRDA
#define AF_IRDA 23
#endif /* AF_IRDA */

/*
 * Function obex_transport_connect_request (self, service)
 *
 *    Try to connect transport
 *
 */
gint obex_transport_connect_request(obex_t *self)
{
	int ret = -1;


	if(self->trans.connected)
		return 1;

	switch (self->trans.type) {
#ifdef HAVE_IRDA
	case OBEX_TRANS_IRDA:
		ret = irobex_connect_request(self);
		break;
#endif /*HAVE_IRDA*/
	case OBEX_TRANS_INET:
		ret = inobex_connect_request(self);
		break;
	case OBEX_TRANS_CUST:
		DEBUG(4, __FUNCTION__ "() Custom connect\n");
		if(self->ctrans.connect)
			ret = self->ctrans.connect(self);
		else
			g_message(__FUNCTION__ "(), No connect-callback exist!\n");
		DEBUG(4, __FUNCTION__ "ret=%d\n", ret);
		break;

	default:
		g_message(__FUNCTION__ "(), domain not implemented!\n");
		break;
	}
	if (ret >= 0)
		self->trans.connected = TRUE;

	return ret;
}

/*
 * Function obex_transport_disconnect_request (self)
 *
 *    Disconnect transport
 *
 */
void obex_transport_disconnect_request(obex_t *self)
{

	switch (self->trans.type) {
#ifdef HAVE_IRDA
	case OBEX_TRANS_IRDA:
		irobex_disconnect_request(self);
		break;
#endif /*HAVE_IRDA*/
	case OBEX_TRANS_INET:
		inobex_disconnect_request(self);
		break;	
	case OBEX_TRANS_CUST:
		DEBUG(4, __FUNCTION__ "() Custom disconnect\n");
		if(self->ctrans.disconnect)
			self->ctrans.disconnect(self);
		else
			g_message(__FUNCTION__ "(), No disconnect-callback exist!\n");
		break;
	default:
		g_message(__FUNCTION__ "() Transport not implemented!\n");
		break;
	}
	self->trans.connected = FALSE;
}

/*
 * Function obex_transport_listen (self, service)
 *
 *    Prepare for incomming connections
 *
 */
gint obex_transport_listen(obex_t *self, char *service)
{
	int ret = -1;

	switch (self->trans.type) {
#ifdef HAVE_IRDA
	case OBEX_TRANS_IRDA:
		ret = irobex_listen(self, service);
		break;
#endif /*HAVE_IRDA*/
	case OBEX_TRANS_INET:
		ret = inobex_listen(self, service);
		break;
	case OBEX_TRANS_CUST:
		DEBUG(4, __FUNCTION__ "() Custom listen\n");
		if(self->ctrans.listen)
			ret = self->ctrans.listen(self);
		else
			g_message(__FUNCTION__ "(), No listen-callback exist!\n");
		break;
	default:
		g_message(__FUNCTION__ "() Transport not implemented!\n");
		break;
	}
	return ret;
}

/*
 * Function obex_transport_write ()
 *
 *    Do the writing
 *
 */
gint obex_transport_write(obex_t *self, GNetBuf *msg)
{
	gint actual = -1;
	gint size;

	DEBUG(4, __FUNCTION__ "()\n");

	switch(self->trans.type)	{
#ifdef HAVE_IRDA
	case OBEX_TRANS_IRDA:
#endif /*HAVE_IRDA*/
	case OBEX_TRANS_INET:
		/* Send and fragment if necessary  */
		while (msg->len) {
			if (msg->len > self->trans.mtu)
				size = self->trans.mtu;
			else
				size = msg->len;
			DEBUG(1, __FUNCTION__ "(), sending %d bytes\n", size);

			actual = send(self->fd, msg->data, size, 0);

			if (actual != size) {
				perror("send");
				return actual;
			}
			/* Hide sent data */
			g_netbuf_pull(msg, size);
		}
		break;
	case OBEX_TRANS_CUST:
		DEBUG(4, __FUNCTION__ "() Custom write\n");
		if(self->ctrans.write)
			actual = self->ctrans.write(self, msg->data, msg->len);
		else
			g_message(__FUNCTION__ "(), No write-callback exist!\n");
		break;
	default:
		g_message(__FUNCTION__ "() Transport not implemented!\n");
		break;
	}	
	return actual;
}

/*
 * Function obex_transport_read ()
 *
 *    Do the reading
 *
 */
gint obex_transport_read(obex_t *self, guint8 *buf, gint buflen)
{
	gint actual = -1;
	GNetBuf *msg = self->rx_msg;

	DEBUG(4, __FUNCTION__ "()\n");

	switch(self->trans.type)	{
#ifdef HAVE_IRDA
	case OBEX_TRANS_IRDA:
#endif /*HAVE_IRDA*/
	case OBEX_TRANS_INET:
		actual = recv(self->fd, msg->tail, g_netbuf_tailroom(msg), 0);
		break;
	case OBEX_TRANS_CUST:
		memcpy(msg->tail, buf, buflen);
		actual = buflen;
		break;
	default:
		g_message(__FUNCTION__ "() Transport not implemented!\n");
		break;
	}	
	return actual;
}


