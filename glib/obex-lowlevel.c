/*
 *
 *  OpenOBEX - Free implementation of the Object Exchange protocol
 *
 *  Copyright (C) 2005-2006  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>

#include "obex-lowlevel.h"

#define OBEX_TIMEOUT 1

enum {
	OBEX_CONNECTED = 1,	/* Equal to TCP_ESTABLISHED */
	OBEX_OPEN,
	OBEX_BOUND,
	OBEX_LISTEN,
	OBEX_CONNECT,
	OBEX_CONNECT2,
	OBEX_CONFIG,
	OBEX_DISCONN,
	OBEX_CLOSED
};

typedef struct {
	unsigned long state;
	uint32_t cid;

	void *user_data;
	obex_callback_t *callback;

	obex_object_t *current, *pending;
} obex_context_t;

static void obex_progress(obex_t *handle, obex_object_t *object)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	if (context->callback && context->callback->progress_ind)
		context->callback->progress_ind(handle, context->user_data);
}

static void obex_connect_done(obex_t *handle,
					obex_object_t *object, int response)
{
	obex_context_t *context = OBEX_GetUserData(handle);
        obex_headerdata_t hd;
        uint32_t hl;
        uint8_t hi;

	if (response != OBEX_RSP_SUCCESS)
		return;

	context->state = OBEX_CONNECTED;

	while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hd, &hl)) {
		switch (hi) {
		case OBEX_HDR_CONNECTION:
			context->cid = hd.bq4;
			break;
		case OBEX_HDR_WHO:
			break;
		}
	}

	if (context->callback && context->callback->connect_cfm)
		context->callback->connect_cfm(handle, context->user_data);
}

static void obex_disconnect_done(obex_t *handle,
					obex_object_t *object, int response)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	context->state = OBEX_CLOSED;
}

static void obex_event(obex_t *handle, obex_object_t *object,
			int mode, int event, int command, int response)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	switch (event) {
	case OBEX_EV_PROGRESS:
		obex_progress(handle, object);
		break;

	case OBEX_EV_REQHINT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, response);
		break;

	case OBEX_EV_REQ:
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, response);
		break;

	case OBEX_EV_REQDONE:
		context->current = context->pending;
		if (context->current)
			OBEX_Request(handle, context->current);
		context->pending = NULL;

	        switch (command) {
	        case OBEX_CMD_CONNECT:
			obex_connect_done(handle, object, response);
			break;
		case OBEX_CMD_DISCONNECT:
			obex_disconnect_done(handle, object, response);
			break;
		case OBEX_CMD_PUT:
			break;
		case OBEX_CMD_GET:
			break;
		case OBEX_CMD_SETPATH:
			break;
		case OBEX_CMD_SESSION:
			break;
		case OBEX_CMD_ABORT:
			break;
		}
		break;

	case OBEX_EV_LINKERR:
		OBEX_TransportDisconnect(handle);
		break;

	case OBEX_EV_PARSEERR:
		OBEX_TransportDisconnect(handle);
		break;

	case OBEX_EV_ACCEPTHINT:
		break;

	case OBEX_EV_ABORT:
		break;

	case OBEX_EV_STREAMEMPTY:
		break;

	case OBEX_EV_STREAMAVAIL:
		break;

	case OBEX_EV_UNEXPECTED:
		break;

	case OBEX_EV_REQCHECK:
		break;
	}
}

obex_t *obex_open(int fd, obex_callback_t *callback, void *data)
{
	obex_t *handle;
	obex_context_t *context;

	context = malloc(sizeof(*context));
	if (!context)
		return NULL;

	memset(context, 0, sizeof(*context));

	context->state = OBEX_OPEN;
	context->cid = 0;

	handle = OBEX_Init(OBEX_TRANS_FD, obex_event, 0);
	if (!handle) {
		free(context);
		return NULL;
	}

	context->user_data = data;
	context->callback = callback;

	OBEX_SetUserData(handle, context);

	OBEX_SetTransportMTU(handle, OBEX_MAXIMUM_MTU, OBEX_MAXIMUM_MTU);

	if (FdOBEX_TransportSetup(handle, fd, fd, 0) < 0) {
		OBEX_Cleanup(handle);
		return NULL;
	}

        return handle;
}

void obex_close(obex_t *handle)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	OBEX_SetUserData(handle, NULL);

	free(context);

	OBEX_Cleanup(handle);
}

void obex_poll(obex_t *handle)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	unsigned long state = context->state;

	while (1) {
		OBEX_HandleInput(handle, OBEX_TIMEOUT);
		if (context->state != state)
			break;
        }
}

static int obex_send_or_queue(obex_t *handle, obex_object_t *object)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	if (context->current) {
		if (!context->pending) {
			context->pending = object;
			return 0;
		} else
			return -EBUSY;
	}

	context->current = object;

	return OBEX_Request(handle, object);
}

int obex_connect(obex_t *handle, const unsigned char *target, size_t size)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	obex_object_t *object;
	obex_headerdata_t hd;
	int err;

	if (context->state != OBEX_OPEN && context->state != OBEX_CLOSED)
		return -EISCONN;

	object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
	if (!object)
		return -ENOMEM;

	if (target) {
                hd.bs = target;

		err = OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_TARGET, hd, size, OBEX_FL_FIT_ONE_PACKET);
		if (err < 0) {
			OBEX_ObjectDelete(handle, object);
			return err;
                }
	}

	context->state = OBEX_CONNECT;

	return obex_send_or_queue(handle, object);	
}

int obex_disconnect(obex_t *handle)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	obex_object_t *object;

	if (context->state != OBEX_CONNECTED)
		return -ENOTCONN;

	object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
	if (!object)
		return -ENOMEM;

	context->state = OBEX_DISCONN;

	if (context->callback && context->callback->disconn_ind)
		context->callback->disconn_ind(handle, context->user_data);

	return obex_send_or_queue(handle, object);	
}

int obex_get(obex_t *handle, const char *type, const char *name)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	obex_object_t *object;
	obex_headerdata_t hd;
	int err;

	if (context->state != OBEX_OPEN && context->state != OBEX_CONNECT
					&& context->state != OBEX_CONNECTED)
		return -ENOTCONN;

	object = OBEX_ObjectNew(handle, OBEX_CMD_GET);
	if (!object)
		return -ENOMEM;

	if (context->cid > 0) {
		hd.bq4 = context->cid;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_CONNECTION, hd, 4, OBEX_FL_FIT_ONE_PACKET);
	}

	if (type) {
		int len = strlen(name);

		hd.bs = (uint8_t *) type;

		err = OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_TYPE, hd, len, OBEX_FL_FIT_ONE_PACKET);
		if (err < 0) {
			OBEX_ObjectDelete(handle, object);
			return err;
                }
        }

	if (name) {
		int len, ulen = (strlen(name) + 1) * 2;
		uint8_t *unicode = malloc(ulen);

		if (!unicode) {
			OBEX_ObjectDelete(handle, object);
			return -ENOMEM;
		}

		len = OBEX_CharToUnicode(unicode, (uint8_t *) name, ulen);
		hd.bs = unicode;

		err = OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_NAME, hd, len, OBEX_FL_FIT_ONE_PACKET);
		if (err < 0) {
			OBEX_ObjectDelete(handle, object);
			free(unicode);
			return err;
                }

                free(unicode);
        }

	return obex_send_or_queue(handle, object);	
}
