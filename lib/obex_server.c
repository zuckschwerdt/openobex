/*********************************************************************
 *                
 * Filename:      obex_server.c
 * Version:       0.5
 * Description:   Handles server-like operations
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus.fuchs@tactel.se>
 * Created at:    Thu Nov 11 20:55:00 1999
 * Modified at:   Sun Dec  5 15:36:36 1999
 * Modified by:   Pontus Fuchs <pontus.fuchs@tactel.se>
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
	obex_common_hdr_t *hdr;
	gint opcode;
	gint ret;

	DEBUG(4, G_GNUC_FUNCTION "()\n");

	hdr = (obex_common_hdr_t *) msg->data;
	opcode = hdr->opcode & ~OBEX_FINAL;

	if(! self->object) {
		DEBUG(4, G_GNUC_FUNCTION "() First package of a request\n");
		/* This is the first package of a request */
		self->object = obex_object_new();

		if(! self->object) {
			DEBUG(1, G_GNUC_FUNCTION "() Allocation of object failed!\n");
			obex_response_request(self, OBEX_RSP_INTERNAL_SERVER_ERROR);
			return -1;
		}


		/* Hint app that something is about to come so that
		   the app can deny a PUT-like request early, or
		   set the header-offset */
		obex_deliver_event(self, OBEX_SERVER, OBEX_EV_REQHINT, opcode, 0, FALSE);

		switch(opcode)	{
		case OBEX_CMD_CONNECT:
			DEBUG(4, G_GNUC_FUNCTION "() Got CMD_CONNECT\n");
			/* Connect needs some special treatment */
			if(obex_parse_connect_header(self, msg) < 0) {
				obex_response_request(self, OBEX_RSP_BAD_REQUEST);
				obex_deliver_event(self, OBEX_CLIENT, OBEX_EV_PARSEERR, self->object->cmd, 0, TRUE);
				return -1;
			}
			self->object->headeroffset = 4;
			break;
		case OBEX_CMD_SETPATH:
			self->object->headeroffset = 2;
			break;
			}

		if(obex_object_receive(self->object, msg) < 0)	{
			obex_response_request(self, OBEX_RSP_BAD_REQUEST);
			obex_deliver_event(self, OBEX_CLIENT, OBEX_EV_PARSEERR, self->object->cmd, 0, TRUE);
			return -1;
		}
	}
	else	{
		if(obex_object_receive(self->object, msg) < 0)	{
			obex_response_request(self, OBEX_RSP_BAD_REQUEST);
			obex_deliver_event(self, OBEX_CLIENT, OBEX_EV_PARSEERR, self->object->cmd, 0, TRUE);
			return -1;
		}
	}

	if( ( self->object->app_is_called == FALSE ) && final)	{
		DEBUG(4, G_GNUC_FUNCTION "() We got a request!\n");
		self->object->app_is_called = TRUE;
		/* More connect-magic woodoo stuff */
		if(opcode == OBEX_CMD_CONNECT)
			obex_insert_connectframe(self, self->object);

		obex_deliver_event(self, OBEX_SERVER, OBEX_EV_REQ, opcode, 0, FALSE);
	}

	ret = obex_object_send(self, self->object, self->object->app_is_called);
	if(ret == 0)
		obex_deliver_event(self, OBEX_SERVER, OBEX_EV_PROGRESS, opcode, 0, FALSE);
	else if(ret > 0) {
		DEBUG(3, G_GNUC_FUNCTION "() Done doing what we did!\n");

		if(opcode == OBEX_CMD_DISCONNECT)	{
			DEBUG(2, G_GNUC_FUNCTION "() CMD_DISCONNECT done. Resetting MTU!\n");
			self->mtu_tx = OBEX_MINIMUM_MTU;
		}
		obex_deliver_event(self, OBEX_SERVER, OBEX_EV_REQDONE, opcode, 0, TRUE);
	}
	else
		obex_deliver_event(self, OBEX_SERVER, OBEX_EV_LINKERR, opcode, 0, TRUE);
	return 0;
}
