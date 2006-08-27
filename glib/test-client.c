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
#include <termios.h>

#include "obex-client.h"

#define FTP_UUID (guchar *) \
	"\xF9\xEC\x7B\xC4\x95\x3C\x11\xD2\x98\x4E\x52\x54\x00\xDC\x9E\x09"

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

int main(int argc, char *argv[])
{
	GMainLoop *mainloop;
	ObexClient *client;
	int fd;

	g_type_init();

	fd = open_device("/dev/rfcomm42");
	if (fd < 0) {
		perror("Can't open device");
		exit(EXIT_FAILURE);
	}

	mainloop = g_main_loop_new(NULL, FALSE);


	client = obex_client_new();

	obex_client_set_auto_connect(client, FALSE);

	obex_client_set_fd(client, fd);

	obex_client_connect(client, FTP_UUID, 16, NULL);


	g_main_loop_run(mainloop);

	obex_client_destroy(client);

	close(fd);

	return 0;
}
