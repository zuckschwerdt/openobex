/*********************************************************************
 *                
 * Filename:      obex.h
 * Version:       0.9.7
 * Description:   OBEX API
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Fri Apr 23 14:02:42 1999
 * Modified at:   Tue Nov 21 19:20:00 PM CEST 2000
 * Modified by:   Pontus Fuchs <pontus.fuchs@tactel.se>
 * 
 *     Copyright (c) 1999, 2000 Dag Brattli, All Rights Reserved.
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

#ifndef OBEX_H
#define OBEX_H

#include <glib.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif

// These are also defined in obex_main.h
// They are defined here otherwise any app including it
// would require obex_main.h which includes a lot of other 
// unnecessary stuff.
typedef void* obex_t;
typedef void* obex_object_t;
typedef void (*obex_event_t)(obex_t *handle, obex_object_t *obj, gint mode, gint event, gint obex_cmd, gint obex_rsp);

#include "obex_const.h"

/*
 *  OBEX API
 */
obex_t *OBEX_Init(gint transport, obex_event_t eventcb, guint flags);
void    OBEX_Cleanup(obex_t *self);
void OBEX_SetUserData(obex_t *self, gpointer data);
gpointer OBEX_GetUserData(obex_t *self);
gint OBEX_GetFD(obex_t *self);

gint OBEX_RegisterCTransport(obex_t *self, obex_ctrans_t *ctrans);

gint    OBEX_TransportConnect(obex_t *self, struct sockaddr *saddr, int addlen);
gint    OBEX_TransportDisconnect(obex_t *self);
gint    OBEX_CustomDataFeed(obex_t *self, guint8 *inputbuf, gint actual);
gint    OBEX_HandleInput(obex_t *self, gint timeout);

gint    OBEX_ServerRegister(obex_t *self, const char *service);
obex_t *OBEX_ServerAccept(obex_t *server, obex_event_t eventcb, gpointer data);
gint    OBEX_Request(obex_t *self, obex_object_t *object);
gint    OBEX_CancelRequest(obex_t *self);

obex_object_t	*OBEX_ObjectNew(obex_t *self, guint8 cmd);
gint		OBEX_ObjectDelete(obex_t *self, obex_object_t *object);

gint		OBEX_ObjectAddHeader(obex_t *self, obex_object_t *object, guint8 hi, 
			obex_headerdata_t hv, guint32 hv_size, guint flags);
gint OBEX_ObjectGetNextHeader(obex_t *self, obex_object_t *object, guint8 *hi,
					obex_headerdata_t *hv,
					guint32 *hv_size);
gint OBEX_ObjectSetRsp(obex_object_t *object, guint8 cmd, guint8 lastcmd);

gint OBEX_ObjectGetNonHdrData(obex_object_t *object, guint8 **buffer);
gint OBEX_ObjectSetNonHdrData(obex_object_t *object, guint8 *buffer, guint len);
gint OBEX_ObjectSetHdrOffset(obex_object_t *object, guint offset);
void OBEX_ObjectSetUserData(obex_object_t *object, gpointer data);
gpointer OBEX_ObjectGetUserData(obex_object_t *object);

gint OBEX_UnicodeToChar(guint8 *c, const guint8 *uc, gint size);
gint OBEX_CharToUnicode(guint8 *uc, const guint8 *c, gint size);

GString* OBEX_GetResponseMessage(obex_t *self, gint rsp);

/*
 * IrOBEX API 
 */
 gint IrOBEX_TransportConnect(obex_t *self, const char *service);

#endif
