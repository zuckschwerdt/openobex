/*********************************************************************
 *                
 * Filename:      obex_main.c
 * Version:       0.5
 * Description:   Implementation of the Object Exchange Protocol OBEX
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Fri Jul 17 23:02:02 1998
 * Modified at:   Sun Aug 13 09:13:58 PM CEST 2000
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
#ifdef _WIN32
#include <winsock.h>
#else /* _WIN32 */

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#endif /* _WIN32 */

#include "obex_main.h"
#include "obex_header.h"
#include "obex_server.h"
#include "obex_client.h"
#include "obex_const.h"

#ifdef HAVE_FASYNC
obex_t *async_self[OBEX_MAXINSTANCE] = {NULL, }; /* Which instances wants SIGIO */
pid_t async_pid[OBEX_MAXINSTANCE] = {0, };
#endif

/*
 * Function obex_register_async()
 *
 *    
 *
 */
gint obex_register_async(obex_t *self, gint fd)
{
#ifdef HAVE_FASYNC
	gint oflags;
	gint i;
	gboolean ok = FALSE;
	pid_t curr_pid;

	DEBUG(4, G_GNUC_FUNCTION "()\n");
	curr_pid = getpid();

	/* Register for asynchronous notification */
	signal(SIGIO, &obex_input_handler);
	fcntl(fd, F_SETOWN, getpid());
	oflags = fcntl(0, F_GETFL);
	if (fcntl(fd, F_SETFL, oflags | FASYNC) < 0) {
		return -1;
	}

	/* Insert handler in async-array */
	for(i=0; i<OBEX_MAXINSTANCE ;i++) {
		if(async_self[i] == NULL) {
			async_self[i] = self;
			async_pid[i] = curr_pid;
			ok = TRUE;
			break;
		}
	}
	if(!ok)
		return -1;
	return self->fd;
#else
	return -1;
#endif
}


/*
 * Function obex_create_socket()
 *
 *    Create socket if needed.
 *
 */
gint obex_create_socket(obex_t *self, gint domain, gboolean async)
{
	gint fd;
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	fd = socket(domain, SOCK_STREAM, 0);
	if(fd < 0)
		return fd;

#ifdef HAVE_FASYNC
	if(async) {
		if(obex_register_async(self, fd) < 0)	{
			close(fd);
			fd = -1;
			return -1;
		}
	}
#endif
	return fd;
}

/*
 * Function obex_delete_socket()
 *
 *    Close socket if opened.
 *
 */
gint obex_delete_socket(obex_t *self, gint fd)
{
	gint ret;

	DEBUG(4, G_GNUC_FUNCTION "()\n");

	if(fd < 0)
		return fd;

#ifdef HAVE_FASYNC
	if(self->async) {
		gint i, cnt = 0;
		/* Remove handle from async-array */
		for(i=0; i<OBEX_MAXINSTANCE ;i++) {
			if(async_self[i] == self)
				async_self[i] = NULL;
			if(async_self[i])
				cnt++;
		}
		if(cnt == 0)
			signal(SIGIO, SIG_DFL);
	}
#endif

#ifdef _WIN32
	ret = closesocket(fd);
#else /* _WIN32 */
	ret = close(fd);
#endif /* _WIN32 */
	return ret;
}

/*
 * Function input_handler (signal)
 *
 *    Called by SIGIO and is used then the OBEX library is in asyncronous mode
 *
 */
void obex_input_handler(int signal)
{
#ifdef HAVE_FASYNC
	gint i;
	pid_t curr_pid;
	
	DEBUG(4, G_GNUC_FUNCTION "() Got some input!\n");
	curr_pid = getpid();

	for(i=0; i<OBEX_MAXINSTANCE ;i++) {
		if( (async_self[i]) && (async_pid[i] == curr_pid) )	{
			obex_data_indication(async_self[i], NULL, 0);
		}
	}
#endif
}

/*
 * Function obex_get_last_response_message (self)
 *
 *    Return a GSting of an OBEX-response
 *
 */
GString *obex_get_response_message(obex_t *self, gint rsp)
{
	GString *string;

	g_return_val_if_fail(self != NULL, NULL);

	switch (rsp) {
	case OBEX_RSP_CONTINUE:
		string = g_string_new("Continue");
		break;
	case OBEX_RSP_SWITCH_PRO:
		string = g_string_new("Switching protocols");
		break;
	case OBEX_RSP_SUCCESS:
		string = g_string_new("OK, Success");
		break;
	case OBEX_RSP_CREATED:
		string = g_string_new("Created");
		break;
	case OBEX_RSP_ACCEPTED:
		string = g_string_new("Accepted");
		break;	
	case OBEX_RSP_NO_CONTENT:
		string = g_string_new("No Content");
		break;
	case OBEX_RSP_BAD_REQUEST:
		string = g_string_new("Bad Request");
		break;
	case OBEX_RSP_UNAUTHORIZED:
		string = g_string_new("Unautorized");
		break;
	case OBEX_RSP_PAYMENT_REQUIRED:
		string = g_string_new("Payment required");
		break;
	case OBEX_RSP_FORBIDDEN:
		string = g_string_new("Forbidden");
		break;
	case OBEX_RSP_NOT_FOUND:
		string = g_string_new("Not found");
		break;
	case OBEX_RSP_METHOD_NOT_ALLOWED:
		string = g_string_new("Method not allowed");
		break;
	case OBEX_RSP_CONFLICT:
		string = g_string_new("Conflict");
		break;
	case OBEX_RSP_INTERNAL_SERVER_ERROR:
		string = g_string_new("Internal server error");
		break;
	case OBEX_RSP_NOT_IMPLEMENTED:
		string = g_string_new("Not implemented!");
		break;
	case OBEX_RSP_DATABASE_FULL:
		string = g_string_new("Database full");
		break;
	case OBEX_RSP_DATABASE_LOCKED:
		string = g_string_new("Database locked");
		break;
	default:
		string = g_string_new("Unknown response");
		break;
	}
	return string;
}

