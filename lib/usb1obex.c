/**
	\file usbobex.c
	USB OBEX, USB transport for OBEX, libusb 1.x support.
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

#ifdef HAVE_USB1

#include <string.h>
#include <unistd.h>
#include <stdio.h>		/* perror */
#include <errno.h>		/* errno and EADDRNOTAVAIL */
#include <stdlib.h>

#include <libusb.h>

#include "obex_main.h"
#include "usbobex.h"
#include "usbutils.h"

static struct libusb_context *libusb_ctx = NULL;

/*
 * Function usbobex_prepare_connect (self, interface)
 *
 *    Prepare for USB OBEX connect
 *
 */
void usbobex_prepare_connect(obex_t *self, struct obex_usb_intf_transport_t *intf)
{
	self->trans.self.usb = *intf;
}

/*
 * Helper function to usbobex_find_interfaces
 */
static void find_eps(struct obex_usb_intf_transport_t *intf, struct libusb_interface_descriptor data_intf, int *found_active, int *found_idle)
{
	struct libusb_endpoint_descriptor ep0, ep1;

	if (data_intf.bNumEndpoints == 2) {
		ep0 = data_intf.endpoint[0];
		ep1 = data_intf.endpoint[1];
		if ((ep0.bEndpointAddress & LIBUSB_ENDPOINT_IN) &&
		    ((ep0.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK) &&
		    !(ep1.bEndpointAddress & LIBUSB_ENDPOINT_IN) &&
		    ((ep1.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK)) {
			*found_active = 1;
			intf->data_active_setting = data_intf.bAlternateSetting;
			intf->data_interface_active_description = data_intf.iInterface;
			intf->data_endpoint_read = ep0.bEndpointAddress;
			intf->data_endpoint_write = ep1.bEndpointAddress;
		}
		if (!(ep0.bEndpointAddress & LIBUSB_ENDPOINT_IN) &&
		    ((ep0.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK) &&
		    (ep1.bEndpointAddress & LIBUSB_ENDPOINT_IN) &&
		    ((ep1.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK)) {
			*found_active = 1;
			intf->data_active_setting = data_intf.bAlternateSetting;
			intf->data_interface_active_description = data_intf.iInterface;
			intf->data_endpoint_read = ep1.bEndpointAddress;
			intf->data_endpoint_write = ep0.bEndpointAddress;
		}
	}
	if (data_intf.bNumEndpoints == 0) {
		*found_idle = 1;
		intf->data_idle_setting = data_intf.bAlternateSetting;
		intf->data_interface_idle_description = data_intf.iInterface;
	}
}

/*
 * Helper function to usbobex_find_interfaces
 */
static int find_obex_data_interface(const unsigned char *buffer, int buflen, struct libusb_config_descriptor *config, struct obex_usb_intf_transport_t *intf)
{
	struct cdc_union_desc *union_header = NULL;
	int i, a;
	int found_active = 0;
	int found_idle = 0;

	if (!buffer) {
		DEBUG(2,"Weird descriptor references");
		return -EINVAL;
	}

	while (buflen > 0) {
		if (buffer [1] != USB_DT_CS_INTERFACE) {
			DEBUG(2,"skipping garbage");
			goto next_desc;
		}
		switch (buffer [2]) {
		case CDC_UNION_TYPE: /* we've found it */
			if (union_header) {
				DEBUG(2,"More than one union descriptor, skiping ...");
				goto next_desc;
			}
			union_header = (struct cdc_union_desc *)buffer;
			break;
		case CDC_OBEX_TYPE: /* maybe check version */
		case CDC_OBEX_SERVICE_ID_TYPE: /* This one is handled later */
		case CDC_HEADER_TYPE:
			break; /* for now we ignore it */
		default:
			DEBUG(2, "Ignoring extra header, type %d, length %d", buffer[2], buffer[0]);
			break;
		}
next_desc:
		buflen -= buffer[0];
		buffer += buffer[0];
	}

	if (!union_header) {
		DEBUG(2,"No union descriptor, giving up\n");
		return -ENODEV;
	}
	/* Found the slave interface, now find active/idle settings and endpoints */
	intf->data_interface = union_header->bSlaveInterface0;
	/* Loop through all of the interfaces */
	for (i = 0; i < config->bNumInterfaces; i++) {
		/* Loop through all of the alternate settings */
		for (a = 0; a < config->interface[i].num_altsetting; a++) {
			/* Check if this interface is OBEX data interface*/
			/* and find endpoints */
			if (config->interface[i].altsetting[a].bInterfaceNumber == intf->data_interface)
				find_eps(intf, config->interface[i].altsetting[a], &found_active, &found_idle);
		}
	}
	if (!found_idle) {
		DEBUG(2,"No idle setting\n");
		return -ENODEV;
	}
	if (!found_active) {
		DEBUG(2,"No active setting\n");
		return -ENODEV;
	}

	return 0;
}


/*
 * Helper function to usbobex_find_interfaces
 */
static int get_intf_string(struct libusb_device_handle *usb_handle, char **string, int id)
{
	if (id) {
		if ((*string = malloc(USB_MAX_STRING_SIZE)) == NULL)
			return -ENOMEM;
		*string[0] = '\0';
		return libusb_get_string_descriptor_ascii(usb_handle, id, (unsigned char*)*string, USB_MAX_STRING_SIZE);
	}

	return 0;
}

/*
 * Helper function to usbobex_find_interfaces
 */
static struct obex_usb_intf_transport_t *check_intf(struct libusb_device *dev,
					struct libusb_config_descriptor *conf_desc, int i, int a,
					struct obex_usb_intf_transport_t *current)
{
	struct obex_usb_intf_transport_t *next = NULL;
	if ((conf_desc->interface[i].altsetting[a].bInterfaceClass == USB_CDC_CLASS)
	    && (conf_desc->interface[i].altsetting[a].bInterfaceSubClass == USB_CDC_OBEX_SUBCLASS)) {
		int err;
		const unsigned char *buffer = conf_desc->interface[i].altsetting[a].extra;
		int buflen = conf_desc->interface[i].altsetting[a].extra_length;

		next = malloc(sizeof(struct obex_usb_intf_transport_t));
		if (next == NULL)
			return current;
		next->device = dev;
		libusb_ref_device(dev);
		next->configuration = conf_desc->bConfigurationValue;
		next->configuration_description = conf_desc->iConfiguration;
		next->control_interface = conf_desc->interface[i].altsetting[a].bInterfaceNumber;
		next->control_interface_description = conf_desc->interface[i].altsetting[a].iInterface;
		next->control_setting = conf_desc->interface[i].altsetting[a].bAlternateSetting;
		next->extra_descriptors = malloc(buflen);
		if (next->extra_descriptors)
			memcpy(next->extra_descriptors, buffer, buflen);
		next->extra_descriptors_len = buflen;

		err = find_obex_data_interface(buffer, buflen, conf_desc, next);
		if (err)
			free(next);
		else {
			if (current)
				current->next = next;
			next->prev = current;
			next->next = NULL;
			current = next;
		}
	}
	return current;
}

/*
 * Function usbobex_find_interfaces ()
 *
 *    Find available USBOBEX interfaces on the system
 */
int usbobex_find_interfaces(obex_interface_t **interfaces)
{
	int usbinit_error = 0;
	struct obex_usb_intf_transport_t *current = NULL, *tmp = NULL;
	int i, a, num;
	obex_interface_t *intf_array = NULL;

	if (libusb_ctx == NULL)
		usbinit_error = libusb_init(&libusb_ctx);
	if (usbinit_error == 0) {
		libusb_device **list;
		size_t cnt_dev = libusb_get_device_list(libusb_ctx, &list);
		size_t d = 0;
		for (d = 0; d < cnt_dev; d++) {
			struct libusb_config_descriptor *conf_desc;
			if (libusb_get_active_config_descriptor(list[d], &conf_desc) == 0) {
				for (i = 0; i < conf_desc->bNumInterfaces; i++) {
					for (a = 0; a < conf_desc->interface[i].num_altsetting; a++) {
						current = check_intf(list[d], conf_desc, i, a, current);
					}
				}
				libusb_free_config_descriptor(conf_desc);
			}
		}
		libusb_free_device_list(list, 1);
	}

	num = 0;
	if (current)
		num++;
	while (current && current->prev) {
		current = current->prev;
		num++;
	}
	intf_array = malloc(sizeof(obex_interface_t) * num);
	if (intf_array == NULL)
		goto cleanup_list;
	memset(intf_array, 0, sizeof(obex_interface_t) * num);
	num = 0;
	while (current) {
		intf_array[num].usb.intf = current;
		struct libusb_device_handle *usb_handle;
		if (libusb_open(current->device, &usb_handle) == 0) {
			struct libusb_device_descriptor dev_desc;
			if (libusb_get_device_descriptor(current->device, &dev_desc) == 0) {
				get_intf_string(usb_handle, &intf_array[num].usb.manufacturer,
					dev_desc.iManufacturer);
				get_intf_string(usb_handle, &intf_array[num].usb.product,
					dev_desc.iProduct);
				get_intf_string(usb_handle, &intf_array[num].usb.serial,
					dev_desc.iSerialNumber);
				get_intf_string(usb_handle, &intf_array[num].usb.configuration,
					current->configuration_description);
				get_intf_string(usb_handle, &intf_array[num].usb.control_interface,
					current->control_interface_description);
				get_intf_string(usb_handle, &intf_array[num].usb.data_interface_idle,
					current->data_interface_idle_description);
				get_intf_string(usb_handle, &intf_array[num].usb.data_interface_active,
					current->data_interface_active_description);
				intf_array[num].usb.idVendor = dev_desc.idVendor;
				intf_array[num].usb.idProduct = dev_desc.idProduct;
				intf_array[num].usb.bus_number = libusb_get_bus_number(current->device);
				intf_array[num].usb.device_address = libusb_get_device_address(current->device);
				intf_array[num].usb.interface_number = current->control_interface;
			}
			find_obex_service_descriptor(current->extra_descriptors,
					current->extra_descriptors_len,
					&intf_array[num].usb.service);
			libusb_close(usb_handle);
		}
		current = current->next; num++;
	}
	*interfaces = intf_array;
	return num;

cleanup_list:
	while (current) {
		tmp = current->next;
		free(current);
		current = tmp;
	}
	return 0;
}

/*
 * Function usbobex_free_interfaces ()
 *
 *    Free the list of discovered USBOBEX interfaces on the system
 */
void usbobex_free_interfaces(int num, obex_interface_t *intf)
{
	int i;

	if (intf == NULL)
		return;

	for (i = 0; i < num; i++) {
		free(intf[i].usb.manufacturer);
		free(intf[i].usb.product);
		free(intf[i].usb.serial);
		free(intf[i].usb.configuration);
		free(intf[i].usb.control_interface);
		free(intf[i].usb.data_interface_idle);
		free(intf[i].usb.data_interface_active);
		free(intf[i].usb.service);
		free(intf[i].usb.intf->extra_descriptors);
		libusb_unref_device(intf[i].usb.intf->device);
		free(intf[i].usb.intf);
	}

	free(intf);
	if (libusb_ctx)
		libusb_exit(libusb_ctx);

}

/*
 * Function usbobex_connect_request (self)
 *
 *    Open the USB connection
 *
 */
int usbobex_connect_request(obex_t *self)
{
	int ret;

	DEBUG(4, "\n");

	ret = libusb_open(self->trans.self.usb.device, &self->trans.self.usb.dev);
	if (ret != 0)
		return ret;

	ret = libusb_claim_interface(self->trans.self.usb.dev, self->trans.self.usb.control_interface);
	if (ret < 0) {
		DEBUG(4, "Can't claim control interface %d", ret);
		goto err1;
	}

	ret = libusb_set_interface_alt_setting(self->trans.self.usb.dev, self->trans.self.usb.control_interface, self->trans.self.usb.control_setting);
	if (ret < 0) {
		DEBUG(4, "Can't set control setting %d", ret);
		goto err2;
	}

	ret = libusb_claim_interface(self->trans.self.usb.dev, self->trans.self.usb.data_interface);
	if (ret < 0) {
		DEBUG(4, "Can't claim data interface %d", ret);
		goto err2;
	}

	ret = libusb_set_interface_alt_setting(self->trans.self.usb.dev, self->trans.self.usb.data_interface, self->trans.self.usb.data_active_setting);
	if (ret < 0) {
		DEBUG(4, "Can't set data active setting %d", ret);
		goto err3;
	}

	self->trans.mtu = OBEX_MAXIMUM_MTU;
	DEBUG(2, "transport mtu=%d\n", self->trans.mtu);
	return 1;

err3:
	libusb_release_interface(self->trans.self.usb.dev, self->trans.self.usb.data_interface);
err2:
	libusb_release_interface(self->trans.self.usb.dev, self->trans.self.usb.control_interface);
err1:
	libusb_close(self->trans.self.usb.dev);
	return ret;
}

/*
 * Function usbobex_disconnect_request (self)
 *
 *    Shutdown the USB link
 *
 */
int usbobex_disconnect_request(obex_t *self)
{
	int ret;
	if (self->trans.connected == FALSE)
		return 0;

	DEBUG(4, "\n");

	libusb_clear_halt(self->trans.self.usb.dev, self->trans.self.usb.data_endpoint_read);
	libusb_clear_halt(self->trans.self.usb.dev, self->trans.self.usb.data_endpoint_write);

	ret = libusb_set_interface_alt_setting(self->trans.self.usb.dev, self->trans.self.usb.data_interface, self->trans.self.usb.data_idle_setting);
	if (ret < 0)
		DEBUG(4, "Can't set data idle setting %d", ret);
	ret = libusb_release_interface(self->trans.self.usb.dev, self->trans.self.usb.data_interface);
	if (ret < 0)
		DEBUG(4, "Can't release data interface %d", ret);
	ret = libusb_release_interface(self->trans.self.usb.dev, self->trans.self.usb.control_interface);
	if (ret < 0)
		DEBUG(4, "Can't release control interface %d", ret);
	libusb_close(self->trans.self.usb.dev);
	return 1;
}


#endif /* HAVE_USB1 */
