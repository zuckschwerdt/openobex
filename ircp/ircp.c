#include <glib.h>
#include "ircp_client.h"

//
//
//
int main(int argc, char *argv[])
{
	int i;
	ircp_client_t *cli;

	if(argc == 1) {




		g_print("We are server\n");
	}
	else {
		cli = ircp_cli_open();
		if(cli == NULL) {
			g_print("Error opening ircp-client\n");
			return -1;
		}
			
		// Connect
		if(ircp_cli_connect(cli) < 0) {
			g_print("Connect failed\n");
			return -1;
		}
			
		// Dend all files
		for(i = 1; i < argc; i++) {
			ircp_put(cli, argv[i]);
		}

		// Disconnect
		if(ircp_cli_disconnect(cli) < 0) {
			g_print("Disonnect failed\n");
			return -1;
		}
		ircp_cli_close(cli);
	}
	return 0;
}
	