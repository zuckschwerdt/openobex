/*********************************************************************
 *                
 * Filename:      obex.c
 * Version:       0.9
 * Description:   API to be used by applications wanting to use OBEX
 * Status:        Stable.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Sat Apr 17 16:50:25 1999
 * CVS ID:        $Id$
 * 
 *     Copyright (c) 1999, 2000 Dag Brattli, All Rights Reserved.
 *     Copyright (c) 1999, 2000 Pontus Fuchs, All Rights Reserved.
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

#include "inobex.h"
#ifdef HAVE_IRDA
#include "irobex.h"
#endif
#ifdef HAVE_BLUETOOTH
#include "btobex.h"
#else
// This is to workaround compilation without BlueTooth support. - Jean II
typedef char *bdaddr_t;
#endif

/**
 * OBEX_Init - Initialize OBEX.
 * @transport: Which transport to use. The following transports are available :
 *             %OBEX_TRANS_IRDA : Use regular IrDA socket (need an IrDA stack)
 *             %OBEX_TRANS_INET : Use regular TCP/IP socket
 *             %OBEX_TRANS_CUST : Use user provided transport
 *             %OBEX_TRANS_BLUETOOTH: Use regular Bluetooth RFCOMM socket (need the BlueZ stack)
 *             If you use %OBEX_TRANS_CUST you must register your own
 *             transport with OBEX_RegisterCTransport()
 * @eventcb: Function pointer to your event callback.
 *           See obex.h for prototype of this callback.
 * @flags: Bitmask of flags. The following flags are available :
 *         %OBEX_FL_KEEPSERVER : Keep the server alive after incomming request
 *         %OBEX_FL_FILTERHINT : Filter target devices based on Obex hint bit
 *         %OBEX_FL_FILTERIAS  : Filter target devices based on IAS entry
 *
 * Returns an OBEX handle or %NULL on error.
 */
obex_t *OBEX_Init(gint transport, obex_event_t eventcb, guint flags)
{
	obex_t *self;

#ifdef OBEX_DEBUG
	obex_debug = OBEX_DEBUG;
#endif

	g_return_val_if_fail(eventcb != NULL, NULL);

#ifdef _WIN32
	{
		WSADATA WSAData;
	  	if (WSAStartup (MAKEWORD(2,0), &WSAData) != 0) {
			g_message("WSAStartup failed\n");
			return NULL;
		}
	}
#endif

	self = g_new0(obex_t, 1);
	if (self == NULL)
		return NULL;

	self->eventcb = eventcb;

	self->keepserver = (flags & OBEX_FL_KEEPSERVER) ? TRUE : FALSE;
	self->filterhint = (flags & OBEX_FL_FILTERHINT) ? TRUE : FALSE;
	self->filterias  = (flags & OBEX_FL_FILTERIAS ) ? TRUE : FALSE;
	self->fd = -1;
	self->serverfd = -1;
        self->state = MODE_SRV | STATE_IDLE;
	
	/* Init transport */
	self->trans.type = transport;
	self->trans.connected = FALSE;

	/* Allocate message buffers */
	/* It's safe to allocate them smaller than OBEX_MAXIMUM_MTU
	 * because netbuf will realloc data as needed. - Jean II */
	self->rx_msg = g_netbuf_new(OBEX_DEFAULT_MTU);
	if (self->rx_msg == NULL)
		goto out_err;

	self->tx_msg = g_netbuf_new(OBEX_DEFAULT_MTU);
	if (self->tx_msg == NULL)
		goto out_err;

	/* Safe values.
	 * Both self->mtu_rx and self->mtu_tx_max can be increased by app
	 * self->mtu_tx will be whatever the other end sneds us - Jean II */
	self->mtu_rx = OBEX_DEFAULT_MTU;
	self->mtu_tx = OBEX_MINIMUM_MTU;
	self->mtu_tx_max = OBEX_DEFAULT_MTU;

#ifndef _WIN32
	/* Ignore SIGPIPE. Otherwise send() will raise it and the app will quit */
	signal(SIGPIPE, SIG_IGN);
#endif

	return self;

out_err:
	if (self->tx_msg != NULL)
		g_netbuf_free(self->tx_msg);
	if (self->rx_msg != NULL)
		g_netbuf_free(self->rx_msg);
	g_free(self);
	return NULL;
}

