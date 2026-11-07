#ifndef HCKR_SECTIONS_H
#define HCKR_SECTIONS_H

#include <stddef.h>

struct hn_section {
    const char *label;
    const char *path;
    int comments;
};

const struct hn_section *hn_sections(size_t *count);

#endif
