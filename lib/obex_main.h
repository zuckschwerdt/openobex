/*********************************************************************
 *                
 * Filename:      obex_main.h
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Mon Jul 20 22:28:23 1998
 * Modified at:   Fri Nov 19 16:02:18 1999
 * Modified by:   Pontus Fuchs <pontus@tactel.se>
 * 
 *     Copyright (c) 1998, 1999, Dag Brattli, All Rights Reserved.
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

#ifndef OBEX_MAIN_H
#define OBEX_MAIN_H

#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>

/* Forward decl */
struct obex;
typedef struct obex obex_t;

#include <obex_const.h>
#include "obex_object.h"
#include "obex_transport.h"
#include "netbuf.h"

/* use 0 for production, 1 for sendbuff, 2 for receivebuff and 3 for both */
#ifndef DEBUG_DUMPBUFFERS
#define DEBUG_DUMPBUFFERS 0
#endif

/* use 0 for production, 1 for verification, >2 for debug */
#ifndef NET_DEBUG
//#define NET_DEBUG 4
#endif
#ifdef NET_DEBUG
unsigned int obex_net_debug;
#define DEBUG(n, args...) if (obex_net_debug >= (n)) g_print(args)
#else
#define DEBUG(n, args...)
#endif /* NET_DEBUG */

#define OBEX_VERSION		0x11      /* Version 1.1 */
#define OBEX_DEFAULT_MTU	1024
#define OBEX_MINIMUM_MTU	255      

#define OBEX_CMD	1
#define OBEX_RSP	0

#define OBEX_MAXINSTANCE 16

typedef void (*obex_event_t)(obex_object_t *obj, gint mode, gint event, gint obex_cmd, gint obex_rsp);

struct obex {
	guint16 mtu_tx;			/* Maximum OBEX Tx packet size */
        guint16 mtu_rx;			/* Maximum OBEX Rx packet size */

	gint fd;			/* Socket descriptor */

	gboolean async;

	GNetBuf *tx_msg;		/* Reusable transmit message */
	GNetBuf *rx_msg;		/* Reusable receive message */

	obex_object_t	*object;	/* Current object being transfered */      
	obex_event_t	eventcb;	/* Event-callback */

	volatile int response_next;	/* Next indication a response? */
	gint lastcmd;

	obex_transport_t trans;		/* Transport being used */
	obex_ctrans_t ctrans;
	gpointer userdata;		/* For user */
};

gint obex_register_async(obex_t *self, gint fd);
gint obex_create_socket(obex_t *self, gint domain, gboolean async);
gint obex_delete_socket(obex_t *self);

void obex_deliver_event(obex_t *self, gint mode, gint event, gint cmd, gint rsp, gboolean del);
void obex_input_handler(int signal);
gint obex_data_indication(obex_t *self, guint8 *buf, gint buflen);

gint obex_data_request(obex_t *self, GNetBuf *msg, gint opcode, 
		       gint response);

void obex_response_request(obex_t *self, guint8 opcode);

GString *obex_get_response_message(obex_t *self, gint rsp);

gint obex_transport_connect_request(obex_t *self);
void obex_transport_disconnect_request(obex_t *self);
gint obex_transport_listen(obex_t *self, char *service);
gint obex_transport_write(obex_t *self, GNetBuf *msg);
gint obex_transport_read(obex_t *self, guint8 *buf, gint buflen);

extern obex_t *async_self[OBEX_MAXINSTANCE];


#endif