/**
 * OBEX_RegisterCTransport - Register a custom transport
 * @self: OBEX handle
 * @ctrans: Structure with callbacks to transport operations
 * (see obex_const.h for details)
 *
 * Call this function directly after OBEX_Init if you are using
 * a custom transport.
 */
gint OBEX_RegisterCTransport(obex_t *self, obex_ctrans_t *ctrans)
{
	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(ctrans != NULL, -1);

	memcpy(&self->ctrans, ctrans, sizeof(obex_ctrans_t));
	return 1;
}

/**
 * OBEX_Cleanup - Close down an OBEX instance
 * @self: OBEX handle
 *
 * Close down an OBEX instance.
 */
void OBEX_Cleanup(obex_t *self)
{
	g_return_if_fail(self != NULL);
	
	obex_transport_disconnect_request(self);
	obex_transport_disconnect_server(self);

	if (self->tx_msg)
		g_netbuf_free(self->tx_msg);
	
	if (self->rx_msg)
		g_netbuf_free(self->rx_msg);
	
	g_free(self);
}


/**
 * OBEX_SetUserData - Set userdata of an OBEX handle
 * @self: OBEX handle
 * @data: It's all up to you!
 */
void OBEX_SetUserData(obex_t *self, gpointer data)
{
	g_return_if_fail(self != NULL);
	self->userdata=data;
}

/**
 * OBEX_GetUserData - Read the userdata from an OBEX handle
 * @self: OBEX handle
 *
 * Returns userdata
 */
gpointer OBEX_GetUserData(obex_t *self)
{
	g_return_val_if_fail(self != NULL, 0);
	return self->userdata;
}

/**
 * OBEX_SetUserCallBack - Change user callback on an OBEX handle
 * @self: OBEX handle
 * @eventcb: Function pointer to your new event callback.
 * @data: Pointer to the new user data to pass to the new callback (optional)
 */
void OBEX_SetUserCallBack(obex_t *self, obex_event_t eventcb, gpointer data)
{
	g_return_if_fail(self != NULL);
	/* The callback can't be NULL */
	if(eventcb != NULL) {
		self->eventcb = eventcb;
		/* Optionaly change the user data */
		if(data != NULL)
			self->userdata = data;
	}
}

/**
 * OBEX_SetTransportMTU - Set MTU to be used for receive and transmit
 * @self: OBEX handle
 * @mtu_rx: maximum receive transport packet size
 * @mtu_tx_max: maximum transmit transport packet size negociated
 *
 * Changing those values can increase the performance of the underlying
 * transport, but will increase memory consumption and latency (especially
 * abort latency), and may trigger bugs in buggy transport.
 * This need to be set *before* establishing the connection.
 *
 * Returns -1 on error.
 */
gint OBEX_SetTransportMTU(obex_t *self, guint16 mtu_rx, guint16 mtu_tx_max)
{
	g_return_val_if_fail(self != NULL, -EFAULT);
	if (self->object)	{
		DEBUG(1, G_GNUC_FUNCTION "() We are busy.\n");
		return -EBUSY;
	}
	if((mtu_rx < OBEX_MINIMUM_MTU) || (mtu_rx > OBEX_MAXIMUM_MTU))
		return -E2BIG;
	if((mtu_tx_max < OBEX_MINIMUM_MTU) || (mtu_tx_max > OBEX_MAXIMUM_MTU))
		return -E2BIG;

	/* Change MTUs */
	self->mtu_rx = mtu_rx;
	self->mtu_tx_max = mtu_tx_max;
	/* Reallocate transport buffers */
	self->rx_msg = g_netbuf_realloc(self->rx_msg, self->mtu_rx);
	if (self->rx_msg == NULL)
		return -ENOMEM;
	self->tx_msg = g_netbuf_realloc(self->tx_msg, self->mtu_tx_max);
	if (self->tx_msg == NULL)
		return -ENOMEM;
	return 0;
}

