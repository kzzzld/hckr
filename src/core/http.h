#ifndef HCKR_HTTP_H
#define HCKR_HTTP_H

#include <stddef.h>

struct http_request;

int http_init(void);
void http_cleanup(void);
struct http_request *http_start(const char *base_url, const char *path,
                                char *error, size_t error_size);
/* Returns 0 pending, 1 complete, -1 failed. */
int http_poll(struct http_request *request, char *error, size_t error_size);
const char *http_body(const struct http_request *request, size_t *length);
void http_free(struct http_request *request);

#endif
