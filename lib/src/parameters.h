/*********************************************************************
 *                
 * Filename:      parameters.h
 * Version:       1.0
 * Description:   A more general way to handle (pi,pl,pv) parameters
 * Status:        Experimental.
 * Author:        Dag Brattli <dagb@cs.uit.no>
 * Created at:    Mon Jun  7 08:47:28 1999
 * Modified at:   Fri Oct  8 11:15:40 1999
 * Modified by:   Dag Brattli <dagb@cs.uit.no>
 * 
 *     Copyright (c) 1999 Dag Brattli, All Rights Reserved.
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
 ********************************************************************/

#ifndef IRDA_PARAMS_H
#define IRDA_PARAMS_H

#include <glib.h>

/*
 *  The currently supported types. Beware not to change the sequence since
 *  it a good reason why the sized integers has a value equal to their size
 */
typedef enum {
	PV_INTEGER,      /* Integer of any (pl) length */
	PV_INT_8_BITS,   /* Integer of 8 bits in length */
	PV_INT_16_BITS,  /* Integer of 16 bits in length */
	PV_STRING,       /* \0 terminated string */
	PV_INT_32_BITS,  /* Integer of 32 bits in length */
	PV_OCT_SEQ,      /* Octet sequence */
	PV_NO_VALUE      /* Does not contain any value (pl=0) */
} PV_TYPE;

/* Bit 7 of type field */
#define PV_BIG_ENDIAN    0x80 
#define PV_LITTLE_ENDIAN 0x00
#define PV_MASK          0x7f   /* To mask away endian bit */

#define PV_PUT 0
#define PV_GET 1

typedef union {
	gchar   *c;
	guint8   b;
	guint16  s;
	guint32  i;
	guint8  *bp;
	guint16 *sp;
	guint32 *ip;
} pv_t;

typedef struct {
	guint8 pi;
	guint8 pl;
	pv_t   pv;
} param_t;

typedef int (*PI_HANDLER)(void *self, param_t *param, int get);
typedef int (*PV_HANDLER)(void *self, guint8 *buf, gint len, guint8 pi,
			  PV_TYPE type, PI_HANDLER func);

typedef struct {
	PI_HANDLER func;  /* Handler for this parameter identifier */
	PV_TYPE    type;  /* Data type for this parameter */
} pi_minor_info_t;

typedef struct {
	pi_minor_info_t *pi_minor_call_table;
	gint len;
} pi_major_info_t;

typedef struct {
	pi_major_info_t *tables;
	gint             len;
	guint8           pi_mask;
	gint             pi_major_offset;
} pi_param_info_t;

int irda_param_pack(guint8 *buf, gchar *fmt, ...);
int irda_param_unpack(guint8 *buf, gchar *fmt, ...);

int irda_param_insert(void *self, guint8 pi, guint8 *buf, gint len, 
		      pi_param_info_t *info);
int irda_param_extract(void *self, guint8 *buf, int len, 
		       pi_param_info_t *info);
int irda_param_extract_all(void *self, guint8 *buf, gint len, 
			   pi_param_info_t *info);

extern inline int irda_param_insert_byte(guint8 *buf, guint8 pi, guint8 pv)
{
	return irda_param_pack(buf, "bbb", pi, 1, pv);
}

#endif /* IRDA_PARAMS_H */

