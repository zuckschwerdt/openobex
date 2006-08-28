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

#include <errno.h>

#include "obex-debug.h"
#include "obex-lowlevel.h"
#include "obex-marshal.h"
#include "obex-client.h"

#define OBEX_CLIENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
					OBEX_TYPE_CLIENT, ObexClientPrivate))

typedef struct _ObexClientPrivate ObexClientPrivate;

struct _ObexClientPrivate {
	gboolean auto_connect;

#ifdef G_THREADS_ENABLED
	GMutex *mutex;
#endif

	GMainContext *context;
	GIOChannel *channel;
	obex_t *handle;

	gpointer watch_data;
	ObexClientFunc watch_func;
	GDestroyNotify watch_destroy;

	gboolean connected;
};

G_DEFINE_TYPE(ObexClient, obex_client, G_TYPE_OBJECT)

static void obex_client_init(ObexClient *self)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

#ifdef G_THREADS_ENABLED
	priv->mutex = g_mutex_new();
#endif

	priv->auto_connect = TRUE;

	priv->context = g_main_context_default();
	g_main_context_ref(priv->context);

	priv->connected = FALSE;
}

static void obex_client_finalize(GObject *object)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(object);

	if (priv->connected == TRUE && priv->auto_connect == TRUE)
		obex_disconnect(priv->handle);

	obex_close(priv->handle);
	priv->handle = NULL;

	g_main_context_unref(priv->context);
	priv->context = NULL;

#ifdef G_THREADS_ENABLED
	g_mutex_free(priv->mutex);
#endif
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
	DISCONNECT_SIGNAL,
	CANCELED_SIGNAL,
	PROGRESS_SIGNAL,
	IDLE_SIGNAL,
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

	signals[DISCONNECT_SIGNAL] = g_signal_new("disconnect",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, disconnect),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[CANCELED_SIGNAL] = g_signal_new("canceled",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, canceled),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[PROGRESS_SIGNAL] = g_signal_new("progress",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, progress),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[IDLE_SIGNAL] = g_signal_new("idle",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET(ObexClientClass, idle),
			NULL, NULL,
			obex_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
}

static gboolean obex_client_callback(GIOChannel *source,
					GIOCondition cond, gpointer data)
{
	ObexClient *self = data;
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
		debug("link error");
		return FALSE;
	}

	if (OBEX_HandleInput(priv->handle, 1) < 0) {
		debug("input error");
	}

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

void obex_client_add_watch_full(ObexClient *self,
			ObexClientCondition condition, ObexClientFunc func,
					gpointer data, GDestroyNotify notify)
{
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	priv->watch_data = data;
	priv->watch_func = func;
	priv->watch_destroy = notify;
}

void obex_client_add_watch(ObexClient *self, ObexClientCondition condition,
				ObexClientFunc func, gpointer data)
{
	obex_client_add_watch_full(self, condition, func, data, NULL);
}

static void obex_connect_cfm(obex_t *handle, void *user_data)
{
	ObexClient *self = user_data;
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	debug("connected");

	priv->connected = TRUE;

	g_signal_emit(self, signals[CONNECTED_SIGNAL], 0, NULL);
}

static void obex_disconn_ind(obex_t *handle, void *user_data)
{
	ObexClient *self = user_data;

	debug("disconnect");

	g_signal_emit(self, signals[DISCONNECT_SIGNAL], 0, NULL);
}

static void obex_progress_ind(obex_t *handle, void *user_data)
{
	ObexClient *self = user_data;
	ObexClientPrivate *priv = OBEX_CLIENT_GET_PRIVATE(self);

	debug("progress");

	if (priv->watch_func)
		priv->watch_func(self, 0, priv->watch_data);

	g_signal_emit(self, signals[PROGRESS_SIGNAL], 0, NULL);
}

static obex_callback_t callback = {
	.connect_cfm  = obex_connect_cfm,
	.disconn_ind  = obex_disconn_ind,
	.progress_ind = obex_progress_ind,
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

	if (priv->connected == FALSE && priv->auto_connect == TRUE)
		obex_connect(priv->handle, NULL, 0);

	if (obex_get(priv->handle, type, name) < 0)
			return FALSE;

	return TRUE;
}

ObexClientError obex_client_read(ObexClient *self, gchar *buf,
					gsize count, gsize *bytes_read)
{
	*bytes_read = 0;

	return OBEX_CLIENT_ERROR_NONE;
}
