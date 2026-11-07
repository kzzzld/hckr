#include "tui/tui.h"
#include "tui/input.h"
#include "tui/discussion.h"
#include "core/http.h"
#include "core/feed.h"
#include "platform/browser.h"

#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum { TITLES_PER_GROUP = 3 };

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


static void draw_content(const struct feed *feed, size_t offset, size_t article,
                         int loading, const char *error, const char *notice)
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
        size_t displayed = 0;
        for (int row = 2; row < LINES - 1 && offset < feed->count; ++offset) {
            if (offset == article) {
                attron(A_REVERSE);
            }
            mvaddnstr(row, 0, feed->items[offset].title, COLS);
            if (offset == article) {
                attroff(A_REVERSE);
            }
            ++displayed;
            row += displayed % TITLES_PER_GROUP == 0 ? 2 : 1;
        }
    }
    char footer[128];
    snprintf(footer, sizeof(footer), "%zu/%zu | j/k select | Enter open | Shift-Enter/c details | r reload | q quit",
             feed->count == 0 ? 0 : article + 1, feed->count);
    mvaddnstr(LINES - 1, 0, notice[0] != '\0' ? notice : footer, COLS);
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
    size_t article = 0;
    struct browser browser = {0};
    char notice[256] = "";
    int reload = 1;
    int input_active = 0;
    int details = 0;
    int detail_reload = 0;
    size_t detail_offset = 0;
    size_t detail_lines = 0;
    struct discussion discussion = {0};
    struct http_request *detail_request = NULL;
    char detail_error[256] = "";

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
    input_start();
    input_active = 1;

    for (;;) {
        if (reload) {
            http_free(request);
            feed_free(&feed);
            offset = 0;
            article = 0;
            notice[0] = '\0';
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
                    feed_parse(body, length, sections[selected].comments, config->hn_base_url,
                               &feed, error, sizeof(error));
                }
                http_free(request);
                request = NULL;
            }
        }
        if (details && detail_reload) {
            http_free(detail_request);
            discussion_free(&discussion);
            detail_error[0] = '\0';
            detail_offset = 0;
            detail_lines = 0;
            detail_request = http_start(config->hn_base_url, feed.items[article].discussion_path,
                                        detail_error, sizeof(detail_error));
            detail_reload = 0;
        }
        if (detail_request != NULL) {
            int status = http_poll(detail_request, detail_error, sizeof(detail_error));
            if (status != 0) {
                if (status > 0) {
                    size_t length;
                    const char *body = http_body(detail_request, &length);
                    discussion_parse(body, length, &discussion, detail_error, sizeof(detail_error));
                }
                http_free(detail_request);
                detail_request = NULL;
            }
        }
        browser_poll(&browser, notice, sizeof(notice));
        size_t rows = LINES > 3 ? (size_t)(LINES - 3) : 1;
        size_t visible = rows - rows / (TITLES_PER_GROUP + 1);
        if (article < offset) {
            offset = article;
        } else if (article - offset >= visible) {
            offset = article - visible + 1;
        }
        if (details) {
            detail_lines = discussion_draw(&discussion, detail_offset,
                                           detail_request != NULL, detail_error, notice);
            size_t height = LINES > 1 ? (size_t)(LINES - 1) : 1;
            size_t maximum = detail_lines > height ? detail_lines - height : 0;
            if (detail_offset > maximum) {
                detail_offset = maximum;
                discussion_draw(&discussion, detail_offset, detail_request != NULL, detail_error, notice);
            }
        } else {
            draw_tabs(sections, count, selected);
            draw_content(&feed, offset, article, request != NULL, error, notice);
        }
        int key = input_read();
        if (key == KEY_CANCEL) {
            break;
        }
        if (key == 'q' || key == 27 || key == KEY_EXIT) {
            if (details) {
                http_free(detail_request);
                detail_request = NULL;
                discussion_free(&discussion);
                details = 0;
                detail_reload = 0;
                continue;
            }
            break;
        }
        if (details) {
            size_t height = LINES > 1 ? (size_t)(LINES - 1) : 1;
            size_t maximum = detail_lines > height ? detail_lines - height : 0;
            if (key != ERR) {
                notice[0] = '\0';
            }
            if ((key == 'j' || key == KEY_DOWN) && detail_offset < maximum) {
                ++detail_offset;
            } else if ((key == 'k' || key == KEY_UP) && detail_offset > 0) {
                --detail_offset;
            } else if (key == 'r') {
                detail_reload = 1;
            } else if (key == '\n' || key == '\r' || key == KEY_ENTER) {
                browser_open(&browser, feed.items[article].url, notice, sizeof(notice));
            }
            continue;
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
        case KEY_DISCUSSION:
        case 'c':
            if (article < feed.count && feed.items[article].discussion_path != NULL) {
                notice[0] = '\0';
                details = 1;
                detail_reload = 1;
            } else if (article < feed.count) {
                snprintf(notice, sizeof(notice), "no discussion link available for this item");
            }
            break;
        case 'r':
            reload = 1;
            break;
        case KEY_DOWN:
        case 'j':
            notice[0] = '\0';
            if (article + 1 < feed.count) {
                ++article;
            }
            break;
        case KEY_UP:
        case 'k':
            notice[0] = '\0';
            if (article > 0) {
                --article;
            }
            break;
        case '\n':
        case '\r':
        case KEY_ENTER:
            if (article < feed.count) {
                notice[0] = '\0';
                browser_open(&browser, feed.items[article].url, notice, sizeof(notice));
            }
            break;
        default:
            break;
        }
    }

cleanup:
    if (input_active) {
        input_stop();
    }
    http_free(detail_request);
    discussion_free(&discussion);
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
