/*********************************************************************
 *                
 * Filename:      obex_server.c
 * Version:       0.8
 * Description:   Handles server operations
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus.fuchs@tactel.se>
 * Created at:    Thu Nov 11 20:55:00 1999
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

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "obex_main.h"
#include "obex_header.h"
#include "obex_connect.h"


/*
 * Function obex_server ()
 *
 *    Handle server-operations
 *
 */
gint obex_server(obex_t *self, GNetBuf *msg, gint final)
{
	obex_common_hdr_t *request;
	gint cmd, ret;
	guint len;

	
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	request = (obex_common_hdr_t *) msg->data;
	cmd = request->opcode & ~OBEX_FINAL;
	len = ntohs(request->len);
	
	switch(self->state & ~MODE_SRV)
	{
	case STATE_IDLE:
		/* Nothing has been recieved yet, so this is probably a new request */
		DEBUG(4, G_GNUC_FUNCTION "() STATE_IDLE\n");
		
		if(self->object) {
			/* What shall we do here? I don't know!*/
			DEBUG(0, G_GNUC_FUNCTION "() Got a new server-request while already having one!\n");
			obex_response_request(self, OBEX_RSP_INTERNAL_SERVER_ERROR);
			return -1;
		}

		self->object = obex_object_new();

		if(self->object == NULL) {
			DEBUG(1, G_GNUC_FUNCTION "() Allocation of object failed!\n");
			obex_response_request(self, OBEX_RSP_INTERNAL_SERVER_ERROR);
			return -1;
		}
		/* Remember the initial command of the request.*/
		self->object->cmd = cmd;

		/* Hint app that something is about to come so that
		   the app can deny a PUT-like request early, or
		   set the header-offset */
		obex_deliver_event(self, OBEX_EV_REQHINT, cmd, 0, FALSE);
						
		/* Some commands needs special treatment (data outside headers) */
		switch(cmd)	{
		case OBEX_CMD_CONNECT:
			DEBUG(4, G_GNUC_FUNCTION "() Got CMD_CONNECT\n");
			/* Connect needs some extra special treatment */
			if(obex_parse_connect_header(self, msg) < 0) {
				obex_response_request(self, OBEX_RSP_BAD_REQUEST);
				obex_deliver_event(self, OBEX_EV_PARSEERR, self->object->opcode, 0, TRUE);
				return -1;
			}
			self->object->headeroffset = 4;
			break;
		case OBEX_CMD_SETPATH:
			self->object->headeroffset = 2;
			break;
		}

		self->state = MODE_SRV | STATE_REC;
		// no break! Fallthrough */
	
	case STATE_REC:
		DEBUG(4, G_GNUC_FUNCTION "() STATE_REC\n");
		/* In progress of recieving a request */
		
		/* Abort? */
		if(cmd == OBEX_CMD_ABORT) {
			DEBUG(1, G_GNUC_FUNCTION "() Got OBEX_ABORT request!\n");
			obex_deliver_event(self, OBEX_EV_ABORT, self->object->opcode, cmd, TRUE);
			obex_response_request(self, OBEX_RSP_SUCCESS);
			return 0;		
		}
		
		/* Sanity check */
		if(cmd != self->object->cmd) {
			/* The cmd-field of this packet is not the
			   same as int the first fragment. Bail out! */
			obex_response_request(self, OBEX_RSP_BAD_REQUEST);
			obex_deliver_event(self, OBEX_EV_PARSEERR, self->object->opcode, cmd, TRUE);
			return -1;
		}
		
		/* Get the headers... */
		if(obex_object_receive(self, msg) < 0)	{
			obex_response_request(self, OBEX_RSP_BAD_REQUEST);
			obex_deliver_event(self, OBEX_EV_PARSEERR, self->object->opcode, 0, TRUE);
			return -1;
		}
		
		if(!final) {
			if(obex_object_send(self, self->object, FALSE) < 0) {
				obex_deliver_event(self, OBEX_EV_LINKERR, cmd, 0, TRUE);
				return -1;
			}			
			else {
				obex_deliver_event(self, OBEX_EV_PROGRESS, cmd, 0, FALSE);
			}
			break; /* Stay in this state if not final */
		}
		else {
			DEBUG(4, G_GNUC_FUNCTION "() We got a request!\n");
			/* More connect-magic woodoo stuff */
			if(cmd == OBEX_CMD_CONNECT)
				obex_insert_connectframe(self, self->object);

			/* Tell the app that a whole request has arrived. While
			   this event is delivered the app should append the
			   headers that should be in the response */
			obex_deliver_event(self, OBEX_EV_REQ, cmd, 0, FALSE);
			self->state = MODE_SRV | STATE_SEND;
			len = 3; /* Otherwise sanitycheck later will fail */
		}
		/* Note the conditional fallthrough! */
	
	case STATE_SEND:
		/* Send back response */
		DEBUG(4, G_GNUC_FUNCTION "() STATE_SEND\n");
		
		/* Abort? */
		if(cmd == OBEX_CMD_ABORT) {
			DEBUG(1, G_GNUC_FUNCTION "() Got OBEX_ABORT request!\n");
			obex_response_request(self, OBEX_RSP_SUCCESS);
			obex_deliver_event(self, OBEX_EV_ABORT, self->object->opcode, cmd, TRUE);
			return 0;		
		}
		
		if(len > 3) {
			DEBUG(0, G_GNUC_FUNCTION "() STATE_SEND Didn't excpect data from peer (%d)\n", len);
			/* Hmmm, we got some data while sending. This is no good! */
			obex_response_request(self, OBEX_RSP_BAD_REQUEST);
			obex_deliver_event(self, OBEX_EV_PARSEERR, cmd, 0, TRUE);
			return 0;
		}
				
		ret = obex_object_send(self, self->object, TRUE);
		if(ret == 0) {
			/* Made some progress */
			obex_deliver_event(self, OBEX_EV_PROGRESS, cmd, 0, FALSE);
		}
		else if(ret < 0) {
			/* Error sending response */
			obex_deliver_event(self, OBEX_EV_LINKERR, cmd, 0, TRUE);
			return -1;
		}
		else {
			/* Response sent! */
			if(cmd == OBEX_CMD_DISCONNECT)	{
				DEBUG(2, G_GNUC_FUNCTION "() CMD_DISCONNECT done. Resetting MTU!\n");
				self->mtu_tx = OBEX_MINIMUM_MTU;
			}
			obex_deliver_event(self, OBEX_EV_REQDONE, cmd, 0, TRUE);
 			self->state = MODE_SRV | STATE_IDLE;
		}
		break;
	
	default:
		DEBUG(0, G_GNUC_FUNCTION "() Unknown state\n");
		obex_response_request(self, OBEX_RSP_BAD_REQUEST);
		obex_deliver_event(self, OBEX_EV_PARSEERR, cmd, 0, TRUE);
		return -1;
	}
	return 0;
}
