#ifndef HCKR_FEED_H
#define HCKR_FEED_H

#include <stddef.h>

struct feed_item {
    char *title;
    char *url;
    char *discussion_path;
};

struct feed {
    struct feed_item *items;
    size_t count;
};

/* Output must be zero-initialized or previously freed. */
int feed_parse(const char *html, size_t length, int comments, const char *base_url,
               struct feed *feed,
               char *error, size_t error_size);
void feed_free(struct feed *feed);

#endif
