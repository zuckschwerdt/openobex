/*********************************************************************
 *                
 * Filename:      obex_test.c
 * Version:       0.8
 * Description:   Test IrOBEX, TCPOBEX and OBEX over cable to R320s. 
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus.fuchs@tactel.se>
 * Created at:    Wed Nov 17 22:05:16 1999
 * Modified at:   Sun Aug 13 02:08:45 PM CEST 2000
 * Modified by:   Pontus Fuchs <pontus.fuchs@tactel.se>
 * 
 *     Copyright (c) 2000, Pontus Fuchs, All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <obex/obex.h>
#include <glib.h>

#if _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif /* _WIN32 */

#include "obex_test.h"
#include "obex_test_client.h"
#include "obex_test_server.h"

#ifndef _WIN32
#include "obex_test_cable.h"
#endif

//
// Called by the obex-layer when some event occurs.
//
void obex_event(obex_t *handle, obex_object_t *object, gint mode, gint event, gint obex_cmd, gint obex_rsp)
{
	switch (event)	{
	case OBEX_EV_PROGRESS:
		g_print("Made some progress...\n");
		break;

	case OBEX_EV_REQDONE:
		if(mode == OBEX_CLIENT) {
			client_done(handle, object, obex_cmd, obex_rsp);
		}
		else	{
			server_done(handle, object, obex_cmd, obex_rsp);
		}
		break;
	case OBEX_EV_REQHINT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);	//Dirty...
		break;

	case OBEX_EV_REQ:
		server_request(handle, object, event, obex_cmd);
		break;

	case OBEX_EV_LINKERR:
		g_print("Link broken!\n");
		break;

	default:
		g_print("Unknown event %02x!\n", event);
		break;
	}
}

/*
 * Function get_peer_addr (name, peer)
 *
 *    
 *
 */
gint get_peer_addr(char *name, struct sockaddr_in *peer) 
{
	struct hostent *host;
	u_long inaddr;
        
	/* Is the address in dotted decimal? */
	if ((inaddr = inet_addr(name)) != INADDR_NONE) {
		memcpy((char *) &peer->sin_addr, (char *) &inaddr,
		      sizeof(inaddr));  
	}
	else {
		if ((host = gethostbyname(name)) == NULL) {
			g_error( "Bad host name: ");
			exit(-1);
                }
		memcpy((char *) &peer->sin_addr, host->h_addr,
				host->h_length);
        }
	return 0;
}

//
//
//
gint inet_connect(obex_t *handle)
{
	struct sockaddr_in peer;

	get_peer_addr("localhost", &peer);
	return OBEX_TransportConnect(handle, (struct sockaddr *) &peer,
				  sizeof(struct sockaddr_in));
}
	
//
//
//
int main (int argc, char *argv[])
{
	char cmd[10];
	int end = 0;
	gboolean cobex = FALSE, tcpobex = FALSE, r320 = FALSE;
	obex_t *handle;

	struct context global_context = {0,};

#ifndef _WIN32

	gchar *port;
	obex_ctrans_t custfunc;

	if( (argc == 2 || argc ==3) && (strcmp(argv[1], "-s") == 0 ) )
		cobex = TRUE;
	if( (argc == 2 || argc ==3) && (strcmp(argv[1], "-r") == 0 ) ) {
		cobex = TRUE;
		r320 = TRUE;
	}
#endif

	if( (argc == 2) && (strcmp(argv[1], "-i") == 0 ) )
		tcpobex = TRUE;


	if(cobex)	{
#ifndef _WIN32
		if(argc == 3)
			port = argv[2];
		else
			port = "/dev/ttyS0";

		if(r320)
			g_print("OBEX to R320 on %s!\n", port);
		else
			g_print("OBEX on %s!\n", port);

		custfunc.userdata = cobex_open(port, r320);

		if(custfunc.userdata == NULL) {
			g_print("cobex_open() failed\n");
			return -1;
		}

		if(! (handle = OBEX_Init(OBEX_TRANS_CUST, obex_event, 0)))	{
			perror( "OBEX_Init failed");
			return -1;
		}

		custfunc.connect = cobex_connect;
		custfunc.disconnect = cobex_disconnect;
		custfunc.write = cobex_write;
		custfunc.handleinput = cobex_handle_input;
		custfunc.listen = cobex_connect;	// Listen and connect is 100% same on cable

		if(OBEX_RegisterCTransport(handle, &custfunc) < 0)	{
			g_print("Custom transport callback-registration failed\n");
		}
#else
		g_message("Not implemented in Win32 yet.\n");
#endif	// _WIN32
	}

	else if(tcpobex) {
		g_print("Using TCP transport\n");
		if(! (handle = OBEX_Init(OBEX_TRANS_INET, obex_event, 0)))	{
			perror( "OBEX_Init failed");
			exit(0);
		}
	}
	else	{
		g_print("Using IRDA transport\n");
		if(! (handle = OBEX_Init(OBEX_TRANS_IRDA, obex_event, 0)))	{
			perror( "OBEX_Init failed");
			exit(0);
		}
	}

	OBEX_SetUserData(handle, &global_context);
	
	g_print( "OBEX Interactive test client/server.\n");

	while (!end) {
		printf("> ");
		scanf("%s", cmd);
		switch (cmd[0] | 0x20)	{
			case 'q':
				end=1;
			break;
			case 'g':
				get_client(handle);
			break;
			case 't':
				setpath_client(handle);
			break;
			case 'p':
				put_client(handle);
			break;
			case 'c':
				/* First connect transport */
				if(tcpobex) {
					if(inet_connect(handle)) {
						g_print("Transport connect error!\n");
						break;
					}
				}
				if(cobex) {
					if(OBEX_TransportConnect(handle, (void*) 1, 0) < 0) {
						g_print("Transport connect error!\n");
						break;
					}
				}
				else {
					if(IrOBEX_TransportConnect(handle, "OBEX") < 0)	{
						g_print("Transport connect error!\n");
						break;
					}
				}
				// Now send OBEX-connect.	
				connect_client(handle);
			break;
			case 'd':
				disconnect_client(handle);
			break;
			case 's':
				server(handle);
			break;
			default:
				printf("Unknown command %s\n", cmd);
		}
	}
#ifndef _WIN32
	if(cobex)
		cobex_close(custfunc.userdata);
#endif

	return 0;
}

