/*********************************************************************
 *                
 * Filename:      obex_client.c
 * Version:       0.5
 * Description:   Handles client-like operations
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus@tactel.se>
 * Created at:    Thu Nov 11 20:56:00 1999
 * Modified at:   Sun Dec  5 15:36:47 1999
 * Modified by:   Pontus Fuchs <pontus@tactel.se>
 * 
 *     Copyright (c) 1998-1999 Dag Brattli, All Rights Reserved.
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
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <obex_main.h>
#include <obex_header.h>
#include <obex_connect.h>

/*
 * Function obex_client ()
 *
 *    Handle client operations
 *
 */
gint obex_client(obex_t *self, GNetBuf *msg, gint final)
{
	obex_common_hdr_t *hdr;
	gint opcode;
	gint len;

	DEBUG(4, __FUNCTION__ "()\n");

	hdr = (obex_common_hdr_t *) msg->data;
	opcode = hdr->opcode & ~OBEX_FINAL;
	len = ntohs(hdr->len);

	/* Catch and handle a response from a CMD_CONNECT */
	if(self->lastcmd == OBEX_CMD_CONNECT)	{
		DEBUG(2, __FUNCTION__ "() We excpect a connect-rsp\n");
		if(obex_parse_connect_header(self, msg) < 0)	{
			obex_deliver_event(self, OBEX_CLIENT, OBEX_EV_PARSEERR, self->object->cmd, 0, TRUE);
			return -1;
		}
		self->object->headeroffset=4;
	}

	if(self->lastcmd == OBEX_CMD_DISCONNECT)	{
		DEBUG(2, __FUNCTION__ "() CMD_DISCONNECT done. Resetting MTU!\n");
		self->mtu_tx = OBEX_MINIMUM_MTU;
	}

	DEBUG(4, __FUNCTION__ "() RSP = %02x Final= %02x\n", opcode, final);
	if(len > 3) {
		DEBUG(3, __FUNCTION__ "() Response has headers. Size=%d\n", len);
		if(obex_object_receive(self->object, msg) < 0)	{
			obex_deliver_event(self, OBEX_CLIENT, OBEX_EV_PARSEERR, self->object->cmd, 0, TRUE);
			return -1;
		}
	}

	if(opcode == OBEX_RSP_CONTINUE)	{
		DEBUG(3, __FUNCTION__ "() Continue...\n");
		if(obex_object_send(self, self->object, 1) < 0)
			obex_deliver_event(self, OBEX_CLIENT, OBEX_EV_LINKERR, self->object->cmd, 0, TRUE);
		else
			obex_deliver_event(self, OBEX_CLIENT, OBEX_EV_PROGRESS, self->object->cmd, 0, FALSE);
	}
	else	{
		/* Notify app that client-operation is done! */
		DEBUG(3, __FUNCTION__ "() Done doing what we did. Rsp=%02x!\n", opcode);
		obex_deliver_event(self, OBEX_CLIENT, OBEX_EV_REQDONE, self->object->cmd, opcode, TRUE);
	}
	return 0;
}
