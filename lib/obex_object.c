/*********************************************************************
 *                
 * Filename:      obex_object.c
 * Version:       0.5
 * Description:   OBEX object related functions
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Fri Apr 23 14:04:29 1999
 * Modified at:   Sun Dec  5 15:35:42 1999
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

#include "config.h"
//#include <assert.h>

#include <string.h>

#include "obex_main.h"
#include "obex_object.h"
#include "obex_header.h"
#include "obex_connect.h"

/*
 * Function obex_object_new (name, description)
 *
 *    Create a new OBEX object
 *
 */
obex_object_t *obex_object_new(void)
{
	obex_object_t *object;

     	object = g_malloc0(sizeof(obex_object_t));
	if (!object)
		return NULL;

	object->app_is_called = FALSE;
	obex_object_setrsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);

	return object;
}

/*
 * Function obex_object_delete (object)
 *
 *    Delete OBEX object
 *
 */
gint obex_object_delete(obex_object_t *object)
{
	struct obex_header_element *h;

	DEBUG(4, G_GNUC_FUNCTION "()\n");
	g_return_val_if_fail(object != NULL, -1);

	//Free TX headers
	while(object->tx_headerq)	{
	 	h = object->tx_headerq->data;
		g_netbuf_free(h->buf);
		object->tx_headerq = g_slist_remove(object->tx_headerq, h);
		g_free(h);
	}

	//Free RX headers
	while(object->rx_headerq)	{
	 	h = object->rx_headerq->data;
		g_netbuf_free(h->buf);
		object->rx_headerq = g_slist_remove(object->rx_headerq, h);
		g_free(h);
	}

	g_netbuf_free(object->rx_body);
	object->rx_body = NULL;

	g_netbuf_free(object->tx_nonhdr_data);
	object->tx_nonhdr_data = NULL;

	g_netbuf_free(object->rx_nonhdr_data);
	object->rx_nonhdr_data = NULL;
	
	g_free(object);

	return 0;
}


/*
 * Function obex_object_setcmd ()
 *
 *    Set command of object
 *
 */
gint obex_object_setcmd(obex_object_t *object, guint8 cmd, guint8 lastcmd)
{
	DEBUG(4,G_GNUC_FUNCTION "() %02x\n", cmd);
	object->cmd = cmd;
	object->lastcmd = lastcmd;
	object->mode = OBEX_CMD;
	return 1;
}

/*
 * Function obex_object_setrsp ()
 *
 *    Set the response for an object
 *
 */
gint obex_object_setrsp(obex_object_t *object, guint8 cmd, guint8 lastcmd)
{
	DEBUG(4,G_GNUC_FUNCTION "()\n");
	object->cmd = cmd;
	object->lastcmd = lastcmd;
	object->mode = OBEX_RSP;
	return 1;
}

/*
 * Function gint obex_object_addheader(obex_object_t *object, guint8 hi, 
 *                      obex_headerdata_t hv, guint32 hv_size, guint flags)
 *
 *    Add a header to an object for sending later on.
 *
 */
