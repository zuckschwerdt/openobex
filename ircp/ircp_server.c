#include <openobex/obex.h>
#include <string.h>

#include "ircp.h"
#include "ircp_io.h"
#include "ircp_server.h"
#include "debug.h"


//
//
//
void srv_obex_event(obex_t *handle, obex_object_t *object, gint mode, gint event, gint obex_cmd, gint obex_rsp)
{
	ircp_server_t *srv;
	int ret;
	
	srv = OBEX_GetUserData(handle);
//	DEBUG(4, G_GNUC_FUNCTION "()\n");

	switch (event)	{
	case OBEX_EV_PROGRESS:
		break;
	case OBEX_EV_REQ:
		DEBUG(4, G_GNUC_FUNCTION "() Incoming request %02x, %d\n", obex_cmd, OBEX_CMD_SETPATH);

		switch(obex_cmd) {
		case OBEX_CMD_CONNECT:
			srv->infocb(IRCP_EV_CONNECTIND, "");	
			ret = 1;
			break;
		case OBEX_CMD_DISCONNECT:
			srv->infocb(IRCP_EV_DISCONNECTIND, "");	
			ret = 1;
			break;

		case OBEX_CMD_PUT:
			ret = ircp_srv_got_file(srv, object);
			break;

		case OBEX_CMD_SETPATH:
			ret = ircp_srv_setpath(srv, object);
			break;
		default:
			ret = 1;
			break;
		}

		if(ret < 0) {
			srv->finished = TRUE;
			srv->success = FALSE;
		}
			
		break;

	case OBEX_EV_REQHINT:
		switch(obex_cmd) {
		case OBEX_CMD_PUT:
		case OBEX_CMD_SETPATH:
		case OBEX_CMD_CONNECT:
		case OBEX_CMD_DISCONNECT:
			OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
			break;
		default:
			DEBUG(0, G_GNUC_FUNCTION "() Skipping unsupported command:%02x\n", obex_cmd);
			OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
			break;
		}
		break;

	case OBEX_EV_REQDONE:
		if(obex_cmd == OBEX_CMD_DISCONNECT) {
			srv->finished = TRUE;
			srv->success = TRUE;
		}
		break;

	case OBEX_EV_LINKERR:
		DEBUG(0, G_GNUC_FUNCTION "() Link error\n");
		srv->finished = TRUE;
		srv->success = FALSE;
		break;
	default:
		break;
	}
}

//
//
//
gint ircp_srv_sync_wait(ircp_server_t *srv)
{
	gint ret;
	while(srv->finished == FALSE) {
		ret = OBEX_HandleInput(srv->obexhandle, 1);
		if (ret < 0)
			return -1;
	}
	if(srv->success)
		return 1;
	else
		return -1;
}

//
//
//
gint ircp_srv_setpath(ircp_server_t *srv, obex_object_t *object)
{
	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;

	guint8 *nonhdr_data = NULL;
	gint nonhdr_data_len;
	gchar *name = NULL;
	int ret = -1;

	DEBUG(4, G_GNUC_FUNCTION "()\n");
	nonhdr_data_len = OBEX_ObjectGetNonHdrData(object, &nonhdr_data);
	if(nonhdr_data_len != 2) {
		DEBUG(0, G_GNUC_FUNCTION "() Error parsing SetPath-command\n");
		return -1;
	}

	while(OBEX_ObjectGetNextHeader(srv->obexhandle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_NAME:
			if( (name = g_malloc(hlen / 2)))	{
				OBEX_UnicodeToChar(name, hv.bs, hlen);
			}
			break;
		default:
			DEBUG(2, G_GNUC_FUNCTION "() Skipped header %02x\n", hi);
		}
	}

	DEBUG(2, G_GNUC_FUNCTION "() Got setpath flags=%d, name=%s\n", nonhdr_data[0], name);

	// If bit 0 is set we shall go up
	if(nonhdr_data[0] & 1) {
		gchar *lastslash;

		if(strcmp(srv->currdir->str, srv->origdir) == 0) {
			DEBUG(0, G_GNUC_FUNCTION "() Trying to go above inbox\n");
			srv->infocb(IRCP_EV_ERRMSG, "Client trying to go above inbox!\n");
			goto out;
		}

		// Find last / in filename and remove enything after it.
		lastslash = strrchr(srv->currdir->str, '/');
		if(lastslash == NULL)
			ret = -1;
		else {
			g_string_truncate(srv->currdir, (lastslash- srv->currdir->str));
			ret = 1;
		}		
	}
	else {
		if(name == NULL)
			goto out;
		// A setpath with empty name meens "goto root"
		if(strcmp(name, "") == 0) {
			g_string_assign(srv->currdir, srv->origdir);
		}
		else {
			DEBUG(4, G_GNUC_FUNCTION "() Going down to %s\n", name);
			// We could use the "create if needed" bit that the standard describes for setpath
			// but irftp in Win2K does not use this bit. Stupid....

			if(ircp_checkdir(srv->currdir->str, name, TRUE) < 0)
				goto out;
			g_string_append(srv->currdir , "/");
			g_string_append(srv->currdir , name);
			DEBUG(4, G_GNUC_FUNCTION "() New directory = %s\n", srv->currdir->str);
		}
	}
				
	ret = 1;

out:
	if(ret < 0)
		OBEX_ObjectSetRsp(object, OBEX_RSP_FORBIDDEN, OBEX_RSP_FORBIDDEN);
	g_free(name);
	return ret;
}


