#include <sys/stat.h>
#include <unistd.h>

#include <fcntl.h>
#include <string.h>

#include <glib.h>
#include <openobex/obex.h>

#include "debug.h"

//
// Get the filesize.
//
gint get_filesize(char *filename)
{
	struct stat stats;

	stat(filename, &stats);
	return (gint) stats.st_size;
}


//
// Read a file and alloc a buffer for it
//
guint8* easy_readfile(char *filename, int *file_size)
{
	int actual;
	int fd;
	guint8 *buf;

	*file_size = get_filesize(filename);
	DEBUG(4, G_GNUC_FUNCTION "()name=%s, size=%d\n", filename, *file_size);

	fd = open(filename, O_RDONLY, 0);

	if (fd == -1) {
		return NULL;
	}
	
	if(! (buf = g_malloc(*file_size)) )	{
		return NULL;
	}

	actual = read(fd, buf, *file_size);
	close(fd); 

	*file_size = actual;
	return buf;
}


//
//
//
obex_object_t *build_object_from_file(obex_t *handle, gchar *localname, gchar *remotename)
{
	obex_object_t *object = NULL;
	obex_headerdata_t hdd;
	guint8 *ucname;
	gint ucname_len;
	int file_size;
	guint8 *buf;

	buf = easy_readfile(localname, &file_size);
	if(buf == NULL)
		return NULL;

	object = OBEX_ObjectNew(handle, OBEX_CMD_PUT);
	if(object == NULL)
		goto err;

	ucname_len = strlen(remotename)*2 + 2;
	ucname = g_malloc(ucname_len);
	if(ucname == NULL)
		goto err;

	ucname_len = OBEX_CharToUnicode(ucname, remotename, ucname_len);

	hdd.bs = ucname;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hdd, ucname_len, 0);
	g_free(ucname);

	hdd.bq4 = file_size;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hdd, sizeof(guint32), 0);

#if 0
	/* Optional header for win95 irxfer, allows date to be set on file */
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_TIME2,
				(obex_headerdata_t) (guint32) stats.st_mtime,
				sizeof(guint32), 0);
#endif

	hdd.bs = buf;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY,
				hdd, file_size, 0);

	g_free(buf);
	return object;

err:
	g_free(buf);
	if(object != NULL)
		OBEX_ObjectDelete(handle, object);
	return NULL;
}

//
// Check for dangerous filenames (TODO: Make better)
//
gboolean ircp_nameok(gchar *name)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");
	if(name[0] == '/')
		return FALSE;

	if(strlen(name) == 3 && name[0] == '.' && name[1] == '.' && name[2] == '/')
		return FALSE;

	return TRUE;
}
	


//
// Save a file...
//
gint ircp_save_file(gchar *path, gchar *name, gchar *body, guint len)
{
	GString *diskname;
	gint fd, ret;

	DEBUG(4, G_GNUC_FUNCTION "()\n");
	//Check for dangerous filenames
	if(ircp_nameok(name) == FALSE)
		return -1;

	diskname = g_string_new(path);

	if(diskname->len > 0)
		g_string_append(diskname, "/");
	g_string_append(diskname, name);

	DEBUG(4, G_GNUC_FUNCTION "() Going to save with name %s\n", diskname->str);

	fd = open(diskname->str, O_RDWR | O_CREAT | O_TRUNC, DEFFILEMODE);

	if ( fd < 0) {
		ret = -1;
		goto out;
	}

	ret = write(fd, body, len);
	close(fd);

	DEBUG( 4, G_GNUC_FUNCTION "() Wrote %s (%d bytes)\n", diskname->str, ret);

out:
	g_string_free(diskname, TRUE);
	return ret;
}

//
// Go to a directory. Create if not exists and create is true.
//
gint ircp_changedir(gchar *path, gchar *dir, gboolean create)
{
	GString *newpath;
	struct stat statbuf;
	gint ret = -1;

	if(ircp_nameok(dir) == FALSE)
		return -1;

	newpath = g_string_new(path);
	g_string_append(newpath, "/");
	g_string_append(newpath, dir);

	DEBUG(4, G_GNUC_FUNCTION "() path = %s dir = %s, create = %d\n", path, dir, create);
	if(stat(newpath->str, &statbuf) == 0) {
		// If this directory aleady exist we are happy
		if(S_ISDIR(statbuf.st_mode)) {
			DEBUG(4, G_GNUC_FUNCTION "() Using existing dir\n");
			ret = 1;
			goto out;
		}
		else  {
			// A non-directory with this name already exist.
			DEBUG(4, G_GNUC_FUNCTION "() A non-dir called %s already exist\n", newpath->str);
			ret = -1;
			goto out;
		}
	}
	if(create) {
		DEBUG(4, G_GNUC_FUNCTION "() Will try to create %s\n", newpath->str);
		ret = mkdir(newpath->str, DEFFILEMODE | S_IXGRP | S_IXUSR | S_IXOTH);
	}
	else {
		ret = -1;
	}

out:	g_string_free(newpath, TRUE);
	return ret;
}
	