gint obex_object_addheader(obex_t *self, obex_object_t *object, guint8 hi,
	obex_headerdata_t hv, guint32 hv_size, guint flags)
{
	gint ret = -1;
	struct obex_header_element *element;
	guint maxlen;

	if(flags & OBEX_FL_FIT_ONE_PACKET)	{
		/* In this command all headers must fit in one packet! */
		DEBUG(3, G_GNUC_FUNCTION "() Fit one packet!\n");
		maxlen = self->mtu_tx - object->totallen - sizeof(struct obex_common_hdr);
	}
	else	{
		maxlen = self->mtu_tx - sizeof(struct obex_common_hdr);
	}


	element = g_malloc0(sizeof(struct obex_header_element));
	if(!element)
		return -1;

	switch (hi & OBEX_HI_MASK) {
	case OBEX_INT:
		DEBUG(2, G_GNUC_FUNCTION "() 4BQ header %d\n", hv.bq4);
		
		element->buf = g_netbuf_new(sizeof(struct obex_uint_hdr));
		if(element->buf) {
			element->length = (guint) sizeof(struct obex_uint_hdr);
			element->hi = hi;
			object->totallen += insert_uint_header(element->buf, hi, hv.bq4);
			ret = 1;
		}
		break;
		
	case OBEX_BYTE:
		DEBUG(2, G_GNUC_FUNCTION "() 1BQ header %d\n", hv.bq1);

		element->buf = g_netbuf_new(sizeof(struct obex_ubyte_hdr));
		if(element->buf) {
			element->length = sizeof(struct obex_ubyte_hdr);
			element->hi = hi;
			object->totallen += insert_ubyte_header(element->buf, hi, hv.bq1);
			ret = 1;
		}
	break;

	case OBEX_BYTE_STREAM:
		DEBUG(2, G_GNUC_FUNCTION "() BS  header size %d\n", hv_size);

		element->buf = g_netbuf_new(hv_size + sizeof(struct obex_byte_stream_hdr) );
		if(element->buf) {
			element->length = hv_size + sizeof(struct obex_byte_stream_hdr);
			element->hi = hi;
			object->totallen += insert_byte_stream_header(element->buf, hi, hv.bs, hv_size);
			ret = 1;
		}
	break;
	case OBEX_UNICODE:
		DEBUG(2, G_GNUC_FUNCTION "() Unicode header size %d\n", hv_size);

		element->buf = g_netbuf_new(hv_size + sizeof(struct obex_unicode_hdr) );
		if(element->buf) {
			element->length = hv_size + sizeof(struct obex_unicode_hdr);
			element->hi = hi;
			object->totallen += insert_unicode_header(element->buf, hi, hv.bs, hv_size);
			ret = 1;
		}
	break;
	default:
		DEBUG(2, G_GNUC_FUNCTION "() Unsupported encoding %02x\n", hi & OBEX_HI_MASK);
		break;
	}

	/* Check if you can send this header without violating MTU or OBEX_FIT_ONE_PACKET */
	if( (element->hi != OBEX_HDR_BODY) || (flags & OBEX_FL_FIT_ONE_PACKET) )	{
		if(maxlen < element->length)	{
			DEBUG(1, G_GNUC_FUNCTION "() Header to big\n");
			g_free(element->buf);
			g_free(element);
			ret = -1;
		}
	}

	if(ret > 0)
		object->tx_headerq = g_slist_append(object->tx_headerq, element);
	else {
		g_free(element);
	}
	
	return ret;
}

/* 
 * Function obex_object_send()
 *
 *    Send away all headers attached to an object. Returns:
 *       1 on sucessfully done
 *       0 on progress made
 *     < 0 on error
 */
