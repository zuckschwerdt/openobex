/*********************************************************************
 *                
 * Filename:      irobex_palm3.c
 * Version:       0.3
 * Description:   Demonstrates use of PUT command
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus.fuchs@tactel.se>
 * Created at:    Tue Mar 23 09:44:56 1999
 * Modified at:   Sun Aug 13 09:25:40 PM CEST 2000
 * Modified by:   Pontus Fuchs <pontus.fuchs@tactel.se>
 * 
 *     Copyright (c) 1999, 2000 Ponts Fuchs, All Rights Reserved.
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
 *     The Palm 3 always send a HEADER_CREATORID (0xcf).
 *
 *     Start without arguments to receive a file.
 *     Start with filename as argument to send a file. 
 *     
 ********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#endif

//#include <openobex/obex.h>
#include "obex.h"

#include "obex_put_common.h"
#include "obex_io.h"

#define TRUE  1
#define FALSE 0

obex_t *handle = NULL;
volatile int finished = FALSE;
extern int last_rsp;

/*
 * Function main (argc, )
 *
 *    Starts all the fun!
 *
 */
int main(int argc, char *argv[])
{
	obex_object_t *object;
	int ret, exitval = EXIT_SUCCESS;

	printf("Send and receive files to Palm3\n");
	if ((argc < 1) || (argc > 2))	{
		printf ("Usage: %s [name]\n", argv[0]); 
		return -1;
	}
	handle = OBEX_Init(OBEX_TRANS_IRDA, obex_event, 0);

	if (argc == 1)	{
		/* We are server*/

		printf("Waiting for files\n");
		IrOBEX_ServerRegister(handle, "OBEX");

		while (!finished)
			OBEX_HandleInput(handle, 1);
	}
	else {
		/* We are a client */

		/* Try to connect to peer */
		ret = IrOBEX_TransportConnect(handle, "OBEX");
		if (ret < 0) {
			printf("Sorry, unable to connect!\n");
			return EXIT_FAILURE;
		}

		object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
		ret = do_sync_request(handle, object, FALSE);
		if ((last_rsp != OBEX_RSP_SUCCESS) || (ret < 0)) {
			printf("Sorry, unable to connect!\n");
			return EXIT_FAILURE;
		}
		if ((object = build_object_from_file(handle, argv[1])))	{
			ret = do_sync_request(handle, object, FALSE);
			if ((last_rsp != OBEX_RSP_SUCCESS) || (ret < 0))
				exitval = EXIT_FAILURE;
		} else
			exitval = EXIT_FAILURE;

		object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
		ret = do_sync_request(handle, object, FALSE);
		if ((last_rsp != OBEX_RSP_SUCCESS) || (ret < 0))
			exitval = EXIT_FAILURE;

		if (exitval == EXIT_SUCCESS)
			printf("PUT successful\n");
		else
			printf("PUT failed\n");
	}
//	sleep(1);
	return exitval;
}
