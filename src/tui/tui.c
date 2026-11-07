#include "tui/tui.h"
#include "core/http.h"
#include "core/feed.h"

#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void draw_tabs(const struct hn_section *sections, size_t count,
                      size_t selected)
{
    size_t first = selected;
    size_t width = strlen(sections[selected].label);
    int column = 0;

    while (first > 0) {
        size_t previous_width = strlen(sections[first - 1].label) + 3;
        if (width + previous_width > (size_t)COLS) {
            break;
        }
        width += previous_width;
        --first;
    }

    erase();
    for (size_t i = first; i < count && column < COLS; ++i) {
        if (i != first) {
            mvaddnstr(0, column, " | ", COLS - column);
            column += 3;
        }
        if (column >= COLS) {
            break;
        }
        if (i == selected) {
            attron(A_REVERSE);
        }
        mvaddnstr(0, column, sections[i].label, COLS - column);
        if (i == selected) {
            attroff(A_REVERSE);
        }
        column += (int)strlen(sections[i].label);
    }
}


static void draw_content(const struct feed *feed, size_t offset,
                         int loading, const char *error)
{
    if (LINES < 3 || COLS < 1) {
        refresh();
        return;
    }
    if (loading) {
        mvaddnstr(2, 0, "Loading...", COLS);
    } else if (error[0] != '\0') {
        mvaddnstr(2, 0, error, COLS);
    } else {
        for (int row = 2; row < LINES - 1 && offset < feed->count; ++row, ++offset) {
            mvaddnstr(row, 0, feed->items[offset], COLS);
        }
    }
    char footer[128];
    snprintf(footer, sizeof(footer), "%zu items | j/k scroll | r reload | q quit", feed->count);
    mvaddnstr(LINES - 1, 0, footer, COLS);
    refresh();
}

int tui_run(const struct hn_section *sections, size_t count, size_t selected,
            const struct app_config *config)
{
    int result = 0;
    struct feed feed = {0};
    struct http_request *request = NULL;
    char error[256] = "";
    size_t offset = 0;
    int reload = 1;

    if (sections == NULL || count == 0 || selected >= count) {
        fputs("hckr: no sections configured\n", stderr);
        return 1;
    }

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fputs("hckr: an interactive terminal is required\n", stderr);
        return 1;
    }

    if (initscr() == NULL) {
        fputs("hckr: could not initialize terminal\n", stderr);
        return 1;
    }

    if (cbreak() == ERR || noecho() == ERR || keypad(stdscr, TRUE) == ERR) {
        result = 1;
        goto cleanup;
    }

    curs_set(0);
    timeout(100);

    for (;;) {
        if (reload) {
            http_free(request);
            feed_free(&feed);
            offset = 0;
            error[0] = '\0';
            request = http_start(config->hn_base_url, sections[selected].path,
                                 error, sizeof(error));
            reload = 0;
        }
        if (request != NULL) {
            int status = http_poll(request, error, sizeof(error));
            if (status != 0) {
                if (status > 0) {
                    size_t length;
                    const char *body = http_body(request, &length);
                    feed_parse(body, length, sections[selected].comments,
                               &feed, error, sizeof(error));
                }
                http_free(request);
                request = NULL;
            }
        }
        draw_tabs(sections, count, selected);
        draw_content(&feed, offset, request != NULL, error);
        int key = getch();
        if (key == 'q' || key == 27) {
            break;
        }
        switch (key) {
        case KEY_RIGHT:
        case '\t':
        case 'l':
            selected = (selected + 1) % count;
            reload = 1;
            break;
        case KEY_LEFT:
        case KEY_BTAB:
        case 'h':
            selected = selected == 0 ? count - 1 : selected - 1;
            reload = 1;
            break;
        case 'r':
            reload = 1;
            break;
        case KEY_DOWN:
        case 'j':
            if (offset + 1 < feed.count) {
                ++offset;
            }
            break;
        case KEY_UP:
        case 'k':
            if (offset > 0) {
                --offset;
            }
            break;
        default:
            break;
        }
    }

cleanup:
    http_free(request);
    feed_free(&feed);
    if (endwin() == ERR) {
        result = 1;
    }
    if (result != 0) {
        fputs("hckr: terminal operation failed\n", stderr);
    }
    return result;
}
