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
#include <time.h>
#include <arpa/inet.h>

#include "obex-debug.h"
#include "obex-lowlevel.h"

#define BUF_SIZE 32767

#define CID_INVALID 0xFFFFFFFF

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

typedef struct obex_setpath_hdr {
    uint8_t  flags;
    uint8_t constants;
} __attribute__ ((packed)) obex_setpath_hdr_t;

typedef struct obex_connect_hdr {
    uint8_t  version;
    uint8_t  flags;
    uint16_t mtu;
} __attribute__ ((packed)) obex_connect_hdr_t;

typedef struct {
	unsigned long state;
	uint32_t cid;

	void *user_data;
	obex_callback_t *callback;

	obex_object_t *pending;

	int obex_rsp;		/* Response to last OBEX command */

	/* Transfer related variables */
	char buf[BUF_SIZE];	/* Data buffer for put and get requests */
	int data_start;		/* Offset of data in buffer */
	int data_length;	/* Length of actual data in buffer */
	int counter;		/* Total bytes transfered so far */
	int target_size;	/* Final length of object under transfer */
	time_t modtime;		/* Modification time of object under transfer */
	int tx_max;		/* Maximum size for sent chunks of data */
	int close;		/* If the user has called obex_close */
	int do_cb;		/* Whether to call the transfer callback */
	int cb_event;		/* Event to pass to the transfer callback */
} obex_context_t;

static int make_iso8601(time_t time, char *str, int len) {
    struct tm tm;
#if defined(HAVE_TIMEZONE) && defined(USE_LOCALTIME)
    time_t tz_offset = 0;

    tzset();

    tz_offset = -timezone;
    if (daylight > 0)
        tz_offset += 3600;
    time += tz_offset;
#endif

    if (gmtime_r(&time, &tm) == NULL)
        return -1;

    tm.tm_year += 1900;
    tm.tm_mon++;

    return snprintf(str, len,
#ifdef USE_LOCALTIME
                    "%04u%02u%02uT%02u%02u%02u",
#else
                    "%04u%02u%02uT%02u%02u%02uZ",
#endif
                    tm.tm_year, tm.tm_mon, tm.tm_mday,
                    tm.tm_hour, tm.tm_min, tm.tm_sec);
}

time_t parse_iso8601(const char *str, int len)
{
	char *tstr, tz;
	struct tm tm;
	int nr;
	time_t time, tz_offset = 0;

	memset(&tm, 0, sizeof(struct tm));

	/* According to spec the time doesn't have to be null terminated */
	tstr = malloc(len + 1);
	if (!tstr)
		return -1;

	strncpy(tstr, str, len);
	tstr[len] = '\0';

	nr = sscanf(tstr, "%04u%02u%02uT%02u%02u%02u%c",
			&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
			&tm.tm_hour, &tm.tm_min, &tm.tm_sec,
			&tz);

	free(tstr);

	/* Fixup the tm values */
	tm.tm_year -= 1900;       /* Year since 1900 */
	tm.tm_mon--;              /* Months since January, values 0-11 */
	tm.tm_isdst = -1;         /* Daylight savings information not avail */

	if (nr < 6) {
		/* Invalid time format */
		return -1;
	} 

	time = mktime(&tm);

#if defined(HAVE_TM_GMTOFF)
	tz_offset = tm.tm_gmtoff;
#elif defined(HAVE_TIMEZONE)
	tz_offset = -timezone;
	if (tm.tm_isdst > 0)
		tz_offset += 3600;
#endif

	if (nr == 7) { /* Date/Time was in localtime (to remote device)
			* already. Since we don't know anything about the
			* timezone on that one we won't try to apply UTC offset
			*/
		time += tz_offset;
	}

	return time;
}

static void get_target_size_and_time(obex_t *handle, obex_object_t *object,
                                     int *size, time_t *time) {
    obex_headerdata_t hv;
    uint8_t hi;
    unsigned int hlen;

    *size = -1;
    *time = -1;

    while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen)) {
        switch (hi) {
            case OBEX_HDR_LENGTH:
                *size = hv.bq4;
                break;
            case OBEX_HDR_TIME:
                *time = parse_iso8601((char *) hv.bs, hlen);
                break;
            default:
                break;
        }
    }

    OBEX_ObjectReParseHeaders(handle, object);
}

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
        uint8_t hi, *ptr;

	if (response != OBEX_RSP_SUCCESS)
		return;

	context->state = OBEX_CONNECTED;

	if (OBEX_ObjectGetNonHdrData(object, &ptr) == sizeof(obex_connect_hdr_t)) {
		obex_connect_hdr_t *chdr = (obex_connect_hdr_t *) ptr;
		uint16_t mtu = ntohs(chdr->mtu);
		int new_size;
		
		debug("Connect success. Version: 0x%02x. Flags: 0x%02x OBEX packet length: %d",
				chdr->version, chdr->flags, mtu);

		/* Leave space for headers */
		new_size = mtu - 200;
		if (new_size < context->tx_max) {
			debug("Resizing stream chunks to %d", new_size);
			context->tx_max = new_size;
		}
	}

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

