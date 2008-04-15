/*********************************************************************
 *                
 * Filename:      irxfer.c
 * Version:       0.1
 * Description:   Send and receive files using IRDA
 * Status:        Experimental.
 * Author:        Fons Botman (budely@tref.nl)
 * Created at:    Sat May 15 11:19:34 CEST 1999
 * 
 *     Copyright (c) 1999, 2000 Fons Botman, All Rights Reserved.
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
 *
 *     Copied parts from irobex_palm3.c
 *
 *     Start without arguments to receive a file.
 *     Start with filename as argument to send a file. 
 *     
 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*sec)
#endif

#include <openobex/obex.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*sec)
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "obex_put_common.h"
#include "obex_io.h"

#define TRUE  1
#define FALSE 0

volatile int finished = FALSE;
obex_t *handle = NULL;

/*
 * Function main (argc, argv)
 *
 *    Starts all the fun!
 *
 */
int main(int argc, char *argv[])
{
	obex_object_t *object;
	int ret;

	if ((argc < 1) || (argc > 2) )	{
		printf ("Usage: %s [name]\n", argv[0]); 
		return -1;
	}

	handle = OBEX_Init(OBEX_TRANS_IRDA, obex_event, 0);
	obex_protocol_type = OBEX_PROTOCOL_WIN95_IRXFER;

	if (argc == 1) {
		/* We are server */
		printf("Waiting for files\n");
		IrOBEX_ServerRegister(handle, "OBEX:IrXfer");

		while (!finished) {
			OBEX_HandleInput(handle, 1);
		}
	} else {
		/* We are a client */

		/* Try to connect to peer */
		ret = IrOBEX_TransportConnect(handle, "OBEX:IrXfer");
		if (ret < 0) {
			printf("Sorry, unable to connect!\n");
			exit(ret);
		}

		object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
		ret = do_sync_request(handle, object, 0);

		if( (object = build_object_from_file(handle, argv[1], 0)) )	{
			ret = do_sync_request(handle, object, 0);
		}
		else	{
			perror("PUT failed");
		}

		object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
		ret = do_sync_request(handle, object, 0);

		printf("PUT successful\n");
		sleep(3);
	}
	return 0;
}
