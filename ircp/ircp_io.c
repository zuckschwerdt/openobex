#include <sys/stat.h>
#include <unistd.h>

#include <fcntl.h>
#include <string.h>

#include <glib.h>
#include <openobex/obex.h>

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
	g_print("name=%s, size=%d\n", filename, *file_size);

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
