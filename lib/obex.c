/*********************************************************************
 *                
 * Filename:      obex.c
 * Version:       0.5
 * Description:   API to be used by applications wanting to use OBEX
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Sat Apr 17 16:50:25 1999
 * Modified at:   Sun Aug 13 11:51:47 AM CEST 2000
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

#include <config.h>

#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock.h>
#define ESOCKTNOSUPPORT 1
#else /* _WIN32 */

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#endif

#include "obex_main.h"
#include "obex_object.h"
#include "obex_connect.h"
#include "obex_client.h"

#ifdef HAVE_IRDA
#include "irobex.h"
#endif

/*
 * Function OBEX_Init ()
 *
 *    Initialize OBEX
 *
 */
obex_t *OBEX_Init(gint transport, obex_event_t eventcb, guint flags)
{
	obex_t *self;

#ifdef OBEX_DEBUG
	obex_net_debug = OBEX_DEBUG;
#endif

#ifndef HAVE_FASYNC
	if(flags & OBEX_FL_ASYNC) {
		g_message("This platform doesn't support async IO\n");
		return NULL;
	}
#endif

	g_return_val_if_fail(eventcb != NULL, NULL);

#ifdef _WIN32
	{
		WSADATA WSAData;	 // Contains details of the Winsocket implementation
	  	if (WSAStartup (MAKEWORD(2,0), &WSAData) != 0) {
			g_message("WSAStartup failed\n");
			return NULL;
		}
	}
#endif

	self = (obex_t*) g_malloc(sizeof(obex_t));
	if (!self) {
		/* perror(G_GNUC_FUNCTION "()"); */
		return NULL;
	}
	memset(self, 0, sizeof(obex_t));

	self->eventcb = eventcb;

	if(flags & OBEX_FL_ASYNC) {
		self->async = 1;
	}
	self->fd = -1;
	self->serverfd = -1;

	/* Init transport */
	self->trans.type = transport;
	self->trans.connected = FALSE;

	/* Allocate message buffers */
	self->rx_msg = g_netbuf_new(OBEX_DEFAULT_MTU);
	if (self->rx_msg == NULL)
		return NULL;

	self->tx_msg = g_netbuf_new(OBEX_DEFAULT_MTU);
	if (self->tx_msg == NULL) {
		g_netbuf_free(self->rx_msg);
		return NULL;
	}

	self->mtu_rx = OBEX_DEFAULT_MTU;
	self->mtu_tx = OBEX_MINIMUM_MTU;

#ifndef _WIN32
	/* Ignore SIGPIPE. Otherwise send() will raise it and the app will quit */
	signal(SIGPIPE, SIG_IGN);
#endif

	return self;
}

/*
 * Function OBEX_RegisterCTransport()
 *
 *    Register a custom transport.
 *
 */
gint OBEX_RegisterCTransport(obex_t *self, obex_ctrans_t *ctrans)
{
	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(ctrans != NULL, -1);

	memcpy(&self->ctrans, ctrans, sizeof(obex_ctrans_t));
	return 1;
}

/*
 * Function OBEX_Cleanup (self)
 *
 *    Call this when your're finished using OBEX
 *
 */
void OBEX_Cleanup(obex_t *self)
{
	g_return_if_fail(self != NULL);
	
	obex_transport_disconnect_request(self);
	
	if (self->tx_msg)
		g_netbuf_free(self->tx_msg);
	
	if (self->rx_msg)
		g_netbuf_free(self->rx_msg);
	
	g_free(self);
}


/*
 * Function OBEX_ObjectSetUserData()
 *
 *    Set userdata on handle
 *
 */
void OBEX_SetUserData(obex_t *self, gpointer data)
{
	g_return_if_fail(self != NULL);
	self->userdata=data;
}

/*
 * Function OBEX_GetUserData()
 *
 *    Get userdata from handle
 *
 */
gpointer OBEX_GetUserData(obex_t *self)
{
	g_return_val_if_fail(self != NULL, 0);
	return self->userdata;
}

/*
 * Function OBEX_ServerRegister (self, service)
 *
 *    Register server interest in OBEX
 *
 */
gint OBEX_ServerRegister(obex_t *self, char *service)
{
	DEBUG(3, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(service != NULL, -1);

	return obex_transport_listen(self, service);
}


/*
 * Function OBEX_HandleInput (self, timeout)
 *
 *    Used when the client or server is working in synchronous mode.
 *
 */
gint OBEX_HandleInput(obex_t *self, gint timeout)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");
	g_return_val_if_fail(self != NULL, -1);
	return obex_transport_handle_input(self, timeout);
}


