/*
 *
 *  OpenOBEX - Free implementation of the Object Exchange protocol
 *
 *  Copyright (C) 2005-2006  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "obex-client.h"

#define FTP_UUID (guchar *) \
	"\xF9\xEC\x7B\xC4\x95\x3C\x11\xD2\x98\x4E\x52\x54\x00\xDC\x9E\x09"

static GMainLoop *mainloop;

static void sig_term(int sig)
{
	g_main_loop_quit(mainloop);
}

static int open_device(const char *device)
{
	struct termios ti;
	int fd;

	fd = open(device, O_RDWR | O_NOCTTY);
	if (fd < 0)
		return fd;

	tcflush(fd, TCIOFLUSH);

	cfmakeraw(&ti);
	tcsetattr(fd, TCSANOW, &ti);

	return fd;
}

static void transfer(ObexClient *client, ObexClientCondition cond, gpointer data)
{
	int *input = data;

	if (cond & OBEX_CLIENT_COND_IN) {
		gchar buf[1024];
		gsize len;

		printf("OBEX_CLIENT_COND_IN\n");

		obex_client_read(client, buf, sizeof(buf), &len, NULL);

		printf("Data buffer with size %zd available\n", len);

		if (len > 0)
			printf("%s\n", buf);
	}

	if (cond & OBEX_CLIENT_COND_OUT) {
		char buf[10000];
		int actual;
		gsize written;

		printf("OBEX_CLIENT_COND_OUT\n");

		if (*input < 0) {
			printf("No data to send!\n");
			return;
		}

		actual = read(*input, buf, sizeof(buf));
		if (actual == 0)
			obex_client_close(client, NULL);
		else if (actual > 0) {
			if (!obex_client_write(client, buf, actual, &written, NULL))
				fprintf(stderr, "writing data failed\n");
		}
		else
			fprintf(stderr, "read: %s\n", strerror(errno));
	}
	
	if (cond & OBEX_CLIENT_COND_DONE) {
		printf("OBEX_CLIENT_COND_DONE\n");

		if (!obex_client_get_error(client, NULL))
			printf("Transfer failed\n");
		else
			printf("Transfer completed\n");
	}
		
	if (cond & OBEX_CLIENT_COND_ERR)
		printf("Error in transfer\n");

		
}

int main(int argc, char *argv[])
{
	ObexClient *client;
	struct sigaction sa;
	int fd, input = -1;

	g_type_init();

	fd = open_device("/dev/rfcomm42");
	if (fd < 0) {
		perror("Can't open device");
		exit(EXIT_FAILURE);
	}

	mainloop = g_main_loop_new(NULL, FALSE);

	client = obex_client_new();

	obex_client_add_watch(client, 0, transfer, &input);

	obex_client_attach_fd(client, fd);

	if (argc > 1) {
		struct stat s;

		if (stat(argv[1], &s) < 0) {
			fprintf(stderr, "stat(%s): %s\n", argv[1], strerror(errno));
			return 1;
		}

		input = open(argv[1], O_RDONLY);
		if (input < 0) {
			fprintf(stderr, "open(%s): %s\n", argv[1], strerror(errno));
			return 1;
		}

		obex_client_put_object(client, NULL, argv[1], s.st_size, s.st_mtime, NULL);
	}
	else
		obex_client_get_object(client, NULL, "telecom/devinfo.txt", NULL);


	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);

	g_main_loop_run(mainloop);

	obex_client_destroy(client);

	if (input >= 0)
		close(input);

	close(fd);

	return 0;
}
