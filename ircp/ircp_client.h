#ifndef IRCP_CLIENT_H
#define IRCP_CLIENT_H

#include <openobex/obex.h>


typedef struct ircp_client
{
	obex_t *obexhandle;
	gboolean finished;
	gboolean success;
	gint obex_rsp;
} ircp_client_t;


ircp_client_t *ircp_cli_open();
void ircp_cli_close(ircp_client_t *cli);
gint ircp_cli_connect(ircp_client_t *cli);
gint ircp_cli_disconnect(ircp_client_t *cli);
//gint ircp_put_file(ircp_client_t *cli, gchar *localname, gchar *remotename);
gint ircp_put(ircp_client_t *cli, gchar *name);
	
#endif
