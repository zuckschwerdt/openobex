/*
 *
 *  OpenOBEX - Free implementation of the Object Exchange protocol
 *
 *  Copyright (C) 2005-2006  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "obex-client.h"

#include <glib/gprintf.h>

#include <openobex/obex.h>

G_DEFINE_TYPE(ObexClient, obex_client, G_TYPE_OBJECT)

static void obex_client_init(ObexClient *self)
{
	g_printf("object %p init\n", self);
}

static void obex_client_finalize(GObject *object)
{
	ObexClient *self = OBEX_CLIENT(object);

	g_printf("object %p finalize\n", self);
}

enum {
	PROP_TEST = 1
};

static void obex_client_set_property(GObject *object, guint prop_id,
					const GValue *value, GParamSpec *pspec)
{
	ObexClient *self = OBEX_CLIENT(object);
	gint test;

	g_printf("object %p set property %d\n", self, prop_id);

	switch (prop_id) {
	case PROP_TEST:
		test = g_value_get_int(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void obex_client_get_property(GObject *object, guint prop_id,
					GValue *value, GParamSpec *pspec)
{
	ObexClient *self = OBEX_CLIENT(object);
	gint test = 0;

	g_printf("object %p get property %d\n", self, prop_id);

	switch (prop_id) {
	case PROP_TEST:
		g_value_set_int(value, test);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void obex_client_class_init(ObexClientClass *klass)
{
	g_printf("class init\n");

	G_OBJECT_CLASS(klass)->finalize = obex_client_finalize;

	G_OBJECT_CLASS(klass)->set_property = obex_client_set_property;
	G_OBJECT_CLASS(klass)->get_property = obex_client_get_property;

	g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_TEST, 
			g_param_spec_int("test", NULL, NULL,
				0, G_MAXINT,0 , G_PARAM_READWRITE));
}

ObexClient *obex_client_new(void)
{
	return OBEX_CLIENT(g_object_new(OBEX_TYPE_CLIENT, NULL));
}
