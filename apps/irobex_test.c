/*********************************************************************
 *                
 * Filename:      irobex_test.c
 * Version:       0.7
 * Description:   Test IrOBEX and OBEX over cable to R320s. 
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus@tactel.se>
 * Created at:    Wed Nov 17 22:05:16 1999
 * Modified at:   Fri Nov 26 18:38:54 1999
 * Modified by:   Pontus Fuchs <pontus@tactel.se>
 * 
 *     Copyright (c) 1998, 1999, Dag Brattli, All Rights Reserved.
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

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include <obex/obex.h>
#include "obex_io.h"

#ifndef _WIN32
#include "cobex_R320.h"
#endif

obex_t *handle;
gchar req_name[200];
gboolean finished = FALSE;;

#define IOMODE 0	/* 0 for sync, 1 for async */

/*
 * Function waitsync ()
 *
 * Wait for an obex command to finish.
 *
 */
void syncwait()
{
	while(!finished)
		OBEX_HandleInput(handle, 1);

	finished = FALSE;
}


/*
 * Function client_done ()
 *
 * 
 *
 */
void client_done(obex_object_t *object, gint obex_cmd, gint obex_rsp)
{
	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;
	GString *err;

	guint8 *body = NULL;
	guint8 *nonhdrdata;
	gint body_len;

	g_print(G_GNUC_FUNCTION "()\n");

#ifndef _WIN32
	if(obex_rsp != OBEX_RSP_SUCCESS)	{
		err = OBEX_GetResponseMessage(handle, obex_rsp);
		g_print(G_GNUC_FUNCTION "() Operation failed %s (%02x)\n", err->str, obex_rsp);
		g_string_free(err, TRUE);
	}
#endif

	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		if(hi == OBEX_HDR_BODY)	{
		g_print(G_GNUC_FUNCTION "() Found body\n");
			body = hv.bs;
			body_len = hlen;
			break;
		}
		else	{
			g_print(G_GNUC_FUNCTION "() Skipped header %02x\n", hi);
		}
	}

	switch(obex_cmd)	{
	case OBEX_CMD_CONNECT:
		if(obex_rsp == OBEX_RSP_SUCCESS) {
			g_print("Connect OK!\n");
			if(OBEX_ObjectGetNonHdrData(object, &nonhdrdata) == 4) {
				g_print("Version: 0x%02x. Flags: 0x%02x\n", nonhdrdata[0], nonhdrdata[1]);
			}
		}	
		else {
			g_print("Connect failed!\n");
		}


		break;
	case OBEX_CMD_DISCONNECT:
		g_print("Disconnect done!\n");
		OBEX_TransportDisconnect(handle);
		break;
	}
	
	if(body)
		safe_save_file(req_name, body, body_len);
}



/* Handle an incoming PUT*/
void put_server(obex_object_t *object)
{
	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;

	guint8 *body = NULL;
	gint body_len;
	gchar *name = NULL;
	gchar *namebuf = NULL;

	g_print(G_GNUC_FUNCTION "()\n");

	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_BODY:
			g_print(G_GNUC_FUNCTION "() Found body\n");
			body = hv.bs;
			body_len = hlen;
			break;
		case OBEX_HDR_NAME:
			g_print(G_GNUC_FUNCTION "() Found name\n");
			if( (namebuf = g_malloc(hlen / 2)))	{
				OBEX_UnicodeToChar(namebuf, hv.bs, hlen);
				name = namebuf;
			}
			break;
		
		default:
			g_print(G_GNUC_FUNCTION "() Skipped header %02x\n", hi);
		}
	}
	if(!body)	{
		g_print("Got a PUT without a body\n");
		return;
	}
	if(!name)	{
		name = "OBEX_PUT_Unknown_object";
		g_print("Got a PUT without a name. Setting name to %s\n", name);

	}
	safe_save_file(name, body, body_len);
	g_free(namebuf);
}