/**
 * OBEX_ServerRegister - Start listening for incoming connections
 * @self: OBEX handle
 * @saddr: Local address to bind to
 * @addrlen: Length of address
 *
 * Returns -1 on error.
 */
gint OBEX_ServerRegister(obex_t *self, struct sockaddr *saddr, int addrlen)
{
	DEBUG(3, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(saddr != NULL, -1);

	memcpy(&self->trans.self, saddr, addrlen);

	return obex_transport_listen(self);
}


/**
 * OBEX_ServerAccept - Accept an incoming connection
 * @server: OBEX handle
 * @eventcb: Event callback for client (use %NULL for same as server)
 * @data: Userdata for client (use %NULL for same as server)
 *
 * Create a new OBEX instance to handle the incomming connection.
 * The old OBEX instance will continue to listen for new connections.
 * The two OBEX instances become totally independant from each other.
 *
 * This function should be called after the library generates
 * an %OBEX_EV_ACCEPTHINT event to the user, but before the user
 * start to pull data out of the incomming connection.
 *
 * Using this function also requires that the OBEX handle was created
 * with the %OBEX_FL_KEEPSERVER flag set while calling OBEX_Init().
 *
 * Returns the client instance or %NULL for error.
 */
obex_t *OBEX_ServerAccept(obex_t *server, obex_event_t eventcb, gpointer data)
{
	obex_t *self;

	DEBUG(3, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(server != NULL, NULL);

	/* We can accept only if both the server and the connection socket
	 * are active */
	if((server->fd <= 0) || (server->serverfd <= 0))
		return(NULL);

	/* If we have started receiving something, it's too late... */
	if(server->object != NULL)
		return(NULL);

	/* Allocate new instance */
	self =  g_new0(obex_t, 1);
	if (self == NULL)
		return(NULL);

	/* Set callback and callback data as needed */
	if(eventcb != NULL)
		self->eventcb = eventcb;
	else
		self->eventcb = server->eventcb;
	
	if(data != NULL)
		self->userdata = data;
	else
		self->userdata = server->userdata;

	self->keepserver = server->keepserver;

	/* Copy transport data */
	memcpy(&self->trans, &server->trans, sizeof(obex_transport_t));
	memcpy(&self->ctrans, &server->ctrans, sizeof(obex_ctrans_t));

	self->mtu_rx = server->mtu_rx;
	self->mtu_tx = server->mtu_tx;
	self->mtu_tx_max = server->mtu_tx_max;

	/* Allocate message buffers */
	self->rx_msg = g_netbuf_new(self->mtu_rx);
	if (self->rx_msg == NULL)
		goto out_err;

	/* Note : mtu_tx not yet negociated, so let's be safe here - Jean II */
	self->tx_msg = g_netbuf_new(self->mtu_tx_max);
	if (self->tx_msg == NULL)
		goto out_err;

	/* Now, that's the interesting bit !
	 * We split the sockets apart, one for each instance */
	self->fd = server->fd;
	self->serverfd = -1;
	server->fd = -1;
        self->state = MODE_SRV | STATE_IDLE;

	return self;

out_err:
	if (self->tx_msg != NULL)
		g_netbuf_free(self->tx_msg);
	if (self->rx_msg != NULL)
		g_netbuf_free(self->rx_msg);
	g_free(self);
	return NULL;
}


/**
 * OBEX_HandleInput - Let the OBEX parser do some work
 * @self: OBEX handle
 * @timeout: Maximum time to wait in seconds
 *
 * Let the OBEX parser read and process incoming data. If no data
 * is available this call will block.
 *
 * When a request has been sent or you are waiting for an incoming server-
 * request you should call this function until the request has finished.
 *
 * Like select() this function returns -1 on error, 0 on timeout or
 * positive on success.
 */
gint OBEX_HandleInput(obex_t *self, gint timeout)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");
	g_return_val_if_fail(self != NULL, -1);
	return obex_transport_handle_input(self, timeout);
}


/**
 * OBEX_CustomDataFeed - Feed OBEX with data when using a custom transport
 * @self: OBEX handle
 * @inputbuf: Pointer to custom data
 * @actual: Length of buffer
 */
gint OBEX_CustomDataFeed(obex_t *self, guint8 *inputbuf, gint actual)
{
	DEBUG(3, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(inputbuf != NULL, -1);

	return obex_data_indication(self, inputbuf, actual);
}


/**
 * OBEX_TransportConnect - Try to connect to peer
 * @self: OBEX handle
 * @saddr: Address to connect to
 * @addrlen: Length of address
 *
 * Returns -1 on error.
 */
gint OBEX_TransportConnect(obex_t *self, struct sockaddr *saddr, int addrlen)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(saddr != NULL, -1);

	memcpy(&self->trans.peer, saddr, addrlen);

	return obex_transport_connect_request(self);
}


/**
 * OBEX_TransportDisconnect - Disconnect transport
 * @self: OBEX handle
 */
gint OBEX_TransportDisconnect(obex_t *self)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);
	obex_transport_disconnect_request(self);
	return 0;
}


