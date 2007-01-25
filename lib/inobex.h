/*********************************************************************
 *                
 * Filename:      inobex.h
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Mon Apr 26 13:55:27 1999
 * CVS ID:        $Id$
 * 
 *     Copyright (c) 1999 Dag Brattli, All Rights Reserved.
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

#ifndef INOBEX_H
#define INOBEX_H

void inobex_prepare_connect(obex_t *self, struct sockaddr *saddr, int addrlen);
void inobex_prepare_listen(obex_t *self, struct sockaddr *saddr, int addrlen);
int inobex_listen(obex_t *self);
int inobex_accept(obex_t *self);
int inobex_connect_request(obex_t *self);
int inobex_disconnect_request(obex_t *self);
int inobex_disconnect_server(obex_t *self);

#endif
