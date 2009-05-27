/**
	\file usbutils.c
	USB OBEX, USB transport for OBEX, support functions.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 2009 Alex Kanavin, All Rights Reserved.

	OpenOBEX is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2.1 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with OpenOBEX. If not, see <http://www.gnu.org/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_USB

#include <string.h>
#include <unistd.h>
#include <stdio.h>		/* perror */
#include <errno.h>		/* errno and EADDRNOTAVAIL */
#include <stdlib.h>
#include "obex_main.h"
#include "usbobex.h"
#include "usbutils.h"

/*
 * Helper function to usbobex_find_interfaces
 */
void find_obex_service_descriptor(const char *buffer, int buflen, obex_usb_intf_service_t **service)
{
	if (!buffer) {
		DEBUG(2, "Weird descriptor references");
		return ;
	}
	while (buflen > 0) {
		if (buffer[1] != USB_DT_CS_INTERFACE) {
			DEBUG(2, "skipping garbage");
			goto next_desc;
		}
		switch (buffer[2]) {
		case CDC_OBEX_SERVICE_ID_TYPE: /* we've found it */
			if (buflen < 22) /* Check descriptor size */
				DEBUG(2, "Invalid service id descriptor");
			else if (*service == NULL) {
				*service = malloc(sizeof(obex_usb_intf_service_t));
				if (*service != NULL) {
					const uint8_t default_uuid[16] = WMC_DEFAULT_OBEX_SERVER_UUID;
					(*service)->role = buffer[3];
					memcpy((*service)->uuid, buffer+4, 16);
					(*service)->version = (buffer[20]<<8)|(buffer[21]);
					if (memcmp((*service)->uuid, default_uuid, 16) == 0 )
						(*service)->is_default_uuid = 1;
					else
						(*service)->is_default_uuid = 0;
				}
			}
			break;
		case CDC_OBEX_TYPE: /* maybe check version */
		case CDC_UNION_TYPE:
		case CDC_HEADER_TYPE:
			break;
		default:
			DEBUG(2, "Ignoring extra header, type %d, length %d", buffer[2], buffer[0]);
			break;
		}
next_desc:
		buflen -= buffer[0];
		buffer += buffer[0];
	}
}

#endif