/**
 * OBEX_GetFD - Get FD
 * @self: OBEX handle
 *
 * Returns the filedescriptor of the transport. Returns -1 on error.
 * Note that if you for example have a custom transport, no fd
 * is available.
 *
 * The returned filehandle can be used to do select() on, before
 * calling OBEX_HandleInput()
 *
 * There is one subtelty about this function. When the OBEX connection is
 * established, it returns the connection filedescriptor, while for
 * an unconnected server it will return the listening filedescriptor.
 * This mean that after receiving an incomming connection, you need to
 * call this function again.
 */
gint OBEX_GetFD(obex_t *self)
{
	g_return_val_if_fail(self != NULL, -1);
	if(self->fd == -1)
		return self->serverfd;
	return self->fd;
}

/**
 * OBEX_Request - Start a request (as client)
 * @self: OBEX handle
 * @object: Object containing request
 *
 * Returns negative on error.
 */
gint OBEX_Request(obex_t *self, obex_object_t *object)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	if (self->object)	{
		DEBUG(1, G_GNUC_FUNCTION "() We are busy.\n");
		return -EBUSY;
	}

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(object != NULL, -1);

	self->object = object;
        self->state = STATE_START | MODE_CLI;
	
	obex_client(self, NULL, 0);
	return 0;
}


/**
 * OBEX_CancelRequest - Cancel an ongoing operation
 * @self: OBEX handle
 * @nice: If true an OBEX Abort will be sent if beeing client
 *        or OBEX_RSP_UNAUTHORIZED as reponse if beeing server.
 *
 *
 */
gint OBEX_CancelRequest(obex_t *self, gboolean nice)
{
	g_return_val_if_fail(self != NULL, -1);
	return obex_cancelrequest(self, nice);
}


/**
 * OBEX_ObjectNew - Create a new OBEX Object
 * @self: OBEX handle
 * @cmd: command of object
 *
 * Returns a pointer to a new OBEX Object or %NULL on error.
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

/**
 * OBEX_ObjectDelete - Delete an OBEX object
 * @self: OBEX handle
 * @object: object to delete.
 *
 * Note that as soon as you have passed an object to the lib using
 * OBEX_Request(), you shall not delete it yourself.
 */
gint OBEX_ObjectDelete(obex_t *self, obex_object_t *object)
{
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_delete(object);
}


