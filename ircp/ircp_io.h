#ifndef IRCP_IO_H
#define IRCP_IO_H

#include <glib.h>

obex_object_t *build_object_from_file(obex_t *handle, gchar *localname, gchar *remotename);
gint ircp_save_file(gchar *path, gchar *name, gchar *body, guint len);
gint ircp_changedir(gchar *path, gchar *dir, gboolean create);

#endif /* IRCP_IO_H */
