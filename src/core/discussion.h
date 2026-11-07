#ifndef HCKR_DISCUSSION_H
#define HCKR_DISCUSSION_H

#include <stddef.h>

struct comment {
    char *author;
    char *text;
    unsigned depth;
};

struct discussion {
    char *title;
    char *points;
    char *text;
    struct comment *comments;
    size_t count;
    int more;
};

int discussion_parse(const char *html, size_t length, struct discussion *discussion,
                     char *error, size_t error_size);
void discussion_free(struct discussion *discussion);

#endif