gint obex_object_send(obex_t *self, obex_object_t *object, gint allowfinish)
{
	struct obex_header_element *h;
	GNetBuf *txmsg;
	gint actual, finished = 0;
	gint lastcmd;
	guint16 tx_left;
	gboolean addmore = TRUE;
	struct obex_byte_stream_hdr *body_txh;

	/* Calc how many bytes of headers we can fit in this package */
	tx_left = self->mtu_tx - sizeof(struct obex_common_hdr);

	DEBUG(4, G_GNUC_FUNCTION "()\n");

	/* Automagical final-handling */
	if(allowfinish)
		lastcmd = object->lastcmd;
	else
		lastcmd = object->cmd;
	
	DEBUG(4, G_GNUC_FUNCTION "() Allowfinish = %d\n", allowfinish);

	/* Reuse transmit buffer */
	txmsg = g_netbuf_recycle(self->tx_msg);
	
	/* Reserve space for common header */
	g_netbuf_reserve(txmsg, sizeof(obex_common_hdr_t));

	/* Check if this command has some nonheader data to send */
	if(object->tx_nonhdr_data) {
		DEBUG(4, G_GNUC_FUNCTION "() Adding %d bytes of non-headerdata to tx buffer\n", object->tx_nonhdr_data->len);
		g_netbuf_put_data(txmsg, object->tx_nonhdr_data->data, object->tx_nonhdr_data->len);
		g_netbuf_free(object->tx_nonhdr_data);
		object->tx_nonhdr_data = NULL;
	}



	/* This loop takes params from the tx queue and tries to stuff as
	   many as possile into the send-buffer */
	while (addmore)	{
		/* Stop when we have no headers to send */
		if(! object->tx_headerq)
			break;

		h =  object->tx_headerq->data;

		body_txh = (struct obex_byte_stream_hdr*) txmsg->tail;

		/* The body may be sent over several packets. Take care of this! */
		if(h->hi == OBEX_HDR_BODY) {
			/* Remove the header from the buffer. We will add it later
			   in all fragments of the body */
			if(! g_netbuf_headroom(h->buf))	{
				DEBUG(4, G_GNUC_FUNCTION "() Removing old body-header\n");
				g_netbuf_pull(h->buf,  sizeof(struct obex_byte_stream_hdr) );
			}
		
			if(tx_left < ( h->buf->len + 
					sizeof(struct obex_byte_stream_hdr) ) )	{
				DEBUG(4, G_GNUC_FUNCTION "() Add BODY header\n");
				body_txh->hi = OBEX_HDR_BODY;
				body_txh->hl = htons((guint16)tx_left);

				g_netbuf_put(txmsg, sizeof(struct obex_byte_stream_hdr) );
				g_netbuf_put_data(txmsg, h->buf->data, tx_left
						- sizeof(struct obex_byte_stream_hdr) );

				g_netbuf_pull(h->buf, tx_left
						- sizeof(struct obex_byte_stream_hdr) ); 
				/* We have completely filled the tx-buffer */
				addmore = FALSE;
			}
			else	{
				DEBUG(4, G_GNUC_FUNCTION "() Add BODY_END header\n");

				body_txh->hi = OBEX_HDR_BODY_END;
				body_txh->hl = htons((guint16) (h->buf->len + sizeof(struct obex_byte_stream_hdr)));
				g_netbuf_put(txmsg, sizeof(struct obex_byte_stream_hdr) );

				g_netbuf_put_data(txmsg, h->buf->data, h->buf->len);
				object->tx_headerq = g_slist_remove(object->tx_headerq, h);
				g_netbuf_free(h->buf);
				g_free(h);
			}
		}
		else	{
			DEBUG(4, G_GNUC_FUNCTION "() Add NON-BODY header\n");
			if(h->length <= tx_left) {
				object->tx_headerq = g_slist_remove(object->tx_headerq, h);
				g_netbuf_put_data(txmsg, h->buf->data, h->length);
				g_netbuf_free(h->buf);
				tx_left -= h->length;
				g_free(h);
			}
			else if(h->length > self->mtu_tx) {
				DEBUG(1, G_GNUC_FUNCTION "() ERROR! Non-body header to big for MTU. Skipping it\n");
				/* If this happens we just skip the header.
				   OBEX_ObjectAddHeader() have made this impossible */
				object->tx_headerq = g_slist_remove(object->tx_headerq, h);
			}
			else	{
				/* This header won't fit. Now go send what we have! */
				addmore = FALSE;
			}
		}
	}


	if(object->tx_headerq) {
		DEBUG(4, G_GNUC_FUNCTION "() Sending non-last package\n");
		actual = obex_data_request(self, txmsg, object->cmd, object->mode);
		finished = 0;
	}
	else {
		DEBUG(4, G_GNUC_FUNCTION "() Sending final package\n");
		actual = obex_data_request(self, txmsg, lastcmd | OBEX_FINAL, object->mode);
		finished = allowfinish;
	}

	if(actual < 0) {
		DEBUG(4, G_GNUC_FUNCTION "() Send error\n");
		return actual;
	}
	else {
		return finished;
	}
}


