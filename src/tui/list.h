#ifndef HCKR_LIST_H
#define HCKR_LIST_H

#include "core/feed.h"
#include "tui/mouse.h"

size_t list_capacity(int height, int comments);
int list_item_at(int row, int height, int comments, size_t offset,
                 size_t count, size_t *item);
void list_draw(const struct feed *feed, size_t offset, size_t article, int comments,
               int loading, const char *error, const char *notice,
               const struct mouse_event *pointer);

#endif
