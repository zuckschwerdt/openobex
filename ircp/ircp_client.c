#include <glib.h>
#include <openobex/obex.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "ircp.h"
#include "ircp_client.h"
#include "ircp_io.h"

#include "dirtraverse.h"
#include "debug.h"

#ifdef DEBUG_TCP
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

//
// Add more data to stream.
//
static gint cli_fillstream(ircp_client_t *cli, obex_object_t *object)
{
	gint actual;
	obex_headerdata_t hdd;
		
	DEBUG(4, G_GNUC_FUNCTION "()\n");
	
	actual = read(cli->fd, cli->buf, STREAM_CHUNK);
	
	DEBUG(4, G_GNUC_FUNCTION "() Read %d bytes\n", actual);
	
	if(actual > 0) {
		/* Read was ok! */
		hdd.bs = cli->buf;
		OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hdd, actual, OBEX_FL_STREAM_DATA);
	}
	else if(actual == 0) {
		/* EOF */
		hdd.bs = cli->buf;
		close(cli->fd);
		cli->fd = -1;
		OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hdd, 0, OBEX_FL_STREAM_DATAEND);
	}
        else {
		/* Error */
		hdd.bs = NULL;
		close(cli->fd);
		cli->fd = -1;
		OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hdd, 0, OBEX_FL_STREAM_DATA);
	}

	return actual;
}


//
// Incoming event from OpenOBEX.
//
static void cli_obex_event(obex_t *handle, obex_object_t *object, gint mode, gint event, gint obex_cmd, gint obex_rsp)
{
	ircp_client_t *cli;

	cli = OBEX_GetUserData(handle);

	switch (event)	{
	case OBEX_EV_PROGRESS:
		break;
	case OBEX_EV_REQDONE:
		cli->finished = TRUE;
		if(obex_rsp == OBEX_RSP_SUCCESS)
			cli->success = TRUE;
		else
			cli->success = FALSE;
		cli->obex_rsp = obex_rsp;
		break;
	
	case OBEX_EV_LINKERR:
		cli->finished = 1;
		cli->success = FALSE;
		break;
	
	case OBEX_EV_STREAMEMPTY:
		cli_fillstream(cli, object);
		break;
	
	default:
		DEBUG(1, G_GNUC_FUNCTION "() Unknown event %d\n", event);
		break;
	}
}

//
// Do an OBEX request sync.
//
static gint cli_sync_request(ircp_client_t *cli, obex_object_t *object)
{
	gint ret;
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	cli->finished = FALSE;
	OBEX_Request(cli->obexhandle, object);

	while(cli->finished == FALSE) {
		ret = OBEX_HandleInput(cli->obexhandle, 20);
		DEBUG(4, G_GNUC_FUNCTION "() ret = %d\n", ret);

		if (ret <= 0)
			return -1;
	}

	DEBUG(4, G_GNUC_FUNCTION "() Done success=%d\n", cli->success);

	if(cli->success)
		return 1;
	else
		return -1;
}
	

//
// Create an ircp client
//
ircp_client_t *ircp_cli_open(ircp_info_cb_t infocb)
{
	ircp_client_t *cli;

	DEBUG(4, G_GNUC_FUNCTION "()\n");
	cli = g_new0(ircp_client_t, 1);
	if(cli == NULL)
		return NULL;

	cli->infocb = infocb;
	cli->fd = -1;

#ifdef DEBUG_TCP
	cli->obexhandle = OBEX_Init(OBEX_TRANS_INET, cli_obex_event, 0);
#else
	cli->obexhandle = OBEX_Init(OBEX_TRANS_IRDA, cli_obex_event, 0);
#endif

	if(cli->obexhandle == NULL) {
		goto out_err;
	}
	OBEX_SetUserData(cli->obexhandle, cli);
	
	/* Buffer for body */
	cli->buf = g_malloc(STREAM_CHUNK);
	return cli;

out_err:
	if(cli->obexhandle != NULL)
		OBEX_Cleanup(cli->obexhandle);
	g_free(cli);
	return NULL;
}
	
//
// Close an ircp client
//
void ircp_cli_close(ircp_client_t *cli)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");
	g_return_if_fail(cli != NULL);

	OBEX_Cleanup(cli->obexhandle);
	g_free(cli->buf);
	g_free(cli);
}

//
// Do connect as client
//
gint ircp_cli_connect(ircp_client_t *cli)
{
	obex_object_t *object;
	int ret;


	DEBUG(4, G_GNUC_FUNCTION "\n");
	g_return_val_if_fail(cli != NULL, -1);

	cli->infocb(IRCP_EV_CONNECTING, "");
#ifdef DEBUG_TCP
	{
		struct sockaddr_in peer;
		u_long inaddr;
		if ((inaddr = inet_addr("127.0.0.1")) != INADDR_NONE) {
			memcpy((char *) &peer.sin_addr, (char *) &inaddr,
			      sizeof(inaddr));  
		}

		ret = OBEX_TransportConnect(cli->obexhandle, (struct sockaddr *) &peer,
					  sizeof(struct sockaddr_in));
	}
		
#else
	ret = IrOBEX_TransportConnect(cli->obexhandle, "OBEX:IrXfer");
#endif
	if (ret < 0) {
		cli->infocb(IRCP_EV_ERR, "");
		return -1;
	}

	object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_CONNECT);
	ret = cli_sync_request(cli, object);

	if(ret < 0)
		cli->infocb(IRCP_EV_ERR, "");
	else
		cli->infocb(IRCP_EV_OK, "");

	return ret;
}