/* Handle GET as a server*/
void get_server(obex_object_t *object)
{
	guint8 *buf;
	gint fd;
	gint actual;

	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;
	gint filesize;

	gchar *name = NULL;
	gchar *namebuf = NULL;

	g_print(G_GNUC_FUNCTION "()\n");

	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_NAME:
			g_print(G_GNUC_FUNCTION "() Found name\n");
			if( (namebuf = g_malloc(hlen / 2)))	{
				OBEX_UnicodeToChar(namebuf, hv.bs, hlen);
				name = namebuf;
			}
			break;
		
		default:
			g_print(G_GNUC_FUNCTION "() Skipped header %02x\n", hi);
		}
	}

	if(!name)	{
		g_print(G_GNUC_FUNCTION "() Got a GET without a name-header!\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_FOUND, OBEX_RSP_NOT_FOUND);
		return;
	}
	g_print(G_GNUC_FUNCTION "() Got a request for %s\n", name);

	fd = open(name, O_RDONLY);
	if( fd < 0)	{
		perror("Can't open file");
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_FOUND, OBEX_RSP_NOT_FOUND);
		return;
	}

	filesize = get_filesize(name);
	if(filesize < 0) {
		close(fd);
		OBEX_ObjectSetRsp(object, OBEX_RSP_INTERNAL_SERVER_ERROR, OBEX_RSP_INTERNAL_SERVER_ERROR);
		return;
	}

	if(! (buf = g_malloc(filesize)) )	{
		close(fd);
		OBEX_ObjectSetRsp(object, OBEX_RSP_INTERNAL_SERVER_ERROR, OBEX_RSP_INTERNAL_SERVER_ERROR);
		return;
	}

	actual = read(fd, buf, filesize);
	OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
	hv.bs = buf;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hv, actual, 0);
	hv.bq4 = filesize;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hv, sizeof(guint32), 0);
	close(fd);
	g_free(buf);
	return;
}

/* When server_indication sees a connect-attempt. Here we look for headers, and
   If we find a HEADER_WHO with Linux in it we get happy */

void connect_server(obex_object_t *object)
{
	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;

	guint8 *who;
	gint who_len;
	g_print(G_GNUC_FUNCTION "()\n");

	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		if(hi == OBEX_HDR_WHO)	{
			who = hv.bs;
			who_len = hlen;
		}
		else	{
			g_print(G_GNUC_FUNCTION "() Skipped header %02x\n", hi);
		}
	}
	if (who_len == 6)	{
		if(strncmp("Linux", who, 6) == 0)	{
			g_print("Weeeha. I'm talking to the coolest OS ever!\n");
		}
	}
	OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
}

/*
 * Function server_request ()
 *
 *
 *
 */
void server_request(obex_object_t *object, gint event, gint cmd)
{
	switch(cmd)	{
	case OBEX_CMD_CONNECT:
		connect_server(object);
		break;
	case OBEX_CMD_DISCONNECT:
		g_print("We got a disconnect-request\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		break;
	case OBEX_CMD_GET:
		/* A Get always fits one package */
		get_server(object);
		break;
	case OBEX_CMD_PUT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		put_server(object);
		break;
	case OBEX_CMD_SETPATH:
		g_print("Got a SETPATH request\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		break;
	default:
		g_print(G_GNUC_FUNCTION "() Denied %02x request\n", cmd);
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
		break;
	}
	return;
}


/*
 * Function obex_event ()
 *
 *    Called by the obex-layer when some event occurs.
 *
 */
void obex_event(obex_object_t *object, gint mode, gint event, gint obex_cmd, gint obex_rsp)
{

	switch (event)	{
	case OBEX_EV_PROGRESS:
		g_print("Made some progress...\n");
		break;
	case OBEX_EV_REQDONE:
		if(mode == OBEX_CLIENT)
			client_done(object, obex_cmd, obex_rsp);
		else	{
			switch (obex_cmd) {
			case OBEX_CMD_DISCONNECT:
				g_print("Disconnect done!\n");
				OBEX_TransportDisconnect(handle);
				break;
			default:
				g_print(G_GNUC_FUNCTION "() Command (%02x) has now finished\n", obex_cmd);
				break;
			}
		}
		finished = TRUE;
		break;
	case OBEX_EV_REQHINT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);	//Dirty...
		break;
	case OBEX_EV_REQ:
		server_request(object, event, obex_cmd);
		break;
	case OBEX_EV_LINKERR:
		g_print("Link broken!\n");
		OBEX_TransportDisconnect(handle);
		finished = TRUE;
		break;
	default:
		g_print("Unknown event!\n");
		break;
	}
}


/* Do a PUT of any file on local system */
void put_client(void)
{
	obex_object_t *object;

	guint8 *buf;
	gchar lname[200];
	gchar rname[200];
	guint rname_size;
	obex_headerdata_t hd;
	
	int filesize;
	int fd;
	int actual;

	printf("PUT file (local, remote)> ");
	scanf("%s %s", lname, rname);

	fd = open(lname, O_RDONLY, 0);
	if (fd == -1) {
		perror("Unable to open file");
		return;
	}
	
	filesize = get_filesize(lname);

	if(! (buf = g_malloc(filesize)) )	{
		close(fd);
		return;
	}
	actual = read(fd, buf, filesize);

	/* Build object */
	object = OBEX_ObjectNew(handle, OBEX_CMD_PUT);
	
	rname_size = OBEX_CharToUnicode(rname, rname, sizeof(rname));
	hd.bq4 = filesize;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hd, 4, 0);

	hd.bs = rname;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hd, rname_size, 0);

	hd.bs = buf;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hd, actual, 0);

	close(fd); 
	g_free(buf);

 	OBEX_Request(handle, object);
	syncwait();
}

