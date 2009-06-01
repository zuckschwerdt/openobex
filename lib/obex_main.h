/**
	\file obex_main.h
	Implementation of the Object Exchange Protocol OBEX.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 1998, 1999, 2000 Dag Brattli, All Rights Reserved.
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

#ifndef OBEX_MAIN_H
#define OBEX_MAIN_H

#include "obex_incl.h"

#if defined(_WIN32)
#  include <winsock2.h>
#  define socket_t SOCKET
#else
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <sys/time.h>
#  define socket_t int
#  define INVALID_SOCKET -1
#endif
#include <time.h>
#include <inttypes.h>

struct databuffer;
struct obex_object;

#include "obex_transport.h"
#include "defines.h"
#include "debug.h"

struct obex {
	uint16_t mtu_tx;			/* Maximum OBEX TX packet size */
	uint16_t mtu_rx;			/* Maximum OBEX RX packet size */
	uint16_t mtu_tx_max;		/* Maximum TX we can accept */

	socket_t fd;		/* Socket descriptor */
	socket_t serverfd;
	socket_t writefd;	/* write descriptor - only OBEX_TRANS_FD */
	unsigned int state;

	int keepserver;		/* Keep server alive */
	int filterhint;		/* Filter devices based on hint bits */
	int filterias;		/* Filter devices based on IAS entry */

	struct databuffer *tx_msg;	/* Reusable transmit message */
	struct databuffer *rx_msg;	/* Reusable receive message */

	struct obex_object *object;	/* Current object being transfered */
	obex_event_t eventcb;		/* Event-callback */

	obex_transport_t trans;		/* Transport being used */
	obex_ctrans_t ctrans;

	obex_interface_t *interfaces;	/* Array of discovered interfaces */
	int interfaces_number;		/* Number of discovered interfaces */

	void * userdata;		/* For user */
};

socket_t obex_create_socket(struct obex *self, int domain);
int obex_delete_socket(struct obex *self, socket_t fd);

void obex_deliver_event(struct obex *self, int event, int cmd, int rsp, int del);
int obex_data_indication(struct obex *self, uint8_t *buf, int buflen);

void obex_response_request(struct obex *self, uint8_t opcode);
int obex_data_request(struct obex *self, struct databuffer *msg, int opcode);
int obex_cancelrequest(struct obex *self, int nice);

char *obex_response_to_string(int rsp);

#endif