void obex_do_callback(obex_t *handle)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	if (!context->do_cb)
		return;

	context->do_cb = 0;

	if (context->callback && context->callback->transfer_ind)
		context->callback->transfer_ind(handle, context->cb_event, context->user_data);
}

static void obex_abort_done(obex_t *handle)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	context->do_cb = 1;
	context->cb_event = OBEX_EV_ABORT;
}

static void obex_transfer_done(obex_t *handle, int response)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	context->obex_rsp = response;

	context->do_cb = 1;
	context->cb_event = OBEX_EV_REQDONE;
}

static void obex_readstream(obex_t *handle, obex_object_t *object)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	const uint8_t *buf;
	int actual, free_space;

	if (context->counter == 0)
		get_target_size_and_time(handle, object, &context->target_size,
						&context->modtime);

	actual = OBEX_ObjectReadStream(handle, object, &buf);
	if (actual <= 0) {
		debug("Error or no data on OBEX stream");
		return;
	}

	context->counter += actual;

	debug("obex_readstream: got %d bytes (%d in total)", actual, context->counter);

	free_space = sizeof(context->buf) - (context->data_start + context->data_length);
	if (actual > free_space) {
		/* This should never happen */
		debug("Out of buffer space: actual=%d, free=%d", actual, free_space);
		return;
	}

	memcpy(&context->buf[context->data_start], buf, actual);
	context->data_length += actual;

	debug("OBEX_SuspendRequest");
	OBEX_SuspendRequest(handle, object);

	context->do_cb = 1;
	context->cb_event = OBEX_EV_STREAMAVAIL;
}

static void obex_writestream(obex_t *handle, obex_object_t *object)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	obex_headerdata_t hv;

	if (context->data_length > 0) {
		int send_size = context->data_length > context->tx_max ?
						context->tx_max : context->data_length;

		hv.bs = (uint8_t *) &context->buf[context->data_start];
		OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY,
				hv, send_size, OBEX_FL_STREAM_DATA);

		context->counter += send_size;
		context->data_length -= send_size;

		if (context->data_length == 0)
			context->data_start = 0;
		else
			context->data_start += send_size;

		if (!context->close) {
			debug("OBEX_SuspendRequest");
			OBEX_SuspendRequest(handle, object);
			context->do_cb = 1;
			context->cb_event = OBEX_EV_STREAMEMPTY;
		}
	}
	else {
		hv.bs = NULL;
		OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hv, 0,
					OBEX_FL_STREAM_DATAEND);
	}
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
		debug("OBEX_EV_REQDONE");

		if (context->pending) {
			OBEX_Request(handle, context->pending);
			context->pending = NULL;
		}

	        switch (command) {
	        case OBEX_CMD_CONNECT:
			obex_connect_done(handle, object, response);
			break;
		case OBEX_CMD_DISCONNECT:
			obex_disconnect_done(handle, object, response);
			break;
		case OBEX_CMD_PUT:
		case OBEX_CMD_GET:
			obex_transfer_done(handle, response);
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
		obex_abort_done(handle);
		break;

	case OBEX_EV_STREAMEMPTY:
		debug("OBEX_EV_STREAMEMPTY");
		obex_writestream(handle, object);
		break;

	case OBEX_EV_STREAMAVAIL:
		debug("OBEX_EV_STREAMAVAIL");
		obex_readstream(handle, object);
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
	context->cid = CID_INVALID;

	handle = OBEX_Init(OBEX_TRANS_FD, obex_event, 0);
	if (!handle) {
		free(context);
		return NULL;
	}

	context->user_data = data;
	context->callback = callback;
	context->tx_max = sizeof(context->buf);

	OBEX_SetUserData(handle, context);

	OBEX_SetTransportMTU(handle, sizeof(context->buf), sizeof(context->buf));

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

	if (context->pending)
		OBEX_ObjectDelete(handle, context->pending);

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

	obex_do_callback(handle);
}

static int obex_send_or_queue(obex_t *handle, obex_object_t *object)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	int err;

	err = OBEX_Request(handle, object);

	if (err == -EBUSY && !context->pending) {
		context->pending = object;
		return 0;
	}

	return err;
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

