/*********************************************************************
 *                
 * Filename:      obex_client.h
 * Version:	  
 * Description:   
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus@tactel.se>
 * Created at:    Thu Nov 11 20:58:00 1999
 * Modified at:   Tue Nov 16 19:26:39 1999
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

#ifndef OBEX_CLIENT_H
#define OBEX_CLIENT_H

//void obex_client_event(obex_t *self, gint event, gint cmd, gint rsp, gboolean del);
gint obex_client(obex_t *self, GNetBuf *msg, gint final);

#endif