/*
 * Function OBEX_CustomDataFeed()
 *
 *    Used to feed the OBEX-lib with data when using a custom transport.
 *
 */
gint OBEX_CustomDataFeed(obex_t *self, guint8 *inputbuf, gint actual)
{
	DEBUG(3, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(inputbuf != NULL, -1);

	return obex_data_indication(self, inputbuf, actual);
}


/*
 * Function OBEX_TransportConnect ()
 *
 *    Try to connect to peer.
 *
 */
gint OBEX_TransportConnect(obex_t *self, struct sockaddr *saddr, int addrlen)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(saddr != NULL, -1);

	memcpy(&self->trans.peer, saddr, addrlen);

	return obex_transport_connect_request(self);
}


/*
 * Function OBEX_TransportDisconnect ()
 *
 *    Disconnect transport.
 *
 */
gint OBEX_TransportDisconnect(obex_t *self)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);
	obex_transport_disconnect_request(self);
	return 0;
}


/*
 * Function IrOBEX_TransportConnect ()
 *
 *    An easier connect function to use for IrDA (IrOBEX) only
 *
 */
gint IrOBEX_TransportConnect(obex_t *self, char *service)
{
     	DEBUG(4, G_GNUC_FUNCTION "()\n");

	if (self->object)	{
		DEBUG(3, G_GNUC_FUNCTION "() We are busy. Bail out...\n");
		return -EBUSY;
	}

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(service != NULL, -1);

#ifdef HAVE_IRDA
	irobex_prepare_connect(self, service);
	return obex_transport_connect_request(self);
#else
	return -ESOCKTNOSUPPORT;
#endif /* HAVE_IRDA */
}


/*
 * Function OBEX_GetFD ()
 *
 *    Get FD 
 *
 */
gint OBEX_GetFD(obex_t *self)
{
	g_return_val_if_fail(self != NULL, -1);
	return self->fd;
}

/*
 * Function OBEX_Request ()
 *
 *    Send away a client-request.
 *
 */
gint OBEX_Request(obex_t *self, obex_object_t *object)
{
	DEBUG(3, G_GNUC_FUNCTION "()\n");

	if (self->object)	{
		DEBUG(3, G_GNUC_FUNCTION "() We are busy. Bail out...\n");
		return -EBUSY;
	}

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(object != NULL, -1);

	self->object = object;
	self->response_next = 1;
	self->lastcmd = object->cmd;

	if(obex_object_send(self, self->object, 1) < 0)
		obex_deliver_event(self, OBEX_CLIENT, OBEX_EV_LINKERR, self->object->cmd, 0, TRUE);
	return 0;
}


/*
 * Function OBEX_CancelRequest (self)
 *
 *    Cancel an ongoing tranfer
 *
 */
gint OBEX_CancelRequest(obex_t *self)
{
	g_print(G_GNUC_FUNCTION "(), not implemented yet!\n");
	g_return_val_if_fail(self != NULL, -1);
	return -1;
}


/*
 * Function OBEX_ObjectNew ()
 *
 *    Create a new OBEX-object.
 *
 */
obex_object_t *OBEX_ObjectNew(obex_t *self, guint8 cmd)
{
	obex_object_t *object;

	object = obex_object_new();
	if(object == NULL)
		return NULL;

	obex_object_setcmd(object, cmd, (guint8) (cmd | OBEX_FINAL));
	/* Need some special woodoo magic on connect-frame */
	if(cmd == OBEX_CMD_CONNECT)	{
		if(obex_insert_connectframe(self, object) < 0)	{
			obex_object_delete(object);
			object = NULL;
		}
	}

	return object;
}

/*
 * Function OBEX_ObjectSetUserData()
 *
 *    Set userdata on object
 *
 */
void OBEX_ObjectSetUserData(obex_object_t *object, gpointer data)
{
	g_return_if_fail(object != NULL);
	object->userdata=data;
}

/*
 * Function OBEX_ObjectGetUserData()
 *
 *    Get userdata from object
 *
 */
gpointer OBEX_ObjectGetUserData(obex_object_t *object)
{
	g_return_val_if_fail(object != NULL, 0);
	return object->userdata;
}


