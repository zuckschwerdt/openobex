#ifndef OBEX_TEST_SERVER_H
#define OBEX_TEST_SERVER_H

void server_do(obex_t *handle);
void server_done(obex_t *handle, obex_object_t *object, gint obex_cmd, gint obex_rsp);
void server_request(obex_t *handle, obex_object_t *object, gint event, gint cmd);

#endif
