/**
	\file usbutils.h
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

#ifndef USBUTILS_H
#define USBUTILS_H

#include <openobex/obex_const.h>

void find_obex_service_descriptor(const char *buffer, int buflen, obex_usb_intf_service_t **service);

#endif

