/*********************************************************************
 *                
 * Filename:      obex_test_server.c
 * Version:       0.8
 * Description:   Server OBEX Commands
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
#include <obex/obex.h>

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "obex_io.h"
#include "obex_test.h"
#include "obex_test_server.h"

//
//
//
void put_server(obex_t *handle, obex_object_t *object)
{
	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;

	guint8 *body = NULL;
	gint body_len = 0;
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

//
//
//
void get_server(obex_t *handle, obex_object_t *object)
{
	guint8 *buf;

	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;
	gint file_size;

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

	buf = easy_readfile(name, &file_size);
	if(buf == NULL) {
		g_print("Can't find file %s\n", name);
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_FOUND, OBEX_RSP_NOT_FOUND);
		return;
	}

	OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
	hv.bs = buf;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hv, file_size, 0);
	hv.bq4 = file_size;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hv, sizeof(guint32), 0);
	g_free(buf);
	return;
}

//
//
//
void connect_server(obex_t *handle, obex_object_t *object)
{
	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;

	guint8 *who = NULL;
	gint who_len = 0;
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

//
//
//
void server_request(obex_t *handle, obex_object_t *object, gint event, gint cmd)
{
	switch(cmd)	{
	case OBEX_CMD_CONNECT:
		connect_server(handle, object);
		break;
	case OBEX_CMD_DISCONNECT:
		g_print("We got a disconnect-request\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		break;
	case OBEX_CMD_GET:
		/* A Get always fits one package */
		get_server(handle, object);
		break;
	case OBEX_CMD_PUT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		put_server(handle, object);
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

//
//
//
void server_done(obex_t *handle, obex_object_t *object, gint obex_cmd, gint obex_rsp)
{
	struct context *gt;
	gt = OBEX_GetUserData(handle);

	g_print("Server request finished!\n");

	switch (obex_cmd) {
	case OBEX_CMD_DISCONNECT:
		g_print("Disconnect done!\n");
		OBEX_TransportDisconnect(handle);
		gt->serverdone = TRUE;
		break;

	default:
		g_print(G_GNUC_FUNCTION "() Command (%02x) has now finished\n", obex_cmd);
		break;
	}
}

//
//
//
void server(obex_t *handle, char *service)
{
	struct context *gt;
	gt = OBEX_GetUserData(handle);

	gt->serverdone = FALSE;
	OBEX_ServerRegister(handle, service);	
	while(!gt->serverdone) {
		if(OBEX_HandleInput(handle, 1) < 0) {
			g_print("Error while doing OBEX_HandleInput()\n");
			break;
		}
	}
}
