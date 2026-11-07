#ifndef HCKR_FEED_H
#define HCKR_FEED_H

#include <stddef.h>

struct feed {
    char **items;
    size_t count;
};

/* Output must be zero-initialized or previously freed. */
int feed_parse(const char *html, size_t length, int comments, struct feed *feed,
               char *error, size_t error_size);
void feed_free(struct feed *feed);

#endif
