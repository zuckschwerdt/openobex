/*********************************************************************
 *                
 * Filename:      obex_test_client.c
 * Version:       0.8
 * Description:   Client OBEX Commands
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus.fuchs@tactel.se>
 * Created at:    Sun Aug 13 03:00:28 PM CEST 2000
 * Modified at:   Sun Aug 13 03:00:33 PM CEST 2000
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
#include <openobex/obex.h>
#include <glib.h>

#include "obex_io.h"
#include "obex_test_client.h"
#include "obex_test.h"

//
// Wait for an obex command to finish.
//
void syncwait(obex_t *handle)
{
	struct context *gt;
	gint ret;
	
	gt = OBEX_GetUserData(handle);

	while(!gt->clientdone) {
		ret = OBEX_HandleInput(handle, 20);
		if(ret < 0) {
			g_print("Error while doing OBEX_HandleInput()\n");
			break;
		}
		if(ret == 0) {
			g_print("Timeout waiting for data\n");
			break;
		}
	}

	gt->clientdone = FALSE;
}

//
//
//
void client_done(obex_t *handle, obex_object_t *object, gint obex_cmd, gint obex_rsp)
{
	struct context *gt;
	gt = OBEX_GetUserData(handle);

	switch(obex_cmd)	{
	case OBEX_CMD_CONNECT:
		connect_client_done(handle, object, obex_rsp);
		break;
	case OBEX_CMD_DISCONNECT:
		disconnect_client_done(handle, object, obex_rsp);
		break;
	case OBEX_CMD_PUT:
		put_client_done(handle, object, obex_rsp);
		break;
	case OBEX_CMD_GET:
		get_client_done(handle, object, obex_rsp);
		break;
	case OBEX_CMD_SETPATH:
		setpath_client_done(handle, object, obex_rsp);
		break;
	}
	gt->clientdone = TRUE;
}

//
//
//
void connect_client(obex_t *handle)
{
	obex_object_t *object;
	obex_headerdata_t hd;

	if(! (object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT)))	{
		g_print("Error\n");
		return;
	}

	hd.bs = "Linux";
	if(OBEX_ObjectAddHeader(handle, object, OBEX_HDR_WHO, hd, 6,
				OBEX_FL_FIT_ONE_PACKET) < 0)	{
		g_print("Error adding header\n");
		OBEX_ObjectDelete(handle, object);
		return;
	}
	OBEX_Request(handle, object);
	syncwait(handle);
}

//
//
//
void connect_client_done(obex_t *handle, obex_object_t *object, gint obex_rsp)
{
	guint8 *nonhdrdata;

	if(obex_rsp == OBEX_RSP_SUCCESS) {
		g_print("Connect OK!\n");
		if(OBEX_ObjectGetNonHdrData(object, &nonhdrdata) == 4) {
			g_print("Version: 0x%02x. Flags: 0x%02x\n", nonhdrdata[0], nonhdrdata[1]);
		}
	}	
	else {
		g_print("Connect failed 0x%02x!\n", obex_rsp);
	}
}


//
//
//
void disconnect_client(obex_t *handle)
{
	obex_object_t *object;

	if(! (object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT)))	{
		g_print("Error\n");
		return;
	}

	OBEX_Request(handle, object);
	syncwait(handle);
}

//
//
//
void disconnect_client_done(obex_t *handle, obex_object_t *object, gint obex_rsp)
{
	g_print("Disconnect done!\n");
	OBEX_TransportDisconnect(handle);
}
	

//
//
//
void put_client(obex_t *handle)
{
	obex_object_t *object;

	gchar lname[200];
	gchar rname[200];
	guint rname_size;
	obex_headerdata_t hd;
	
	guint8 *buf;
	int file_size;

	g_print("PUT file (local, remote)> ");
	scanf("%s %s", lname, rname);

	buf = easy_readfile(lname, &file_size);
	if(buf == NULL)
		return;

	g_print("Going to send %d bytes\n", file_size);

	/* Build object */
	object = OBEX_ObjectNew(handle, OBEX_CMD_PUT);
	
	rname_size = OBEX_CharToUnicode(rname, rname, sizeof(rname));
	hd.bq4 = file_size;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hd, 4, 0);

	hd.bs = rname;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hd, rname_size, 0);

	hd.bs = buf;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hd, file_size, 0);

	g_free(buf);

 	OBEX_Request(handle, object);
	syncwait(handle);
}


//
//
//
void put_client_done(obex_t *handle, obex_object_t *object, gint obex_rsp)
{
	if(obex_rsp == OBEX_RSP_SUCCESS) {
		g_print("PUT successful!\n");
	}	
	else {
		g_print("PUT failed 0x%02x!\n", obex_rsp);
	}
}

//
//
//
void get_client(obex_t *handle)
{
	obex_object_t *object;
	gchar rname[200];
	gchar req_name[200];
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

	// Set the userdata of this object to the name so we know the name when the request
	// is ready in get_client_done!

	OBEX_ObjectSetUserData(object, req_name);
	OBEX_Request(handle, object);
	syncwait(handle);
}

//
//
//
void get_client_done(obex_t *handle, obex_object_t *object, gint obex_rsp)
{
	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;

	gchar *req_name;
	const guint8 *body = NULL;
	gint body_len = 0;

	if(obex_rsp != OBEX_RSP_SUCCESS) {
		g_print("GET failed 0x%02x!\n", obex_rsp);
		return;
	}


	// Fetch the name set by get_client()
	req_name = OBEX_ObjectGetUserData(object);

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


	if(!body) {
		g_print("No body found in answer!\n");
		return;
	}	
	g_print("GET successful!\n");
	safe_save_file(req_name, body, body_len);
}
	

//
//
//
void setpath_client(obex_t *handle)
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
	syncwait(handle);
}

//
//
//
void setpath_client_done(obex_t *handle, obex_object_t *object, gint obex_rsp)
{
	if(obex_rsp == OBEX_RSP_SUCCESS) {
		g_print("SETPATH successful!\n");
	}	
	else {
		g_print("SETPATH failed 0x%02x!\n", obex_rsp);
	}
}
	
