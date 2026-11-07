#include "tui/list.h"

#include <curses.h>
#include <stdio.h>

enum { CONTENT_TOP = 2, ARTICLE_ROWS = 3 };

size_t list_capacity(int height, int comments)
{
    int rows = height - CONTENT_TOP - 1;
    if (rows < 2) {
        return 1;
    }
    return comments ? (size_t)rows : (size_t)(rows + 1) / ARTICLE_ROWS;
}

int list_item_at(int row, int height, int comments, size_t offset,
                 size_t count, size_t *item)
{
    if (row < CONTENT_TOP || row >= height - 1) {
        return 0;
    }
    size_t index = (size_t)(row - CONTENT_TOP);
    if (!comments) {
        if (index % ARTICLE_ROWS == ARTICLE_ROWS - 1) {
            return 0;
        }
        index /= ARTICLE_ROWS;
    }
    if (index >= list_capacity(height, comments) || offset >= count || index >= count - offset) {
        return 0;
    }
    *item = offset + index;
    return 1;
}

static void format_metrics(const struct feed_item *item, char *text, size_t size)
{
    char points[64], comments[64];
    if (item->points < 0) {
        snprintf(points, sizeof(points), "points unavailable");
    } else {
        snprintf(points, sizeof(points), "%ld %s", item->points, item->points == 1 ? "point" : "points");
    }
    if (item->comments < 0) {
        snprintf(comments, sizeof(comments), "comments unavailable");
    } else {
        snprintf(comments, sizeof(comments), "%ld %s", item->comments,
                 item->comments == 1 ? "comment" : "comments");
    }
    snprintf(text, size, "  %s - %s", points, comments);
}

void list_draw(const struct feed *feed, size_t offset, size_t article, int comments,
               int loading, const char *error, const char *notice,
               const struct mouse_event *pointer)
{
    if (LINES < 3 || COLS < 1) {
        refresh();
        return;
    }
    if (loading) {
        mvaddnstr(CONTENT_TOP, 0, "Loading...", COLS);
    } else if (error[0] != '\0') {
        mvaddnstr(CONTENT_TOP, 0, error, COLS);
    } else {
        size_t hovered_item = 0;
        int hovering = pointer->x >= 0 && pointer->x < COLS &&
            list_item_at(pointer->y, LINES, comments, offset, feed->count, &hovered_item);
        size_t capacity = list_capacity(LINES, comments);
        int step = comments ? 1 : ARTICLE_ROWS;
        for (size_t i = 0; i < capacity && offset < feed->count; ++i, ++offset) {
            int row = CONTENT_TOP + (int)i * step;
            if (row >= LINES - 1) {
                break;
            }
            int hovered = hovering && hovered_item == offset;
            if (hovered) {
                attron(A_UNDERLINE);
            }
            if (offset == article) {
                attron(A_REVERSE);
            }
            mvaddnstr(row, 0, feed->items[offset].title, COLS);
            if (offset == article) {
                attroff(A_REVERSE);
            }
            if (!comments && row + 1 < LINES - 1) {
                char metrics[140];
                format_metrics(&feed->items[offset], metrics, sizeof(metrics));
                attron(A_DIM);
                mvaddnstr(row + 1, 0, metrics, COLS);
                attroff(A_DIM);
            }
            if (hovered) {
                attroff(A_UNDERLINE);
            }
        }
    }
    char footer[128];
    snprintf(footer, sizeof(footer), "%zu/%zu | j/k select | Enter open | Shift-Enter/c details | r reload | q quit",
             feed->count == 0 ? 0 : article + 1, feed->count);
    int hovered = mouse_over(pointer, 0, LINES - 1, COLS);
    if (hovered) {
        attron(A_UNDERLINE);
    }
    mvaddnstr(LINES - 1, 0, notice[0] != '\0' ? notice : footer, COLS);
    if (hovered) {
        attroff(A_UNDERLINE);
    }
    refresh();
}