/**
 * OBEX_ObjectAddHeader - Attach a header to an object
 * @self: OBEX handle
 * @object: OBEX object
 * @hi: Header identifier
 * @hv: Header value
 * @hv_size: Header size
 * @flags: See obex.h for possible values
 *
 * Add a new header to an object.
 *
 * If you want all headers to fit in one packet, use the flag
 * %OBEX_FL_FIT_ONE_PACKET on all headers you add to an object.
 *
 * To stream a body add a body header with hv.bs = %NULL and set the flag
 * %OBEX_FL_STREAM_START. You will now get %OBEX_EV_STREAMEMPTY events as
 * soon as the the parser wants you to feed it with more data.
 *
 * When you get an %OBEX_EV_STREAMEMPTY event give the parser some data by
 * adding a body-header and set the flag %OBEX_EV_STREAM_DATA. When you
 * have no more data to send set the flag %OBEX_EV_STREAM_DATAEND instead.
 *
 * After adding a header you are free to do whatever you want with the buffer
 * if you are NOT streaming. If you are streaming you may not touch the
 * buffer until you get another %OBEX_EV_STREAMEMTPY or until the request
 * finishes.
 *
 * The headers will be sent in the order you add them.
 */
gint OBEX_ObjectAddHeader(obex_t *self, obex_object_t *object, guint8 hi,
				obex_headerdata_t hv, guint32 hv_size,
				guint flags)
{
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_addheader(self, object, hi, hv, hv_size, flags);
}


/**
 * OBEX_ObjectGetNextHeader - Get next available header from an object
 * @self: OBEX handle
 * @object: OBEX object
 * @hi: Pointer to header identifier
 * @hv: Pointer to hv
 * @hv_size: Pointer to hv_size
 *
 * Returns 0 when no more headers are available.
 *
 * All headers are read-only.
 *
 * You will get the headers in the received order.
 */
gint OBEX_ObjectGetNextHeader(obex_t *self, obex_object_t *object, guint8 *hi,
					obex_headerdata_t *hv,
					guint32 *hv_size)
{
	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_getnextheader(self, object, hi, hv, hv_size);
}

/**
 * OBEX_ObjectReParseHeaders - Allow the user to parse again the rx headers
 * @self: OBEX handle
 * @object: OBEX object
 *
 * The user must have extracted all headers from the object before
 * calling this function (until %OBEX_ObjectGetNextHeader() returns 0).
 * Next call to %OBEX_ObjectGetNextHeader() will return the first received
 * header.
 *
 * Returns 1 on success
 * Returns 0 if failed due previous parsing not completed.
 */
gint OBEX_ObjectReParseHeaders(obex_t *self, obex_object_t *object)
{
	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_reparseheaders(self, object);
}

/**
 * OBEX_ObjectReadStream - Read data from body stream
 * @self: OBEX handle
 * @object: OBEX object
 * @buf: A pointer to a pointer which this function will set to a buffer which
 * shall be read (and ONLY read) after this function returns.
 *
 * To recieve the body as a stream call this function with buf = %NULL as soon
 * as you get an OBEX_EV_REQHINT event.
 *
 * You will now recieve %OBEX_EV_STREAMAVAIL events when data is available
 * for you. Call this function to get the data.
 *
 * Note! When receiving a stream data is not buffered so if you don't call this
 * function when you get an %OBEX_EV_STREAMAVAIL event data will be lost.
 *
 * Returns the number of bytes in buffer, or 0 for end-of-stream.
 */
gint OBEX_ObjectReadStream(obex_t *self, obex_object_t *object, const guint8 **buf)
{
	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_readstream(self, object, buf);
}


/**
 * OBEX_ObjectSetRsp - Sets the response to a received request.
 * @self: OBEX handle
 * @object: OBEX object
 * @rsp: Respose code in non-last packets
 * @lastrsp: Response code in last packet
 *
 * Returns -1 on error.
 */
