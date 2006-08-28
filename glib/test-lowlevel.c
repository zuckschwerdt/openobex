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

#include "obex-lowlevel.h"

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
	obex_t *handle;
	int fd;

	fd = open_device("/dev/rfcomm42");
	if (fd < 0) {
		perror("Can't open device");
		exit(EXIT_FAILURE);
	}

	handle = obex_open(fd, NULL, NULL);

	obex_connect(handle, NULL, 0);
	obex_poll(handle);

	obex_get(handle, NULL, "telecom/devinfo.txt");
	obex_poll(handle);

	obex_disconnect(handle);
	obex_poll(handle);

	obex_close(handle);

	close(fd);

	return 0;
}
