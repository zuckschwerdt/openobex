/*********************************************************************
 *                
 *                
 * Filename:      obex_header.h
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus@tactel.se>
 * Created at:    Mon Mar  1 10:30:54 1999
 * Modified at:   Tue Nov 16 19:24:22 1999
 * Modified by:   Pontus Fuchs <pontus@tactel.se>
 * 
 *     Copyright (c) 1999 Pontus Fuchs, All Rights Reserved.
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
   
#ifndef OBEX_HEADERS_H
#define OBEX_HEADERS_H
 
#include "obex_main.h"

#define OBEX_HI_MASK     0xc0
#define OBEX_UNICODE     0x00
#define OBEX_BYTE_STREAM 0x40
#define OBEX_BYTE        0x80
#define OBEX_INT         0xc0

#define PACKED __attribute__((packed))

/* Common header used by all frames */
struct obex_common_hdr {
	guint8  opcode;
	guint16 len;
} PACKED;
typedef struct obex_common_hdr obex_common_hdr_t;

/* Connect header */
struct obex_connect_hdr {
	guint8  version;
	guint8  flags;
	guint16 mtu;
} PACKED;
typedef struct obex_connect_hdr obex_connect_hdr_t;

struct obex_uint_hdr {
	guint8  hi;
	guint32 hv;
} PACKED;

/*
struct obex_ushort_hdr {
	guint8  hi;
	guint16 hv;	
} PACKED;
*/

struct obex_ubyte_hdr {
	guint8 hi;
	guint8 hv;
} PACKED;

struct obex_unicode_hdr {
	guint8  hi;
	guint16 hl;
	guint8  hv[0];
} PACKED;

#define obex_byte_stream_hdr obex_unicode_hdr

typedef struct {
	guint8 identifier;    /* Header ID */
	gint  length;         /* Total lenght of header */

	gint  val_size;       /* Size of value */
	union {
		gint   integer;
		char   *string;
		guint8 *oct_seq;
	} t;
} obex_header_t;

char *unicode_to_char(guint8 *buf);

gint insert_uint_header(GNetBuf *msg, guint8 identifier, guint32 value);
gint insert_ubyte_header(GNetBuf *msg, guint8 identifier, guint8 value);
gint insert_unicode_header(GNetBuf *msg, guint8 opcode, char *text,
				gint size);
gint insert_byte_stream_header(GNetBuf *msg, guint8 opcode, guint8 *stream, 
			       gint size);

gint obex_extract_header(GNetBuf *msg, obex_header_t *header);

#endif