//
// Do disconnect as client
//
gint ircp_cli_disconnect(ircp_client_t *cli)
{
	obex_object_t *object;
	int ret;

	DEBUG(4, G_GNUC_FUNCTION "\n");
	g_return_val_if_fail(cli != NULL, -1);

	cli->infocb(IRCP_EV_DISCONNECTING, "");

	object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_DISCONNECT);
	ret = cli_sync_request(cli, object);

	if(ret < 0)
		cli->infocb(IRCP_EV_ERR, "");
	else
		cli->infocb(IRCP_EV_OK, "");

	OBEX_TransportDisconnect(cli->obexhandle);
	return ret;
}

//
// Do an OBEX PUT.
//
static gint ircp_put_file(ircp_client_t *cli, gchar *localname, gchar *remotename)
{
	obex_object_t *object;
	int ret;

	cli->infocb(IRCP_EV_SENDING, localname);

	DEBUG(4, G_GNUC_FUNCTION "() Sending %s -> %s\n", localname, remotename);
	g_return_val_if_fail(cli != NULL, -1);

	object = build_object_from_file(cli->obexhandle, localname, remotename);
	
	cli->fd = open(localname, O_RDONLY, 0);
	if(cli->fd < 0)
		ret = -1;
	else
		ret = cli_sync_request(cli, object);
	
	close(cli->fd);
		
	if(ret < 0)
		cli->infocb(IRCP_EV_ERR, localname);
	else
		cli->infocb(IRCP_EV_OK, localname);

	return ret;
}

//
// Do OBEX SetPath
//
static gint ircp_setpath(ircp_client_t *cli, gchar *name, gboolean up)
{
	obex_object_t *object;
	obex_headerdata_t hdd;

	guint8 setpath_nohdr_data[2] = {0,};
	gchar *ucname;
	gint ucname_len;
	gint ret;

	DEBUG(4, G_GNUC_FUNCTION "() %s\n", name);

	object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_SETPATH);

	if(up) {
		setpath_nohdr_data[0] = 1;
	}
	else {
		ucname_len = strlen(name)*2 + 2;
		ucname = g_malloc(ucname_len);
		if(ucname == NULL) {
			OBEX_ObjectDelete(cli->obexhandle, object);
			return -1;
		}
		ucname_len = OBEX_CharToUnicode(ucname, name, ucname_len);

		hdd.bs = ucname;
		OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_NAME, hdd, ucname_len, 0);
		g_free(ucname);
	}

	OBEX_ObjectSetNonHdrData(object, setpath_nohdr_data, 2);
	ret = cli_sync_request(cli, object);
	return ret;
}

//
// Callback from dirtraverse.
//
static gint ircp_visit(gint action, gchar *name, gchar *path, gpointer userdata)
{
	gchar *remotename;
	gint ret = -1;

	DEBUG(4, G_GNUC_FUNCTION "()\n");
	switch(action) {
	case VISIT_FILE:
		// Strip /'s before sending file
		remotename = strrchr(name, '/');
		if(remotename == NULL)
			remotename = name;
		else
			remotename++;
		ret = ircp_put_file(userdata, name, remotename);
		break;

	case VISIT_GOING_DEEPER:
		ret = ircp_setpath(userdata, name, FALSE);
		break;

	case VISIT_GOING_UP:
		ret = ircp_setpath(userdata, "", TRUE);
		break;
	}
	DEBUG(4, G_GNUC_FUNCTION "() returning %d\n", ret);
	return ret;
}


//
// Put file or directory
//
gint ircp_put(ircp_client_t *cli, gchar *name)
{
	struct stat statbuf;
	gchar *origdir;
	gint ret;
	
	/* Remember cwd */
	origdir = getcwd(NULL, 0);
	if(origdir == NULL)
		return -1;

	if(stat(name, &statbuf) == -1) {
		return -1;
	}
	
	/* This is a directory. CD into it */
	if(S_ISDIR(statbuf.st_mode)) {
		gchar *newrealdir = NULL;
		gchar *dirname;
		
		chdir(name);
		name = ".";
		
		/* Get real name of new wd, extract last part of and do setpath to it */
		newrealdir = getcwd(NULL, 0);
		dirname = strrchr(newrealdir, '/') + 1;
		if(strlen(dirname) != 0)
			ircp_setpath(cli, dirname, FALSE);
		
		free(newrealdir);
	}
	
	ret = visit_all_files(name, ircp_visit, cli);

	chdir(origdir);
	free(origdir);
	return ret;

}
