#ifndef HCKR_DISCUSSION_VIEW_H
#define HCKR_DISCUSSION_VIEW_H

#include "core/discussion.h"
#include "tui/mouse.h"
#include <stdint.h>

struct discussion_view {
    size_t offset;
    size_t selected;
    size_t start;
    size_t end;
    size_t lines;
    size_t hovered;
    int follow;
};

size_t discussion_hit(const struct discussion *discussion, struct discussion_view *view,
                      const struct mouse_event *pointer);

size_t discussion_draw(const struct discussion *discussion, struct discussion_view *view,
                       int loading, const char *error, const char *notice,
                       const struct mouse_event *pointer);

#endif