/* Do a GET of some file as a client. */
void get_client()
{
	obex_object_t *object;
	char rname[200];
	gint rname_size;
	obex_headerdata_t hd;

	printf("GET File> ");
	scanf("%s", req_name);

	if(! (object = OBEX_ObjectNew(handle, OBEX_CMD_GET)))	{
		g_print("Error\n");
		return;
	}

	rname_size = OBEX_CharToUnicode(rname, req_name, sizeof(rname));

	hd.bs = rname;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hd,
				rname_size, OBEX_FL_FIT_ONE_PACKET);

	OBEX_Request(handle, object);
	syncwait();
}

/* Do a connect. Throw in a WHO-header which says Linux\0 */
void connect_client()
{
	obex_object_t *object;
	obex_headerdata_t hd;

//	guint8 longstring[245] = {0,};

	/* First connect transport */
	if(IrOBEX_TransportConnect(handle, "OBEX") < 0)	{
		g_print("Transport connect error!\n");
		return;
	}

	if(! (object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT)))	{
		g_print("Error\n");
		return;
	}

	hd.bs = "Linux";
	if(OBEX_ObjectAddHeader(handle, object, OBEX_HDR_WHO,
				hd, 6,
				OBEX_FL_FIT_ONE_PACKET) < 0)	{
//	if(OBEX_ObjectAddHeader(handle, object, OBEX_HDR_WHO,
//				(obex_headerdata_t) (guint8*) longstring, sizeof(longstring),
//				OBEX_FIT_ONE_PACKET) < 0)	{
		g_print("Error adding header\n");
		OBEX_ObjectDelete(handle, object);
		return;
	}
	OBEX_Request(handle, object);
	syncwait();
}

/* Do a Disconnect */
void disconnect_client()
{
	obex_object_t *object;

	if(! (object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT)))	{
		g_print("Error\n");
		return;
	}


	OBEX_Request(handle, object);
	syncwait();
}

void setpath_client()
{
	char setpath_data[2] = {0,0};
	obex_object_t *object;
	char path[200];
	gint path_size;
	obex_headerdata_t hd;

	printf("SETPATH> ");
	scanf("%s", path);

	if(! (object = OBEX_ObjectNew(handle, OBEX_CMD_SETPATH)))	{
		g_print("Error\n");
		return;
	}

	path_size = OBEX_CharToUnicode(path, path, sizeof(path));

	hd.bs = path;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hd,
				path_size, OBEX_FL_FIT_ONE_PACKET);

	OBEX_ObjectSetNonHdrData(object, setpath_data, 2);
	OBEX_Request(handle, object);
	syncwait();
}



int main (int argc, char *argv[])
{
	char cmd[10];
	int end = 0;
	int cobex = FALSE;

	obex_ctrans_t custfunc;

#ifndef _WIN32
	if( (argc == 2) && (strcmp(argv[1], "cable") == 0 ) )
		cobex = TRUE;
#endif

	if(cobex)	{
#ifndef _WIN32
		g_print("Do the cable-OBEX!\n");
		if(! (handle = OBEX_Init(OBEX_TRANS_CUST, obex_event, 0)))	{
			perror( "OBEX_Init failed");
			exit(0);
		}

		custfunc.connect = cobex_connect;
		custfunc.disconnect = cobex_disconnect;
		custfunc.write = cobex_write;
		custfunc.listen = NULL;
		if(OBEX_RegisterCTransport(handle, &custfunc) < 0)	{
			g_print("Custom transport callback-registration failed\n");
		}
#endif
	}
	else	{
		if(! (handle = OBEX_Init(OBEX_TRANS_IRDA, obex_event, IOMODE)))	{
			perror( "OBEX_Init failed");
			exit(0);
		}
	}
	
	printf( "IrOBEX Test-Application.\n");

	while (!end) {
		printf("> ");
		scanf("%s", cmd);
		switch (cmd[0] | 0x20)	{
			case 'q':
				end=1;
			break;
			case 'g':
				get_client();
			break;
			case 't':
				setpath_client();
			break;
			case 'p':
				put_client();
			break;
			case 'c':
				connect_client();
			break;
			case 'd':
				disconnect_client();
			break;
			case 's':
				OBEX_ServerRegister(handle, "OBEX");	
			break;
			default:
				printf("Unknown command %s\n", cmd);
		}
	}
	return 0;
}
