#ifndef HCKR_DISCUSSION_VIEW_H
#define HCKR_DISCUSSION_VIEW_H

#include "core/discussion.h"
#include "tui/mouse.h"

size_t discussion_draw(const struct discussion *discussion, size_t offset,
                       int loading, const char *error, const char *notice,
                       const struct mouse_event *pointer);

#endif
