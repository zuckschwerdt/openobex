#ifndef IRCP_IO_H
#define IRCP_IO_H

#include <glib.h>

obex_object_t *build_object_from_file(obex_t *handle, const gchar *localname, const gchar *remotename);
gint ircp_save_file(const gchar *path, const gchar *name, const gchar *body, guint len);
gint ircp_checkdir(const gchar *path, const gchar *dir, gboolean create);

#endif /* IRCP_IO_H */