/*
 * Function OBEX_ObjectDelete ()
 *
 *    Delete an OBEX-object. Free any header attached to it.
 *
 */
gint OBEX_ObjectDelete(obex_t *self, obex_object_t *object)
{
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_delete(object);
}


/*
 * Function OBEX_ObjectAddHeader ()
 *
 *    Attach a header to an object.
 *
 */
gint OBEX_ObjectAddHeader(obex_t *self, obex_object_t *object, guint8 hi,
				obex_headerdata_t hv, guint32 hv_size,
				guint flags)
{
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_addheader(self, object, hi, hv, hv_size, flags);
}


/*
 * Function OBEX_ObjectGetNextHeader ()
 *
 *    Iterate through the attached to an object
 *
 */
gint OBEX_ObjectGetNextHeader(obex_t *self, obex_object_t *object, guint8 *hi,
					obex_headerdata_t *hv,
					guint32 *hv_size)
{
	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_getnextheader(self, object, hi, hv, hv_size);
}


/*
 * Function OBEX_ObjectSetRsp ()
 *
 *    Sets the response to a received request
 *
 */
gint OBEX_ObjectSetRsp(obex_object_t *object, guint8 cmd, guint8 lastcmd)
{
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_setrsp(object, cmd, lastcmd);
}

/*
 * Function OBEX_ObjectGetNonHdrData ()
 *
 *    Get any data which was before headers.
 *
 */
gint OBEX_ObjectGetNonHdrData(obex_object_t *object, guint8 **buffer)
{
	g_return_val_if_fail(object != NULL, -1);
	if(!object->rx_nonhdr_data)
		return 0;

	*buffer = object->rx_nonhdr_data->data;
	return object->rx_nonhdr_data->len;
}

/*
 * Function OBEX_ObjectSetNonHdrData ()
 *
 *    Set data to send before headers (ie SETPATH)
 *
 */
gint OBEX_ObjectSetNonHdrData(obex_object_t *object, guint8 *buffer, guint len)
{
//TODO: Check that we actually can send len bytes without violating MTU

	g_return_val_if_fail(object != NULL, -1);
	g_return_val_if_fail(buffer != NULL, -1);

	if(object->tx_nonhdr_data)
		return -1;

	object->tx_nonhdr_data = g_netbuf_new(len);
	if(!object->tx_nonhdr_data)
		return -1;

	g_netbuf_put_data(object->tx_nonhdr_data, buffer, len);
	return 1;
}

/*
 * Function OBEX_ObjectSetHdrOffset ()
 *
 *    Set the offset where to parse for headers.
 *    Call this when you get a OBEX_EV_REQHINT and you know
 *    that the command has data before the headers comes.
 *    You do NOT need to use this function on CONNECT and
 *    SETPATH. It's handled internally
 */
gint OBEX_ObjectSetHdrOffset(obex_object_t *object, guint offset)
{
	g_return_val_if_fail(object != NULL, -1);
	object->headeroffset = offset;
	return 1;
}

/*
 * Function OBEX_UnicodeToChar ()
 *
 *    Simple unicode to char function. Buffers may overlap.
 *
 */
gint OBEX_UnicodeToChar(guint8 *c, guint8 *uc, gint size)
{
	//FIXME: Check so that buffer is big enough
	int n = 0;
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(uc != NULL, -1);
	g_return_val_if_fail(c != NULL, -1);

	while ((c[n] = uc[n*2+1]))
		n++;
	return 0;
}


/*
 * Function OBEX_CharToUnicode ()
 *
 *    Simple char to unicode function. Buffers may overlap.
 *
 */
gint OBEX_CharToUnicode(guint8 *uc, guint8 *c, gint size)
{
	int len, n;
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(uc != NULL, -1);
	g_return_val_if_fail(c != NULL, -1);

	len = n = strlen(c);
	g_return_val_if_fail( (n*2 < size), -1);

	uc[n*2+1] = 0;
	uc[n*2] = 0;

	while(n--) {
		uc[n*2+1] = c[n];
		uc[n*2] = 0;
	}

	return (len*2)+2 ;
}


/*
 * Function OBEX_GetResponseMessage (self, rsp)
 *
 *    Return a human understandable string from a response-code.
 *
 */
GString* OBEX_GetResponseMessage(obex_t *self, gint rsp)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, NULL);
	return obex_get_response_message(self, rsp);
}
	