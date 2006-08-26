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

#define OBEX_CLIENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEX_TYPE_CLIENT, ObexClientPrivate))

typedef struct _ObexClientPrivate ObexClientPrivate;

struct _ObexClientPrivate {
	GMainContext *context;
	GIOChannel *channel;
	int fd;
};

G_DEFINE_TYPE(ObexClient, obex_client, G_TYPE_OBJECT)

static void obex_client_init(ObexClient *self)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	g_printf("object %p init\n", self);

	priv->context = g_main_context_default();

	g_main_context_ref(priv->context);

	priv->fd = -1;
}

static void obex_client_finalize(GObject *object)
{
	ObexClient *self = OBEX_CLIENT(object);
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	g_printf("object %p finalize\n", self);

	priv->fd = -1;

	g_main_context_unref(priv->context);
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

#ifdef G_THREADS_ENABLED
	if (!g_thread_supported())
		g_thread_init(NULL);
#endif

	g_type_class_add_private(klass, sizeof(ObexClientPrivate));

	G_OBJECT_CLASS(klass)->finalize = obex_client_finalize;

	G_OBJECT_CLASS(klass)->set_property = obex_client_set_property;
	G_OBJECT_CLASS(klass)->get_property = obex_client_get_property;

	g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_TEST, 
			g_param_spec_int("test", NULL, NULL,
				0, G_MAXINT,0 , G_PARAM_READWRITE));
}

static gboolean obex_client_callback(GIOChannel *source,
					GIOCondition cond, gpointer data)
{
	ObexClientPrivate *priv = data;

	if (priv->fd < 0)
		return FALSE;

	return TRUE;
}

ObexClient *obex_client_new(void)
{
	return OBEX_CLIENT(g_object_new(OBEX_TYPE_CLIENT, NULL));
}

void obex_client_set_fd(ObexClient *self, int fd)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	GSource *source;

	priv->channel = g_io_channel_unix_new(fd);

	source = g_io_create_watch(priv->channel,
		G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL);

	g_source_set_callback(source, (GSourceFunc) obex_client_callback,
								priv, NULL);

	g_source_attach(source, priv->context);

	g_source_unref(source);

	priv->fd = fd;
}
