/*********************************************************************
 *                
 * Filename:      obex_object.h
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Fri Apr 23 14:05:28 1999
 * Modified at:   Sun Dec  5 15:36:22 1999
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

#ifndef OBEX_OBJECT_H
#define OBEX_OBJECT_H

#include "obex_main.h"
#include "netbuf.h"

/* If an object has no expected length we have to 
reallocated every OBEX_OBJECT_ALLOCATIONTRESHOLD bytes */
#define OBEX_OBJECT_ALLOCATIONTRESHOLD 1024

typedef struct {
        time_t time;

	GSList *tx_headerq;		/* List of headers to transmit*/
	GSList *rx_headerq;		/* List of received headers */
	GSList *rx_lasthdr;		/* Last read element */
	GNetBuf *rx_body;		/* The rx body header need some extra help */
	GNetBuf *tx_nonhdr_data;	/* Data outside of headers (like CONNECT and SETPATH) */
	GNetBuf *rx_nonhdr_data;	/* -||- */
	gboolean app_is_called;		/* Set after an app has been called */
	guint8 cmd;			/* Command for normal packets */
	guint8 lastcmd;			/* Command for last packet */
	guint8 mode;			/* OBEX_RSP or OBEX_CMD */
	guint headeroffset;		/* Where to start parsing headers */

	gint hinted_body_len;		/* Hinted body-length or 0 */
	gint totallen;			/* Size of all headers */
	gpointer userdata;		/* Up to the user */

} obex_object_t;

struct obex_header_element {
	GNetBuf *buf;
	guint8 hi;
	guint length;
	guint offset;
};

obex_object_t *obex_object_new(void);
gint obex_object_delete(obex_object_t *object);
gint obex_object_addheader(obex_t *self, obex_object_t *object, guint8 hi,
				obex_headerdata_t hv, guint32 hv_size,
				guint flags);
gint obex_object_getnextheader(obex_t *self, obex_object_t *object, guint8 *hi,
				obex_headerdata_t *hv, guint32 *hv_size);
gint obex_object_setcmd(obex_object_t *object, guint8 cmd, guint8 lastcmd);
gint obex_object_setrsp(obex_object_t *object, guint8 cmd, guint8 lastcmd);
gint obex_object_send(obex_t *self, obex_object_t *object, gint allowfinish);
gint obex_object_receive(obex_object_t *object, GNetBuf *msg);

#endif
