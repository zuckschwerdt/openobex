/*********************************************************************
 *                
 * Filename:      obex_test_cable.c
 * Version:       0.6
 * Description:   OBEX over a serial port in Linux. 
 *                Can be used with an Ericsson R320s phone.
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus.fuchs@tactel.se>
 * Created at:    Wed Nov 17 22:05:16 1999
 * Modified at:   Sun Aug 13 10:56:03 PM CEST 2000
 * Modified by:   Pontus Fuchs <pontus.fuchs@tactel.se>
 * 
 *     Copyright (c) 1999, 2000 Pontus Fuchs, All Rights Reserved.
 *      
 *     This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU Lesser General Public
 *     License as published by the Free Software Foundation; either
 *     version 2 of the License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *     Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public
 *     License along with this library; if not, write to the Free Software
 *     Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *     MA  02111-1307  USA
 *     
 ********************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include <glib.h>
#include <openobex/obex.h>
#include "obex_test_cable.h"

static void cobex_cleanup(struct cobex_context *gt, gboolean force);

//
// Send an AT-command and return back one line of answer if any.
// To read a line without sending anything set cmd as NULL
// (this function should be rewritten!)
//
gint cobex_do_at_cmd(struct cobex_context *gt, gchar *cmd, gchar *rspbuf, gint rspbuflen, gint timeout)
{
	fd_set ttyset;
	struct timeval tv;
	gint fd;
	
	char *answer = NULL;
	char *answer_end = NULL;
	unsigned int answer_size;

	char tmpbuf[100] = {0,};
	int actual;
	int total = 0;
	int done = 0;
	
	CDEBUG("\n");
	
	fd = gt->ttyfd;
	rspbuf[0] = 0;
	if(fd < 0)
		return -1;

	if(cmd != NULL) {
		// Write command
		gint cmdlen;
		
		cmdlen = strlen(cmd);
		CDEBUG("Sending command %s\n", cmd);
		if(write(fd, cmd, cmdlen) < cmdlen)	{
			perror("Error writing to port");
			return -1;
		}
	}
	
	while(!done)	{
		FD_ZERO(&ttyset);
		FD_SET(fd, &ttyset);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		if(select(fd+1, &ttyset, NULL, NULL, &tv))	{
			actual = read(fd, &tmpbuf[total], sizeof(tmpbuf) - total);
			if(actual < 0)
				return actual;
			total += actual;

//			printf("tmpbuf=%d: %s\n", total, tmpbuf);

			/* Answer didn't come within the length of the buffer. Cancel! */
			if(total == sizeof(tmpbuf))
				return -1;

			if( (answer = index(tmpbuf, '\n')) )	{
				// Remove first line (echo)
				if( (answer_end = index(answer+1, '\n')) )	{
					// Found end of answer
					done = 1;
				}
			}
		}
		else	{
			/* Anser didn't come in time. Cancel */
			CDEBUG("Timeout waiting for answer\n");
			return -1;
		}
	}


//	printf("buf:%08lx answer:%08lx end:%08lx\n", tmpbuf, answer, answer_end);


//	printf("Answer: %s\n", answer);

	// Remove heading and trailing \r
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
//	printf("Answer: %s\n", answer);

	answer_size = (answer_end) - answer +1;

//	printf("Answer size=%d\n", answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;


	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = 0;
	return 0;
}

