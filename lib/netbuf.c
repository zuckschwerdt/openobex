/*********************************************************************
 *                
 * Filename:      netbuf.c
 * Version:       0.9
 * Description:   Network buffer handling routines. 
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Fri Mar 19 09:07:21 1999
 * Modified at:   Sat Oct 16 14:53:39 1999
 * Modified by:   Dag Brattli <dagb@cs.uit.no>
 * Sources:       skbuff.c by  Alan Cox <iiitac@pyr.swan.ac.uk> and
 *                             Florian La Roche <rzsfl@rz.uni-sb.de>
 * 
 *     Copyright (c) 1999 Dag Brattli, All Rights Reserved.
 *     
 *     This program is free software; you can redistribute it and/or 
 *     modify it under the terms of the GNU General Public License as 
 *     published by the Free Software Foundation; either version 2 of 
 *     the License, or (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License 
 *     along with this program; if not, write to the Free Software 
 *     Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *     MA 02111-1307 USA
 *     
 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include <netbuf.h>

/*
 * Function msg_recycle (msg)
 *
 *    Reuse an already used message. We just reset the state
 *
 */
GNetBuf *g_netbuf_recycle(GNetBuf *msg)
{
	msg->data = msg->head;
	msg->tail = msg->head;
	msg->len = 0;
	msg->end = msg->head + msg->truesize;

	return msg;
}

/*
 * Function g_netbuf_new (len)
 *
 *    Allocate new network buffer
 *
 */
GNetBuf *g_netbuf_new(guint len)
{
	GNetBuf *msg;
	gint8 *buf;
	
	msg = g_malloc(sizeof(GNetBuf));
	if (msg == NULL)
		return NULL;
	memset(msg, 0, sizeof(GNetBuf));
	
	buf = g_malloc(len);
	if (buf == NULL) {
		g_free(msg);
		return NULL;
	}
	
	/* Init */
	msg->truesize = len;
	msg->head = buf;
	
	g_netbuf_recycle(msg);
	
	return msg;
}

/*
 * Function g_netbuf_realloc (msg, len)
 *
 *    Change the true size of the message
 *
 */
GNetBuf *g_netbuf_realloc(GNetBuf *msg, guint len)
{
	gint8 *buf;

/* 	g_print("msg->head=%p\n", msg->head); */
/* 	g_print("msg->data=%p\n", msg->data); */
/* 	g_print("msg->tail=%p\n", msg->tail); */
/* 	g_print("msg->len=%d\n", msg->len); */
/* 	g_print("msg->truesize=%d\n\n", msg->truesize); */

	buf = realloc(msg->head, len);
	if (buf == NULL)
		return NULL;

	msg->truesize = len;
	msg->data = buf + (msg->data - msg->head);	
	msg->tail = buf + (msg->tail - msg->head);
	msg->head = buf;
	msg->end = msg->head + len;

/* 	g_print("msg->head=%p\n", msg->head); */
/* 	g_print("msg->data=%p\n", msg->data); */
/* 	g_print("msg->tail=%p\n", msg->tail); */
/* 	g_print("msg->len=%d\n", msg->len); */
/* 	g_print("msg->truesize=%d\n", msg->truesize); */

	return msg;
}

/*
 * Function g_netbuf_free (msg)
 *
 *    Free message
 *
 */
void g_netbuf_free(GNetBuf *msg)
{
	if (!msg)
		return;
	if (msg->head)
		g_free(msg->head);
	g_free(msg);
}

/*
 * Function g_netbuf_put (msg, len)
 *
 *    Make space for more data into message
 *
 */
gint8 *g_netbuf_put(GNetBuf *msg, guint len)
{
        gint8 *tmp = msg->tail;
        
	msg->tail += len;
        msg->len += len;
	
        if (msg->tail > msg->end) {
		g_print(__FUNCTION__ "(), put over, trying to realloc ...!\n");
		
		msg = g_netbuf_realloc(msg, msg->truesize+len);
		if (!msg)
			return NULL;

		tmp = msg->tail - len;
        }
        return tmp;
}

gint8 *g_netbuf_put_data(GNetBuf *msg, gint8 *data, guint len)
{
	gint8 *tmp;

	/* Make room for more data */
	tmp = g_netbuf_put(msg, len);

	/* Copy body data to object */
	memcpy(tmp, data, len);

	return tmp;
}

/*
 * Function g_netbuf_push (buf, len)
 *
 *    Insert new header in front of data
 *
 */
gint8 *g_netbuf_push(GNetBuf *msg, guint len)
{
	if ((msg->data - len) < msg->head) {
		g_print(__FUNCTION__ "(), pushed under, trying to realloc!\n");

		msg = g_netbuf_realloc(msg, msg->truesize+len);
		if (!msg)
			return NULL;
		
		/* Move data with offset len */
		g_memmove(msg->data+len, msg->data, msg->len);
		msg->data = msg->data+len;
		msg->tail = msg->tail+len;
	}

	msg->data -= len;
	msg->len += len;

	return msg->data;
}

/*
 * Function g_netbuf_prepend_hdr (msg, hdr, len)
 *
 *    
 *
 */
gint8 *g_netbuf_prepend_hdr(GNetBuf *msg, gint8 *hdr, guint len)
{
	gint8 *tmp;
	
	/* Make room for header */
	tmp = g_netbuf_push(msg, len);

	/* Copy body data to object */
	memcpy(tmp, hdr, len);

	return tmp;
}

/*
 * Function g_netbuf_pull (msg, len)
 *
 *    Remove header or data in front of the message
 *
 */
gint8 *g_netbuf_pull(GNetBuf *msg, guint len)
{
	if (len > msg->len)
                return NULL;
	
	msg->len -= len;
        return msg->data += len;
}

/*
 * Function g_netbuf_reserve (msg, len)
 *
 *    Reserve space in front of message for headers or data
 *
 */
void g_netbuf_reserve(GNetBuf *msg, guint len)
{
        msg->data+=len;
        msg->tail+=len;
}

/*
 * Function msg_headroom (msg)
 *
 *    Returns the number of bytes available for inserting headers or data
 *    in front of the message.
 */
gint g_netbuf_headroom(GNetBuf *msg)
{
	return msg->data - msg->head;
}

/*
 * Function g_netbuf_tailroom (msg)
 *
 *    Returns the number of bytes available for inserting more data into the
 *    message
 */
gint g_netbuf_tailroom(GNetBuf *msg)
{
	return msg->end - msg->tail;
}

/*
 * Function g_netbuf_trim (msg, len)
 *
 *    Set the length of the message
 *
 */
void g_netbuf_trim(GNetBuf *msg, guint len)
{
	if (msg->len > len) {
		msg->len = len;
		msg->tail = msg->data+len;
        }
}

void g_netbuf_print(GNetBuf *msg)
{
	gint i;

	for (i=0; i<msg->len; i++)
		g_print("%02x", msg->data[i]);
	g_print("\n");
}



