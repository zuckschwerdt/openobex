#include <glib.h>
#include <openobex/obex.h>

#include <string.h>



#include "ircp_client.h"
#include "ircp_io.h"

#include "dirtraverse.h"
#include "debug.h"
#ifdef DEBUG_TCP
#include <sys/stat.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif


//
//
//
void cli_obex_event(obex_t *handle, obex_object_t *object, gint mode, gint event, gint obex_cmd, gint obex_rsp)
{
	ircp_client_t *cli;

	cli = OBEX_GetUserData(handle);
//	DEBUG(4, G_GNUC_FUNCTION "\n");

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
	default:
		break;
	}
}

//
//
//
gint cli_sync_request(ircp_client_t *cli, obex_object_t *object)
{
	gint ret;
	DEBUG(4, G_GNUC_FUNCTION "()\n");

	cli->finished = FALSE;
	OBEX_Request(cli->obexhandle, object);

	while(cli->finished == FALSE) {
		ret = OBEX_HandleInput(cli->obexhandle, 20);
		if (ret <= 0)
			return -1;
	}
	if(cli->success)
		return 1;
	else
		return -1;
}
	

//
// Create an ircp client
//
ircp_client_t *ircp_cli_open()
{
	ircp_client_t *cli;

	DEBUG(4, G_GNUC_FUNCTION "()\n");
	cli = g_malloc0(sizeof(ircp_client_t));

#ifdef DEBUG_TCP
	cli->obexhandle = OBEX_Init(OBEX_TRANS_INET, cli_obex_event, 0);
#else
	cli->obexhandle = OBEX_Init(OBEX_TRANS_IRDA, cli_obex_event, 0);
#endif

	if(cli->obexhandle == NULL) {
		g_free(cli);
		return NULL;
	}
	OBEX_SetUserData(cli->obexhandle, cli);
	return cli;
}
	
//
// Close an ircp client
//
void ircp_cli_close(ircp_client_t *cli)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");
	g_return_if_fail(cli != NULL);

	OBEX_Cleanup(cli->obexhandle);
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
		return -1;
	}

	object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_CONNECT);
	ret = cli_sync_request(cli, object);
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

	object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_DISCONNECT);
	ret = cli_sync_request(cli, object);

	OBEX_TransportDisconnect(cli->obexhandle);
	return ret;
}

//
//
//
gint ircp_put_file(ircp_client_t *cli, gchar *localname, gchar *remotename)
{
	obex_object_t *object;
	int ret;

	DEBUG(4, G_GNUC_FUNCTION "() Sending %s -> %s\n", localname, remotename);
	g_return_val_if_fail(cli != NULL, -1);

	object = build_object_from_file(cli->obexhandle, localname, remotename);
	ret = cli_sync_request(cli, object);
	return ret;
}

//
//
//
gint ircp_setpath(ircp_client_t *cli, gchar *name, gboolean up)
{
	obex_object_t *object;
	obex_headerdata_t hdd;

	guint8 setpath_nohdr_data[2] = {0,};
	gchar *ucname;
	gint ucname_len;
	gint ret;

	DEBUG(4, G_GNUC_FUNCTION "()\n");

	object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_SETPATH);

	if(up) {
		setpath_nohdr_data[0] = 1;
	}
	else {
		setpath_nohdr_data[0] = 2;		//Tell server to create dir if it does not exist.

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
//
//
gint ircp_visit(gint action, gchar *name, gchar *path, gpointer userdata)
{
	gchar *remotename;
	gint ret = -1;

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
	return ret;
}


//
// Put file or directory
//
gint ircp_put(ircp_client_t *cli, gchar *name)
{
	return visit_all_files(name, ircp_visit, cli);
}