gint OBEX_ObjectSetRsp(obex_object_t *object, guint8 rsp, guint8 lastrsp)
{
	g_return_val_if_fail(object != NULL, -1);
	return obex_object_setrsp(object, rsp, lastrsp);
}

/**
 * OBEX_ObjectGetNonHdrData - Get any data which was before headers
 * @object: OBEX object
 * @buffer: Pointer to a pointer which will point to a read-only buffer
 *
 * Returns the size of the buffer or -1 for error.
 */
gint OBEX_ObjectGetNonHdrData(obex_object_t *object, guint8 **buffer)
{
	g_return_val_if_fail(object != NULL, -1);
	if(!object->rx_nonhdr_data)
		return 0;

	*buffer = object->rx_nonhdr_data->data;
	return object->rx_nonhdr_data->len;
}

/**
 * OBEX_ObjectSetNonHdrData - Set data to send before headers
 * @object: OBEX object
 * @buffer: Data to send
 * @len: Length to data
 *
 * Some commands (notably SetPath) send data before headers. Use this
 * function to set this data.
 */
gint OBEX_ObjectSetNonHdrData(obex_object_t *object, const guint8 *buffer, guint len)
{
	//TODO: Check that we actually can send len bytes without violating MTU

	g_return_val_if_fail(object != NULL, -1);
	g_return_val_if_fail(buffer != NULL, -1);

	if(object->tx_nonhdr_data)
		return -1;

	object->tx_nonhdr_data = g_netbuf_new(len);
	if(object->tx_nonhdr_data == NULL)
		return -1;

	g_netbuf_put_data(object->tx_nonhdr_data, (guint8 *)buffer, len);
	return 1;
}

/**
 * OBEX_ObjectSetHdrOffset - Set headeroffset
 * @object: OBEX object
 * @offset: Desired offset
 *
 * Call this function when you get a OBEX_EV_REQHINT and you know that the
 * command has data before the headers comes. You do NOT need to use this
 * function on Connect and SetPath, they are handled automatically.
 */
gint OBEX_ObjectSetHdrOffset(obex_object_t *object, guint offset)
{
	g_return_val_if_fail(object != NULL, -1);
	object->headeroffset = offset;
	return 1;
}

/**
 * OBEX_UnicodeToChar - Simple unicode to char function.
 * @c: Destination (char)
 * @uc: Source (unicode)
 * @size: Length of destination buffer
 *
 * Buffers may overlap. Returns -1 on error.
 */
gint OBEX_UnicodeToChar(guint8 *c, const guint8 *uc, gint size)
{
	gint n;
	DEBUG(4, G_GNUC_FUNCTION "()\n");
		
	g_return_val_if_fail(uc != NULL, -1);
	g_return_val_if_fail(c != NULL, -1);

	// Make sure buffer is big enough!
	for(n = 0; uc[n*2+1] != 0; n++);
	g_return_val_if_fail(n < size, -1);

	for(n = 0; uc[n*2+1] != 0; n++)
		c[n] = uc[n*2+1];
	c[n] = 0;
	
	return 0;
}

/**
 * OBEX_CharToUnicode - Simple char to unicode function.
 * @uc: Destination (unicode)
 * @c: Source (char)
 * @size: Length of destination buffer
 *
 * Buffers may overlap. Returns -1 on error.
 */
gint OBEX_CharToUnicode(guint8 *uc, const guint8 *c, gint size)
{
	gint len, n;
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

/**
 * OBEX_GetResponseMessage - Return a human understandable string from a response-code.
 * @self: OBEX handle
 * @rsp: Response code.
 *
 * The returned GString shall be freed by you. Returns %NULL on error.
 */
GString* OBEX_GetResponseMessage(obex_t *self, gint rsp)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, NULL);
	return obex_get_response_message(self, rsp);
}

/* ---------------------------------------------------------------- */

/**
 * InOBEX_ServerRegister - Start listening for incoming connections
 * @self: OBEX handle
 *
 * An easier server function to use for TCP/IP (InOBEX) only.
 *
 * Returns -1 on error.
 */
