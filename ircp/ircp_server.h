#ifndef IRCP_SERVER_H
#define IRCP_SERVER_H

#include <glib.h>
#include <openobex/obex.h>

typedef struct ircp_server
{
	obex_t *obexhandle;
	gboolean finished;
	gboolean success;
	GString *currdir;
	gchar *origdir;
} ircp_server_t;

gint ircp_srv_got_file(ircp_server_t *srv, obex_object_t *object);
gint ircp_srv_setpath(ircp_server_t *srv, obex_object_t *object);

ircp_server_t *ircp_srv_open();
void ircp_srv_close(ircp_server_t *srv);
gint ircp_srv_recv(ircp_server_t *srv, gchar *inbox);


#endif
