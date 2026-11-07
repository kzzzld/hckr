#ifndef HCKR_BROWSER_H
#define HCKR_BROWSER_H

#include <stddef.h>
#include <stdint.h>

struct browser {
    intptr_t child;
};

int browser_open(struct browser *browser, const char *url, char *error, size_t error_size);
/* Returns 0 pending, 1 complete, -1 failed. */
int browser_poll(struct browser *browser, char *error, size_t error_size);

#endif
