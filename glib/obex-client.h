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

#ifndef __OBEX_CLIENT_H
#define __OBEX_CLIENT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define OBEX_TYPE_CLIENT (obex_client_get_type())
#define OBEX_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					OBEX_TYPE_CLIENT, ObexClient))
#define OBEX_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					OBEX_TYPE_CLIENT, ObexClientClass))
#define OBEX_IS_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
							OBEX_TYPE_CLIENT))
#define OBEX_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
							OBEX_TYPE_CLIENT))
#define OBEX_GET_CLIENT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					OBEX_TYPE_CLIENT, ObexClientClass))

typedef struct _ObexClient ObexClient;
typedef struct _ObexClientClass ObexClientClass;

struct _ObexClient {
	GObject parent;
};

struct _ObexClientClass {
	GObjectClass parent_class;

	void (*connected)(ObexClient *self);
	void (*disconnect)(ObexClient *self);
	void (*canceled)(ObexClient *self);
	void (*progress)(ObexClient *self);
	void (*idle)(ObexClient *self);
};

GType obex_client_get_type(void);

ObexClient *obex_client_new(void);

void obex_client_destroy(ObexClient *self);

void obex_client_set_auto_connect(ObexClient *self, gboolean auto_connect);

gboolean obex_client_get_auto_connect(ObexClient *self);

typedef enum {
	OBEX_CLIENT_COND_IN	= 1 << 0,
	OBEX_CLIENT_COND_OUT 	= 1 << 1,
	OBEX_CLIENT_COND_ERR 	= 1 << 2,
} ObexClientCondition;

typedef void (*ObexClientFunc)(ObexClient *client,
				ObexClientCondition condition, gpointer data);

void obex_client_add_watch(ObexClient *self, ObexClientCondition condition,
				ObexClientFunc func, gpointer data);

void obex_client_add_watch_full(ObexClient *self,
			ObexClientCondition condition, ObexClientFunc func,
					gpointer data, GDestroyNotify notify);

void obex_client_attach_fd(ObexClient *self, int fd);

gboolean obex_client_connect(ObexClient *self, const guchar *target,
						gsize size, GError **error);

gboolean obex_client_disconnect(ObexClient *self, GError **error);

gboolean obex_client_get_object(ObexClient *self, const gchar *type,
					const gchar *name, GError **error);

gboolean obex_client_read(ObexClient *self, gchar *buf, gsize count,
					gsize *bytes_read, GError **error);

gboolean obex_client_write(ObexClient *self, const gchar *buf, gsize count,
					gsize *bytes_written, GError **error);

gboolean obex_client_close(ObexClient *self, GError **error);

G_END_DECLS

#endif /* __OBEX_CLIENT_H */
