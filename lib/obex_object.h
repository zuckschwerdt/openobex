/**
	\file obex_object.h
	OBEX object related functions.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 1999, 2000 Dag Brattli, All Rights Reserved.
	Copyright (c) 1999, 2000 Pontus Fuchs, All Rights Reserved.

	OpenOBEX is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2.1 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with OpenOBEX. If not, see <http://www.gnu.org/>.
 */

#ifndef OBEX_OBJECT_H
#define OBEX_OBJECT_H

#include "obex_incl.h"

#if ! defined(_WIN32)
#  include <sys/time.h>
#endif
#include <time.h>
#include <inttypes.h>

struct databuffer;
struct databuffer_list;

/* If an object has no expected length we have to reallocated every
 * OBEX_OBJECT_ALLOCATIONTRESHOLD bytes */
#define OBEX_OBJECT_ALLOCATIONTRESHOLD 10240

struct obex_header_element {
	struct databuffer *buf;
	uint8_t hi;
	unsigned int flags;
	unsigned int length;
	unsigned int offset;
	int body_touched;
	int stream;
};

struct obex_object {
	time_t time;

	struct databuffer_list *tx_headerq;	/* List of headers to transmit*/
	struct databuffer_list *rx_headerq;	/* List of received headers */
	struct databuffer_list *rx_headerq_rm;	/* List of recieved header already read by the app */
	struct databuffer *rx_body;		/* The rx body header need some extra help */
	struct databuffer *tx_nonhdr_data;	/* Data before of headers (like CONNECT and SETPATH) */
	struct databuffer *rx_nonhdr_data;	/* -||- */

	uint8_t cmd;			/* The command of this object */

	/* The opcode fields are used as
	 * command when sending and response
	 * when recieving
	 */
	uint8_t opcode;			/* Opcode for normal packets */
	uint8_t lastopcode;		/* Opcode for last packet */
	unsigned int headeroffset;	/* Where to start parsing headers */

	int hinted_body_len;		/* Hinted body-length or 0 */
	int totallen;			/* Size of all headers */
	int abort;			/* Request shall be aborted */

	int checked;			/* OBEX_EV_REQCHECK has been signaled */

	int suspend;			/* Temporarily stop transfering object */
	int continue_received;		/* CONTINUE received after sending last command */
	int first_packet_sent;		/* Whether we've sent the first packet */

	const uint8_t *s_buf;		/* Pointer to streaming data */
	unsigned int s_len;		/* Length of stream-data */
	unsigned int s_offset;		/* Current offset in buf */
	int s_stop;			/* End of stream */
	int s_srv;			/* Deliver body as stream when server */
};

struct obex_object *obex_object_new(void);
int obex_object_delete(struct obex_object *object);
int obex_object_getspace(struct obex *self, struct obex_object *object, unsigned int flags);
int obex_object_addheader(struct obex *self, struct obex_object *object, uint8_t hi,
			  obex_headerdata_t hv, uint32_t hv_size,
			  unsigned int flags);
int obex_object_getnextheader(struct obex *self, struct obex_object *object, uint8_t *hi,
			      obex_headerdata_t *hv, uint32_t *hv_size);
int obex_object_reparseheaders(struct obex *self, struct obex_object *object);
int obex_object_setcmd(struct obex_object *object, uint8_t cmd, uint8_t lastcmd);
int obex_object_setrsp(struct obex_object *object, uint8_t rsp, uint8_t lastrsp);
int obex_object_send(struct obex *self, struct obex_object *object,
		     int allowfinalcmd, int forcefinalbit);
int obex_object_receive(struct obex *self, struct databuffer *msg);
int obex_object_readstream(struct obex *self, struct obex_object *object, const uint8_t **buf);
int obex_object_suspend(struct obex_object *object);
int obex_object_resume(struct obex *self, struct obex_object *object);

#endif
