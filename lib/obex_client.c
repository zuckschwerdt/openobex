/*********************************************************************
 *                
 * Filename:      obex_client.c
 * Version:       0.8
 * Description:   Handles client operations
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus.fuchs@tactel.se>
 * Created at:    Thu Nov 11 20:56:00 1999
 * CVS ID:        $Id$
 *
 *     Copyright (c) 1999-2000 Pontus Fuchs, All Rights Reserved.
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

#include <stdlib.h>
#include <stdio.h>

#include "obex_main.h"
#include "obex_header.h"
#include "obex_connect.h"

/*
 * Function obex_client ()
 *
 *    Handle client operations
 *
 */
gint obex_client(obex_t *self, GNetBuf *msg, gint final)
{
	obex_common_hdr_t *response = NULL;
	gint rsp = OBEX_RSP_BAD_REQUEST, ret;
	
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	/* If this is a response we have some data in msg */
	if(msg) {
		response = (obex_common_hdr_t *) msg->data;
		rsp = response->opcode & ~OBEX_FINAL;
	}
	
	switch(self->state)
	{
	case STATE_SEND:
		/* In progress of sending request */
		DEBUG(4, G_GNUC_FUNCTION "() STATE_SEND\n");
		
		/* Any errors from peer? Win2k will send RSP_SUCCESS after
		   every fragment sent so we have to accept that too.*/
		if(rsp != OBEX_RSP_SUCCESS && rsp != OBEX_RSP_CONTINUE) {
			obex_deliver_event(self, OBEX_EV_REQDONE, self->object->opcode, rsp, TRUE);
			return 0;		
		}
				
		if(ntohs(response->len) > 3) {
			DEBUG(0, G_GNUC_FUNCTION "() STATE_SEND. Didn't excpect data from peer (%d)\n", ntohs(response->len));
			g_netbuf_print(msg);
			obex_response_request(self, OBEX_RSP_BAD_REQUEST);
			obex_deliver_event(self, OBEX_EV_PARSEERR, rsp, 0, TRUE);
			return 0;
		}
		// No break here!! Fallthrough	
	
	case STATE_START:
		/* Nothing has been sent yet */
		DEBUG(4, G_GNUC_FUNCTION "() STATE_START\n");
		
		ret = obex_object_send(self, self->object, TRUE, FALSE);
		if(ret < 0) {
			/* Error while sending */
			obex_deliver_event(self, OBEX_EV_LINKERR, self->object->opcode, 0, TRUE);
			self->state = MODE_CLI | STATE_IDLE;
		}
		else if (ret == 0) {
			/* Some progress made */			
			obex_deliver_event(self, OBEX_EV_PROGRESS, self->object->opcode, 0, FALSE);
                	self->state = MODE_CLI | STATE_SEND;
		}
                else {
                	/* Sending of object finished.. */
                	self->state = MODE_CLI | STATE_REC;
                }
		break;
			
	case STATE_REC:
		/* Recieving answer of request */
		DEBUG(4, G_GNUC_FUNCTION "() STATE_REC\n");
		
		/* Response of a CMD_CONNECT needs some special treatment.*/
		if(self->object->opcode == OBEX_CMD_CONNECT)	{
			DEBUG(2, G_GNUC_FUNCTION "() We excpect a connect-rsp\n");
			if(obex_parse_connect_header(self, msg) < 0)	{
				obex_deliver_event(self, OBEX_EV_PARSEERR, self->object->opcode, 0, TRUE);
				self->state = MODE_SRV | STATE_IDLE;
				return -1;
			}
			self->object->headeroffset=4;
		}

		/* So does CMD_DISCONNECT */
		if(self->object->opcode == OBEX_CMD_DISCONNECT)	{
			DEBUG(2, G_GNUC_FUNCTION "() CMD_DISCONNECT done. Resetting MTU!\n");
			self->mtu_tx = OBEX_MINIMUM_MTU;
		}

		/* Recieve any headers */
		if(obex_object_receive(self, msg) < 0)	{
			obex_deliver_event(self, OBEX_EV_PARSEERR, self->object->opcode, 0, TRUE);
			self->state = MODE_SRV | STATE_IDLE;
			return -1;
		}
	
		/* Are we done yet? */
		if(rsp == OBEX_RSP_CONTINUE) {
			DEBUG(3, G_GNUC_FUNCTION "() Continue...\n");
			if(obex_object_send(self, self->object, TRUE, FALSE) < 0)
				obex_deliver_event(self, OBEX_EV_LINKERR, self->object->opcode, 0, TRUE);
			else
				obex_deliver_event(self, OBEX_EV_PROGRESS, self->object->opcode, 0, FALSE);
		}
		else	{
			/* Notify app that client-operation is done! */
			DEBUG(3, G_GNUC_FUNCTION "() Done! Rsp=%02x!\n", rsp);
			obex_deliver_event(self, OBEX_EV_REQDONE, self->object->opcode, rsp, TRUE);
			self->state = MODE_SRV | STATE_IDLE;
		}
		break;
       	
       	default:
		DEBUG(0, G_GNUC_FUNCTION "() Unknown state\n");		
		obex_deliver_event(self, OBEX_EV_PARSEERR, rsp, 0, TRUE);
		return -1;
	}

	return 0;
}