//
// Extract interesting things from object and save to disk.
//
gint ircp_srv_got_file(ircp_server_t *srv, obex_object_t *object)
{
	obex_headerdata_t hv;
	guint8 hi;
	gint hlen;
	const guint8 *body = NULL;
	gint body_len = 0;
	gchar *name = NULL;
	int ret = -1;
	GString *fullname;

	while(OBEX_ObjectGetNextHeader(srv->obexhandle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_BODY:
			body = hv.bs;
			body_len = hlen;
			break;
		case OBEX_HDR_NAME:
			if( (name = g_malloc(hlen / 2)))	{
				OBEX_UnicodeToChar(name, hv.bs, hlen);
			}
			break;
		default:
			DEBUG(4, G_GNUC_FUNCTION "() Skipped header %02x\n", hi);
		}
	}
	if(body == NULL)	{
		DEBUG(0, "Got a PUT without a body\n");
		goto out;
	}
	if(name == NULL)	{
		DEBUG(0, "Got a PUT without a name\n");
		goto out;
	}

	fullname = g_string_new(srv->currdir->str);
	g_string_append(fullname, "/");
	g_string_append(fullname, name);

	srv->infocb(IRCP_EV_RECEIVED, fullname->str);	
	g_string_free(fullname, TRUE);

	ret = ircp_save_file(srv->currdir->str, name, body, body_len);

	if(ret < 0)
		srv->infocb(IRCP_EV_ERR, "");
	else
		srv->infocb(IRCP_EV_OK, "");

out:	g_free(name);
	return ret;
}
	

//
// Create an ircp server
//
ircp_server_t *ircp_srv_open(ircp_info_cb_t infocb)
{
	ircp_server_t *srv;

	DEBUG(4, G_GNUC_FUNCTION "()\n");
	srv = g_malloc0(sizeof(ircp_server_t));
	if(srv == NULL)
		return NULL;

	srv->infocb = infocb;

	srv->currdir = g_string_new("");

#ifdef DEBUG_TCP
	srv->obexhandle = OBEX_Init(OBEX_TRANS_INET, srv_obex_event, 0);
#else
	srv->obexhandle = OBEX_Init(OBEX_TRANS_IRDA, srv_obex_event, 0);
#endif

	if(srv->obexhandle == NULL) {
		g_free(srv);
		return NULL;
	}
	OBEX_SetUserData(srv->obexhandle, srv);
	return srv;
}

//
// Close an ircp server
//
void ircp_srv_close(ircp_server_t *srv)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");
	g_return_if_fail(srv != NULL);

	OBEX_Cleanup(srv->obexhandle);
	g_string_free(srv->currdir, TRUE);
	g_free(srv);
}

//
// Wait for incoming files.
//
gint ircp_srv_recv(ircp_server_t *srv, gchar *inbox)
{
	if(ircp_checkdir("", inbox, FALSE) < 0) {
		srv->infocb(IRCP_EV_ERRMSG, "Specified desination directory does not exist.");
		return -1;
	}


	if(OBEX_ServerRegister(srv->obexhandle, "OBEX:IrXfer") < 0)
		return -1;
	srv->infocb(IRCP_EV_LISTENING, "");
	
	srv->origdir = inbox;
	g_string_assign(srv->currdir, inbox);

	return ircp_srv_sync_wait(srv);
}