//
// Open serial port and if we are using an r320, set it in OBEX-mode.
//
static gint cobex_init(struct cobex_context *gt)
{
	char rspbuf[200];

	CDEBUG("\n");

	if( (gt->ttyfd = open(gt->portname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 )	{
		perror("Can' t open tty");
		return -1;
	}

	tcgetattr(gt->ttyfd, &gt->oldtio);
	bzero(&gt->newtio, sizeof(struct termios));
	gt->newtio.c_cflag = B115200 | CS8 | CREAD;
	gt->newtio.c_iflag = IGNPAR;
	gt->newtio.c_oflag = 0;
	tcflush(gt->ttyfd, TCIFLUSH);
	tcsetattr(gt->ttyfd, TCSANOW, &gt->newtio);

	// If we don't speak to an R320s we are happy here.
	if(!gt->r320)
		return 1;

	// Set up R320s phone in OBEX mode.
	if(cobex_do_at_cmd(gt, "ATZ\r", rspbuf, sizeof(rspbuf), 1) < 0)	{
		printf("Comm-error sending ATZ\n");
		goto err;
	}

	if(strcasecmp("OK", rspbuf) != 0)	{
		printf("Error doing ATZ (%s)\n", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(gt, "AT*EOBEX\r", rspbuf, sizeof(rspbuf), 1) < 0)	{
		printf("Comm-error sending AT*EOBEX\n");
		goto err;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0)	{
		printf("Error doing AT*EOBEX (%s)\n", rspbuf);
		goto err;
	}
		return 1;
err:
	cobex_cleanup(gt, TRUE);
	return -1;
}

//
// Close down. If force is TRUE. Try to break out of OBEX-mode.
//
static void cobex_cleanup(struct cobex_context *gt, gboolean force)
{
	if(force)	{
		// Send a break to get out of OBEX-mode
		if(ioctl(gt->ttyfd, TCSBRKP, 0) < 0)	{
			printf("Unable to send break!\n");
		}
	}
	close(gt->ttyfd);
	gt->ttyfd = -1;
}

//
// Open up cable OBEX
//
struct cobex_context * cobex_open(const gchar *port, gboolean r320)
{
	struct cobex_context *gt;
	
	CDEBUG("\n");
	gt = g_new0(struct cobex_context, 1);
	if (gt == NULL)
		return NULL;
	
	gt->ttyfd = -1;
	gt->portname = port;
	gt->r320 = r320;
	return gt;
}

//
// Close down cable OBEX.
//
void cobex_close(struct cobex_context *gt)
{
	g_free(gt);
}
	

//
// Do transport connect or listen
//
gint cobex_connect(obex_t *handle, gpointer userdata)
{
	struct cobex_context *gt;

	CDEBUG("\n");
	
	gt = userdata;

	if(gt->ttyfd >= 0)	{
		CDEBUG("fd already exist. Using it\n");
		return 1;

	}
	if(cobex_init(gt) < 0)
		return -1;
	return 1;
}

//
// Do transport disconnect
//
gint cobex_disconnect(obex_t *handle, gpointer userdata)
{
	gchar rspbuf[20];
	struct cobex_context *gt;

	CDEBUG("\n");
	gt = userdata;

	/* The R320 will send back OK after OBEX disconnect */
	if(gt->r320) {
		CDEBUG("R320!!!\n");
		if(cobex_do_at_cmd(gt, NULL, rspbuf, sizeof(rspbuf), 1) < 0)
			g_print("Comm-error waiting for OK after disconnect\n");
		else if(strcasecmp(rspbuf, "OK") != 0)
			g_print("Excpected OK after OBEX diconnect got %s\n", rspbuf);
	
	}
	
	cobex_cleanup(gt, FALSE);
	return 1;
}

//
//  Called when data needs to be written
//
gint cobex_write(obex_t *handle, gpointer userdata, guint8 *buffer, gint length)
{
	struct cobex_context *gt;
	gint actual;

	CDEBUG("\n");
	gt = userdata;
	actual = write(gt->ttyfd, buffer, length);
	CDEBUG("Wrote %d bytes (expected %d)\n", actual, length);
	return actual;
}

//
// Called when more data is needed.
//
gint cobex_handle_input(obex_t *handle, gpointer userdata, gint timeout)
{
	gint actual;
	struct cobex_context *gt;
	struct timeval time;
	fd_set fdset;
        gint ret;
	
	CDEBUG("\n");

	gt = userdata;
	
	/* Return if no fd */
	if(gt->ttyfd < 0)
		return -1;

	time.tv_sec = timeout;
	time.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(gt->ttyfd, &fdset);

	ret = select(gt->ttyfd+1, &fdset, NULL, NULL, &time);
	
	/* Check if this is a timeout (0) or error (-1) */
	if (ret < 1) {
		return ret;
		CDEBUG("Timeout or error (%d)\n", ret);
	}
	actual = read(gt->ttyfd, &gt->inputbuf, sizeof(gt->inputbuf));
	if(actual <= 0)
		return actual;
	CDEBUG("Read %d bytes\n", actual);
	OBEX_CustomDataFeed(handle, gt->inputbuf, actual);
	return actual;
}