int obex_put(obex_t *handle, const char *type, const char *name, int size, time_t mtime)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	obex_object_t *object;
	obex_headerdata_t hd;
	int err, cmd;

	if (context->state != OBEX_OPEN && context->state != OBEX_CONNECT
					&& context->state != OBEX_CONNECTED)
		return -ENOTCONN;

	cmd = OBEX_ObjectGetCommand(handle, NULL);
	if (cmd == OBEX_CMD_GET || cmd == OBEX_CMD_PUT)
		return -EBUSY;

	/* Initialize transfer variables */
	context->data_start = 0;
	context->data_length = 0;
	context->counter = 0;
	context->target_size = size;
	context->modtime = mtime;
	context->close = 0;

	object = OBEX_ObjectNew(handle, OBEX_CMD_PUT);
	if (!object)
		return -ENOMEM;

	if (context->cid != CID_INVALID) {
		hd.bq4 = context->cid;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_CONNECTION, hd, 4, OBEX_FL_FIT_ONE_PACKET);
	}

	if (type) {
		int len = strlen(name) + 1;

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

	/* Add a time header if possible */
	if (context->modtime >= 0) {
		char tstr[17];
		int len;

		len = make_iso8601(context->modtime, tstr, sizeof(tstr));

		if (len >= 0) {
			debug("Adding time header: %s", tstr);
			hd.bs = (uint8_t *) tstr;
			OBEX_ObjectAddHeader(handle, object, OBEX_HDR_TIME, hd, len, 0);
		}
	}

	/* Add a length header if possible */
	if (context->target_size > 0) {
		debug("Adding length header: %d", context->target_size);
		hd.bq4 = (uint32_t) context->target_size;
		OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hd, 4, 0);
	}

	hd.bs = NULL;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hd, 0, OBEX_FL_STREAM_START);

	/* We need to suspend until the user has provided some data
	 * by calling obex_client_write */
	debug("OBEX_SuspendRequest");
	OBEX_SuspendRequest(handle, object);

	err = obex_send_or_queue(handle, object);
	if (err < 0) {
		context->target_size = -1;
		OBEX_ObjectDelete(handle, object);
	}

	return err;
}

int obex_get(obex_t *handle, const char *type, const char *name)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	obex_object_t *object;
	obex_headerdata_t hd;
	int err, cmd;

	if (context->state != OBEX_OPEN && context->state != OBEX_CONNECT
					&& context->state != OBEX_CONNECTED)
		return -ENOTCONN;

	cmd = OBEX_ObjectGetCommand(handle, NULL);
	if (cmd == OBEX_CMD_GET || cmd == OBEX_CMD_PUT)
		return -EBUSY;

	/* Initialize transfer variables */
	context->data_start = 0;
	context->data_length = 0;
	context->counter = 0;
	context->target_size = -1;
	context->modtime = -1;
	context->close = 0;

	object = OBEX_ObjectNew(handle, OBEX_CMD_GET);
	if (!object)
		return -ENOMEM;

	if (context->cid != CID_INVALID) {
		hd.bq4 = context->cid;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_CONNECTION, hd, 4, OBEX_FL_FIT_ONE_PACKET);
	}

	if (type) {
		int len = strlen(name) + 1;

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

	OBEX_ObjectReadStream(handle, object, NULL);

	err = obex_send_or_queue(handle, object);
	if (err < 0)
		OBEX_ObjectDelete(handle, object);

	return err;
}

int obex_read(obex_t *handle, char *buf, size_t count, size_t *bytes_read)
{
	obex_context_t *context = OBEX_GetUserData(handle);

	if (OBEX_ObjectGetCommand(handle, NULL) != OBEX_CMD_GET)
		return -EINVAL;

	if (context->data_length == 0)
		return -EAGAIN;

	*bytes_read = count < context->data_length ? count : context->data_length;

	memcpy(buf, &context->buf[context->data_start], *bytes_read);

	context->data_length -= *bytes_read;

	if (context->data_length)
		context->data_start += *bytes_read;
	else {
		context->data_start = 0;
		debug("OBEX_ResumeRequest");
		OBEX_ResumeRequest(handle);
	}

	return 0;
}

int obex_write(obex_t *handle, const char *buf, size_t count, size_t *bytes_written)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	int free_space;

	if (OBEX_ObjectGetCommand(handle, NULL) != OBEX_CMD_PUT)
		return -EINVAL;

	free_space = sizeof(context->buf) - (context->data_start + context->data_length);

	*bytes_written = count > free_space ? free_space : count;

	memcpy(&context->buf[context->data_start + context->data_length], buf, *bytes_written);

	context->data_length += *bytes_written;

	if (context->data_length) {
		debug("OBEX_ResumeRequest");
		OBEX_ResumeRequest(handle);
	}

	return 0;
}

int obex_abort(obex_t *handle)
{
	int cmd;

	cmd = OBEX_ObjectGetCommand(handle, NULL);
	if (cmd != OBEX_CMD_PUT || cmd != OBEX_CMD_GET)
		return -EINVAL;

	return OBEX_CancelRequest(handle, 1);
}

int obex_close_transfer(obex_t *handle)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	int cmd;

	cmd = OBEX_ObjectGetCommand(handle, NULL);
	if (cmd != OBEX_CMD_PUT && cmd != OBEX_CMD_GET)
		return -EINVAL;

	context->close = 1;

	debug("OBEX_ResumeRequest");
	OBEX_ResumeRequest(handle);

	return 0;
}

int obex_get_response(obex_t *handle)
{
	obex_context_t *context = OBEX_GetUserData(handle);
	int rsp;

	rsp = context->obex_rsp;
	context->obex_rsp = OBEX_RSP_SUCCESS;

	return rsp;
}
