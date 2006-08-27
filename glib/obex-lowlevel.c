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

#include "obex-lowlevel.h"

typedef struct {
} obex_context_t;

static void obex_event(obex_t *handle, obex_object_t *object,
			int mode, int event, int obex_cmd, int obex_rsp)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	if (!context)
		return;

	switch (event) {
	}
}


obex_t *obex_open(int fd)
{
	obex_t *handle;
	obex_context_t *context;

	context = malloc(sizeof(*context));
	if (!context)
		return NULL;

	handle = OBEX_Init(OBEX_TRANS_FD, obex_event, 0);
	if (!handle) {
		free(context);
		return NULL;
	}

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

int obex_connect(obex_t *handle, const unsigned char *target, size_t size)
{
	obex_object_t *object;
	int err;

	object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
	if (!object)
		return -ENOMEM;

	if (target) {
		obex_headerdata_t hd;
                hd.bs = target;

                err = OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_TARGET, hd, size, OBEX_FL_FIT_ONE_PACKET);
		if (err < 0) {
			OBEX_ObjectDelete(handle, object);
			return err;
                }
	}

	return OBEX_Request(handle, object);
}

int obex_disconnect(obex_t *handle)
{
	obex_object_t *object;

	object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
	if (!object)
		return -ENOMEM;

	return OBEX_Request(handle, object);
}