gint InOBEX_ServerRegister(obex_t *self)
{
	DEBUG(3, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);

	inobex_prepare_listen(self);
	return obex_transport_listen(self);
}

/**
 * InOBEX_TransportConnect - Connect Inet transport
 * @self: OBEX handle
 *
 * An easier connect function to use for TCP/IP (InOBEX) only.
 *
 * Note : I would like feedback on this API to know which input
 * parameter make most sense. Thanks...
 */
gint InOBEX_TransportConnect(obex_t *self, struct sockaddr *saddr, int addrlen)
{
     	DEBUG(4, G_GNUC_FUNCTION "()\n");

	if (self->object)	{
		DEBUG(1, G_GNUC_FUNCTION "() We are busy.\n");
		return -EBUSY;
	}

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(saddr != NULL, -1);

	inobex_prepare_connect(self, saddr, addrlen);
	return obex_transport_connect_request(self);
}

/**
 * IrOBEX_ServerRegister - Start listening for incoming connections
 * @self: OBEX handle
 * @service: Service to bind to.
 *
 * An easier server function to use for IrDA (IrOBEX) only.
 *
 * Returns -1 on error.
 */
gint IrOBEX_ServerRegister(obex_t *self, const char *service)
{
	DEBUG(3, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(service != NULL, -1);

#ifdef HAVE_IRDA
	irobex_prepare_listen(self, service);
	return obex_transport_listen(self);
#else
	return -ESOCKTNOSUPPORT;
#endif /* HAVE_IRDA */
}

/**
 * IrOBEX_TransportConnect - Connect Irda transport
 * @self: OBEX handle
 * @service: IrIAS service name to connect to
 *
 * An easier connect function to use for IrDA (IrOBEX) only.
 */
gint IrOBEX_TransportConnect(obex_t *self, const char *service)
{
     	DEBUG(4, G_GNUC_FUNCTION "()\n");

	if (self->object)	{
		DEBUG(1, G_GNUC_FUNCTION "() We are busy.\n");
		return -EBUSY;
	}

	g_return_val_if_fail(self != NULL, -1);

#ifdef HAVE_IRDA
	irobex_prepare_connect(self, service);
	return obex_transport_connect_request(self);
#else
	return -ESOCKTNOSUPPORT;
#endif /* HAVE_IRDA */
}


/**
 * BtOBEX_ServerRegister - Start listening for incoming connections
 * @self: OBEX handle
 * @service: Service to bind to. **FIXME**
 *
 * An easier server function to use for Bluetooth (Bluetooth OBEX) only. 
 *
 * Returns -1 on error.
 */
gint BtOBEX_ServerRegister(obex_t *self, bdaddr_t *src, int channel)
{
	DEBUG(3, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);

#ifdef HAVE_BLUETOOTH
	if(src == NULL)
		src = BDADDR_ANY;
	btobex_prepare_listen(self, src, channel);
	return obex_transport_listen(self);
#else
	return -ESOCKTNOSUPPORT;
#endif /* HAVE_BLUETOOTH */
}

/**
 *  BtOBEX_TransportConnect - Connect Bluetooth transport
 *  @self: OBEX handle
 *  @service: IrIAS service name to connect to **FIXME**
 *
 *  An easier connect function to use for Bluetooth (Bluetooth OBEX) only. 
 */
gint BtOBEX_TransportConnect(obex_t *self, bdaddr_t *src, bdaddr_t *dst, int channel)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	if (self->object)	{
		DEBUG(1, G_GNUC_FUNCTION "() We are busy.\n");
		return -EBUSY;
	}

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(dst != NULL, -1);

#ifdef HAVE_BLUETOOTH
	if(src == NULL)
		src = BDADDR_ANY;
	btobex_prepare_connect(self, src, dst, channel);
	return obex_transport_connect_request(self);
#else
	return -ESOCKTNOSUPPORT;
#endif /* HAVE_BLUETOOTH */
}
