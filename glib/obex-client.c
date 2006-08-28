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

#include "obex-lowlevel.h"
#include "obex-marshal.h"
#include "obex-client.h"

#define OBEX_CLIENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
					OBEX_TYPE_CLIENT, ObexClientPrivate))

typedef struct _ObexClientPrivate ObexClientPrivate;

struct _ObexClientPrivate {
	gboolean auto_connect;

	GMainContext *context;
	GIOChannel *channel;
	obex_t *handle;
};

G_DEFINE_TYPE(ObexClient, obex_client, G_TYPE_OBJECT)

static void obex_client_init(ObexClient *self)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	priv->auto_connect = TRUE;

	priv->context = g_main_context_default();
	g_main_context_ref(priv->context);
}

static void obex_client_finalize(GObject *object)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(object);

	if (priv->auto_connect == TRUE)
		obex_disconnect(priv->handle);

	obex_close(priv->handle);
	priv->handle = NULL;

	g_main_context_unref(priv->context);
	priv->context = NULL;
}

enum {
	PROP_0,
	PROP_AUTO_CONNECT
};

static void obex_client_set_property(GObject *object, guint prop_id,
					const GValue *value, GParamSpec *pspec)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_AUTO_CONNECT:
		priv->auto_connect = g_value_get_boolean(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void obex_client_get_property(GObject *object, guint prop_id,
					GValue *value, GParamSpec *pspec)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_AUTO_CONNECT:
		g_value_set_boolean(value, priv->auto_connect);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

enum {
	CONNECTED_SIGNAL,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void obex_client_class_init(ObexClientClass *klass)
{
#ifdef G_THREADS_ENABLED
	if (!g_thread_supported())
		g_thread_init(NULL);
#endif

	g_type_class_add_private(klass, sizeof(ObexClientPrivate));

	G_OBJECT_CLASS(klass)->finalize = obex_client_finalize;

	G_OBJECT_CLASS(klass)->set_property = obex_client_set_property;
	G_OBJECT_CLASS(klass)->get_property = obex_client_get_property;

	g_object_class_install_property(G_OBJECT_CLASS(klass),
			PROP_AUTO_CONNECT, g_param_spec_boolean("auto-connect",
					NULL, NULL, TRUE, G_PARAM_READWRITE));

	signals[CONNECTED_SIGNAL] = g_signal_new("connected",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, connected),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
}

static gboolean obex_client_callback(GIOChannel *source,
					GIOCondition cond, gpointer data)
{
	ObexClient *self = data;
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	int err;

	//printf("== [source %p]\n", source);

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
		//printf("== [link error]\n");
		return FALSE;
	}

	err = OBEX_HandleInput(priv->handle, 1);

	//printf("== [error %d]\n", err);

	return TRUE;
}

ObexClient *obex_client_new(void)
{
	return OBEX_CLIENT(g_object_new(OBEX_TYPE_CLIENT, NULL));
}

void obex_client_destroy(ObexClient *self)
{
	g_object_unref(self);
}

void obex_client_set_auto_connect(ObexClient *self, gboolean auto_connect)
{
	g_object_set(self, "auto-connect", auto_connect, NULL);
}

gboolean obex_client_get_auto_connect(ObexClient *self)
{
	gboolean auto_connect;

	g_object_get(self, "auto-connect", &auto_connect, NULL);

	return auto_connect;
}

static void obex_connect_cfm(obex_t *handle, void *user_data)
{
	ObexClient *self = user_data;

	g_signal_emit(self, signals[CONNECTED_SIGNAL], 0, NULL);
}

static obex_callback_t callback = {
	.connect_cfm = obex_connect_cfm,
};

void obex_client_attach_fd(ObexClient *self, int fd)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);
	GSource *source;

	priv->handle = obex_open(fd, &callback, self);
	if (priv->handle == NULL)
		return;

	priv->channel = g_io_channel_unix_new(fd);

	source = g_io_create_watch(priv->channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL);

	g_source_set_callback(source, (GSourceFunc) obex_client_callback,
								self, NULL);

	g_source_attach(source, priv->context);

	g_source_unref(source);

	if (priv->auto_connect == TRUE)
		obex_connect(priv->handle, NULL, 0);
}

gboolean obex_client_connect(ObexClient *self, const guchar *target,
						gsize size, GError *error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	if (obex_connect(priv->handle, target, size) < 0)
		return FALSE;

	return TRUE;
}

gboolean obex_client_disconnect(ObexClient *self, GError *error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	if (obex_disconnect(priv->handle) < 0)
		return FALSE;

	return TRUE;
}

gboolean obex_client_get_object(ObexClient *self, const gchar *type,
					const gchar *name, GError *error)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	if (obex_get(priv->handle, type, name) < 0)
		return FALSE;

	return TRUE;
}