/*
 * Function obex_deliver_event ()
 *
 *    Deliver an event to app.
 *
 */
void obex_deliver_event(obex_t *self, gint mode, gint event, gint cmd, gint rsp, gboolean del)
{
	self->eventcb(self, self->object, mode, event, cmd, rsp);
	if(del) {
		if(self->object)	{
			obex_object_delete(self->object);
			self->object = NULL;
		}
		self->response_next = FALSE;
	}
}


/*
 * Function obex_response_request (self, opcode)
 *
 *    Send a response to peer device
 *
 */
void obex_response_request(obex_t *self, guint8 opcode)
{
	GNetBuf *msg;

	g_return_if_fail(self != NULL);

	msg = g_netbuf_recycle(self->tx_msg);
	g_netbuf_reserve(msg, sizeof(obex_common_hdr_t));

	obex_data_request(self, msg, opcode | OBEX_FINAL, OBEX_RSP);
}

/*
 * Function obex_data_request (self, opcode, cmd)
 *
 *    Send response or command code along with optional headers/data.
 *
 */
gint obex_data_request(obex_t *self, GNetBuf *msg, int opcode, int cmd)
{
	obex_common_hdr_t *hdr;
	int actual = 0;

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(msg != NULL, -1);

	if (cmd == OBEX_CMD) {
		/* This is a half duplex protocol, so remember the last cmd */
		/* self->last_cmd = opcode & ~OBEX_FINAL; */

		/* Next indication will/should be a response */
		self->response_next = TRUE;
	} else
		self->response_next = FALSE;

	DEBUG(4, G_GNUC_FUNCTION "(), self->response_next=%d\n", 
	      self->response_next);

	/* Insert common header */
	hdr = (obex_common_hdr_t *) g_netbuf_push(msg, sizeof(obex_common_hdr_t));

	hdr->opcode = opcode;
	hdr->len = htons((guint16)msg->len);

#if DEBUG_DUMPBUFFERS & 1
	g_netbuf_print(msg);
#endif
	DEBUG(1, G_GNUC_FUNCTION "(), len = %d bytes\n", msg->len);

	actual = obex_transport_write(self, msg);
	return actual;
}

/*
 * Function obex_data_indication (self)
 *
 *    Read/Feed some input from device and find out which packet it is
 *
 */
gint obex_data_indication(obex_t *self, guint8 *buf, gint buflen)
{
	obex_common_hdr_t *hdr;
	GNetBuf *msg;
	gint final;
	gint actual;
	guint8 opcode;
	guint size;

	DEBUG(4, G_GNUC_FUNCTION "()\n");

	g_return_val_if_fail(self != NULL, -1);


	actual = obex_transport_read(self, buf, buflen);
	msg = self->rx_msg;

	DEBUG(1, G_GNUC_FUNCTION "(), got %d bytes\n", actual);

	/* Check if we are still connected */
	if (actual <= 0)	{
		obex_deliver_event(self, 0, OBEX_EV_LINKERR, 0, 0, TRUE);
		return actual;
	}
	/* New data has been inserted at the end of message */
	g_netbuf_put(msg, actual);
	DEBUG(4, G_GNUC_FUNCTION "(), msg len=%d\n", msg->len);

	hdr = (obex_common_hdr_t *) msg->data;

	/* We must have at least 3 bytes of a package to know the size */
	if(actual < 3)	{
		DEBUG(3, G_GNUC_FUNCTION "(), Need MUCH more data, len=%d!\n", msg->len);
		return actual;
	}	

#if DEBUG_DUMPBUFFERS & 2
	g_netbuf_print(msg);
#endif

	/*  
	 * Make sure that the buffer we have, actually has the specified
	 * number of bytes. If not the frame may have been fragmented, and
	 * we will then need to read more from the socket.  
	 */

	DEBUG(4,G_GNUC_FUNCTION "() hdr->len = %04x\n", hdr->len);

	size = ntohs(hdr->len);
	if (size > msg->len) {
		DEBUG(3, G_GNUC_FUNCTION "() Need more data, size=%d, len=%d!\n",
		      size, msg->len);

		/* I'll be back! */
		return actual;
	}

#if DEBUG_DUMPBUFFERS & 2
	g_netbuf_print(msg);
#endif

	final = hdr->opcode & OBEX_FINAL; /* Extract final bit */

	/* Check if this is a response */
	if (self->response_next) {
		DEBUG(3, G_GNUC_FUNCTION "() Expecting response\n");
		obex_client(self, msg, final);
		g_netbuf_recycle(msg);
	}
	else	{
		opcode = hdr->opcode & ~OBEX_FINAL; /* Remove final bit */
		obex_server(self, msg, final);
		g_netbuf_recycle(msg);
	}

	return actual;
}