gint obex_object_getnextheader(obex_t *self, obex_object_t *object, guint8 *hi,
					obex_headerdata_t *hv,
					guint32 *hv_size)
{
	guint32 *bq4;
	struct obex_header_element *h;

	DEBUG(4, G_GNUC_FUNCTION "()\n");

	/* Return if no headers */
	if(object->rx_lasthdr == NULL)
		return 0;

	h = object->rx_lasthdr->data;


	*hi = h->hi;
	*hv_size= h->length;

	switch (h->hi & OBEX_HI_MASK) {
		case OBEX_BYTE_STREAM:
			hv->bs = &h->buf->data[0];
			break;

		case OBEX_UNICODE:
			hv->bs = &h->buf->data[0];
			break;

		case OBEX_INT:
			bq4 = (guint32*) h->buf->data;
			hv->bq4 = ntohl(*bq4);
			break;

		case OBEX_BYTE:
			hv->bq1 = h->buf->data[0];
			break;
	}
	object->rx_lasthdr = g_slist_next(object->rx_lasthdr);
	return 1;
}

gint obex_object_receive_body(obex_object_t *object, GNetBuf *msg, guint8 hi, guint8 *source, guint len)
{
	struct obex_header_element *element;

	DEBUG(4, G_GNUC_FUNCTION "() This is a body-header. Len=%d\n", len);


	if(len > msg->len)	{
		DEBUG(1, G_GNUC_FUNCTION "() Header %d to big. HSize=%d Buffer=%d\n", hi,
						len,
						msg->len);
		return -1;
	}

	if(!object->rx_body)	{
		gint alloclen = OBEX_OBJECT_ALLOCATIONTRESHOLD + len;
		if(object->hinted_body_len)
			alloclen = object->hinted_body_len;

		DEBUG(4, G_GNUC_FUNCTION "() Allocating new body-buffer. Len=%d\n", alloclen);
		if(! (object->rx_body = g_netbuf_new(alloclen)))
			return -1;
	}

	/* Reallocate body-netbuf if needed */ 
	if(g_netbuf_tailroom(object->rx_body) < (gint)len)	{
		DEBUG(4, G_GNUC_FUNCTION "() Buffer too small. Go realloc\n");
		if(! (object->rx_body = g_netbuf_realloc(object->rx_body,
				object->rx_body->truesize + OBEX_OBJECT_ALLOCATIONTRESHOLD + len) ) )	{
			DEBUG(1, G_GNUC_FUNCTION "() Can't realloc rx_body\n");
			return -1;
			// FIXME: Handle this in a nice way...
		}
	}

	g_netbuf_put_data(object->rx_body, source, len);

	if(hi == OBEX_HDR_BODY_END)	{
		DEBUG(4, G_GNUC_FUNCTION "() Body receive done\n");
		if( (element = g_malloc0(sizeof(struct obex_header_element)) ) ) {
			element->length = object->rx_body->len;
			element->hi = OBEX_HDR_BODY;
			element->buf = object->rx_body;

			/* Add element to rx-list */
			object->rx_headerq = g_slist_append(object->rx_headerq, element);
			object->rx_lasthdr = object->rx_headerq;

		}
		else	{
			g_netbuf_free(object->rx_body);
		}
		
		object->rx_body = NULL;
	}
	else	{
		DEBUG(4, G_GNUC_FUNCTION "() Normal body fragment...\n");
	}
	return 1;
}


/* 
 * Function obex_object_receive()
 *
 *    Add any incoming headers to headerqueue.
 *
 */
