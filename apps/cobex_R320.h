/*********************************************************************
 *                
 * Filename:      cobex_R320.h
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus@tactel.se>
 * Created at:    Wed Nov 17 22:05:16 1999
 * Modified at:   Wed Nov 17 22:06:57 1999
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

int cobex_init(char *ttyname);
void cobex_cleanup(int force);
int cobex_start_io(void);

gint cobex_write(obex_t *self, guint8 *buffer, gint length);
gint cobex_connect(obex_t *handle);
gint cobex_disconnect(obex_t *handle);