gint obex_object_receive(obex_object_t *object, GNetBuf *msg)
{
	struct obex_header_element *element;
	guint8 *source = NULL;
	guint len, hlen;
	guint8 hi;
	gint err = 0;

	union {
		struct obex_unicode_hdr     *unicode;
		struct obex_byte_stream_hdr *bstream;
		struct obex_uint_hdr	    *uint;
	} h;


	DEBUG(4, G_GNUC_FUNCTION "()\n");

	/* Remove command from buffer */
	g_netbuf_pull(msg, sizeof(struct obex_common_hdr));

	/* Copy any non-header data (like in CONNECT and SETPATH) */
	if(object->headeroffset) {
		object->rx_nonhdr_data = g_netbuf_new(object->headeroffset);
		if(!object->rx_nonhdr_data)
			return -1;
		g_netbuf_put_data(object->rx_nonhdr_data, msg->data, object->headeroffset);
		DEBUG(4, G_GNUC_FUNCTION "() Command has %d bytes non-headerdata\n", object->rx_nonhdr_data->len);
		g_netbuf_pull(msg, object->headeroffset);
		object->headeroffset = 0;
	}

	while ((msg->len > 0) && (!err)) {
		hi = msg->data[0];
		DEBUG(4, G_GNUC_FUNCTION "() Header: %02x\n", hi);
		switch (hi & OBEX_HI_MASK) {

		case OBEX_UNICODE:
			h.unicode = (struct obex_unicode_hdr *) msg->data;
			source = &msg->data[3];
			hlen = ntohs(h.unicode->hl);
			len = hlen - 3;
			break;

		case OBEX_BYTE_STREAM:
			h.bstream = (struct obex_byte_stream_hdr *) msg->data;
			source = &msg->data[3];
			hlen = ntohs(h.bstream->hl);
			len = hlen - 3;

			if( (hi == OBEX_HDR_BODY) ||
				(hi == OBEX_HDR_BODY_END) ) {
				/* The body-header need special treatment */
				if(obex_object_receive_body(object, msg, hi, source, len) < 0)

					err = -1;
				source = NULL;
			}
			break;

		case OBEX_BYTE:
			source = &msg->data[1];
			len = 1;
			hlen = 2;
			break;

		case OBEX_INT:
			source = &msg->data[1];
			len = 4;
			hlen = 5;
			break;
		default:
			DEBUG(1, G_GNUC_FUNCTION "() Badly formed header received\n");
			source = NULL;
			hlen = 0;
			len = 0;
			err = -1;
			break;
		}

		/* Make sure that the msg is big enough for header */
		if(len > msg->len)	{
			DEBUG(1, G_GNUC_FUNCTION "() Header %d to big. HSize=%d Buffer=%d\n", hi,
						len,
						msg->len);
			source = NULL;
			err = -1;
		}

		if(source) {
			/* The length MAY be useful when receiving body. */
			if(hi == OBEX_HDR_LENGTH)	{
				h.uint = (struct obex_uint_hdr *) msg->data;
				object->hinted_body_len = ntohl(h.uint->hv);
				DEBUG(4, G_GNUC_FUNCTION "() Hinted body len is %d\n",
							object->hinted_body_len);
			}

			if( (element = g_malloc0(sizeof(struct obex_header_element)) ) ) {
				if( (element->buf = g_netbuf_new(len)) )	{
					DEBUG(4, G_GNUC_FUNCTION "() Copying %d bytes\n", len);
					element->length = len;
					element->hi = hi;
					g_netbuf_put_data(element->buf, source, len);

					/* Add element to rx-list */
					object->rx_headerq = g_slist_append(object->rx_headerq, element);
					object->rx_lasthdr = object->rx_headerq;
				}
				else	{
					DEBUG(1, G_GNUC_FUNCTION "() Cannot allocate memory\n");
					g_free(element);
					err = -1;
				}
			}
			else {
				DEBUG(1, G_GNUC_FUNCTION "() Cannot allocate memory\n");
				err = -1;
			}
		}

		if(err)
			return err;

		DEBUG(4, G_GNUC_FUNCTION "() Pulling %d bytes\n", hlen);
		g_netbuf_pull(msg, hlen);
	}

	return 1;
}
