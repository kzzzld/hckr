#include "tui/tui.h"
#include "tui/input.h"
#include "tui/list.h"
#include "tui/theme.h"
#include "tui/discussion.h"
#include "core/http.h"
#include "core/feed.h"
#include "platform/browser.h"

#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

static volatile sig_atomic_t interrupted;

static void handle_signal(int signal_number)
{
    interrupted = signal_number;
}

static size_t draw_tabs(const struct hn_section *sections, size_t count,
                      size_t selected, const struct mouse_event *pointer)
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
        int hovered = mouse_over(pointer, column, 0, (int)strlen(sections[i].label));
        if (hovered) {
            attron(A_UNDERLINE);
        }
        if (i == selected) {
            attron(A_REVERSE);
        }
        mvaddnstr(0, column, sections[i].label, COLS - column);
        if (i == selected) {
            attroff(A_REVERSE);
        }
        if (hovered) {
            attroff(A_UNDERLINE);
        }
        column += (int)strlen(sections[i].label);
    }
    return first;
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
    size_t first_tab = 0;
    struct mouse_event pointer = { .x = -1, .y = -1 };
    size_t last_clicked = 0;
    int click_pending = 0;
    struct timespec last_click = {0};
    struct sigaction previous_int;
    struct sigaction previous_term;
    int signals_active = 0;
    char setup_error[256] = "";
    struct theme_state theme = {0};

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

    if (theme_apply(config, &theme, setup_error, sizeof(setup_error)) != 0) {
        result = 1;
        goto cleanup;
    }

    struct sigaction action = { .sa_handler = handle_signal };
    sigemptyset(&action.sa_mask);
    interrupted = 0;
    if (sigaction(SIGINT, &action, &previous_int) != 0) {
        result = 1;
        goto cleanup;
    }
    if (sigaction(SIGTERM, &action, &previous_term) != 0) {
        sigaction(SIGINT, &previous_int, NULL);
        result = 1;
        goto cleanup;
    }
    signals_active = 1;
    curs_set(0);
    timeout(100);
    input_start();
    input_active = 1;

    while (!interrupted) {
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
        size_t visible = list_capacity(LINES, sections[selected].comments);
        if (article < offset) {
            offset = article;
        } else if (article - offset >= visible) {
            offset = article - visible + 1;
        }
        if (details) {
            detail_lines = discussion_draw(&discussion, detail_offset,
                                           detail_request != NULL, detail_error, notice, &pointer);
            size_t height = LINES > 1 ? (size_t)(LINES - 1) : 1;
            size_t maximum = detail_lines > height ? detail_lines - height : 0;
            if (detail_offset > maximum) {
                detail_offset = maximum;
                discussion_draw(&discussion, detail_offset, detail_request != NULL, detail_error, notice, &pointer);
            }
        } else {
            first_tab = draw_tabs(sections, count, selected, &pointer);
            list_draw(&feed, offset, article, sections[selected].comments,
                      request != NULL, error, notice, &pointer);
        }
        struct mouse_event mouse = {0};
        int key = input_read(&mouse);
        if (key == KEY_MOUSE) {
            pointer = mouse;
            if (mouse.x < 0 || mouse.x >= COLS || mouse.y < 0 || mouse.y >= LINES) {
                pointer.x = pointer.y = -1;
                continue;
            }
            if (mouse.motion) {
                continue;
            }
            if (!mouse.released && mouse.button == MOUSE_UP) {
                key = KEY_UP;
                click_pending = 0;
            } else if (!mouse.released && mouse.button == MOUSE_DOWN) {
                key = KEY_DOWN;
                click_pending = 0;
            } else if (mouse.released && mouse.button == MOUSE_RIGHT && details) {
                key = 'q';
            } else if (mouse.released && mouse.button == MOUSE_LEFT && !details) {
                notice[0] = '\0';
                if (mouse.y == 0) {
                    int column = 0;
                    for (size_t i = first_tab; i < count && column < COLS; ++i) {
                        int width = (int)strlen(sections[i].label);
                        if (mouse.x >= column && mouse.x < column + width) {
                            if (selected != i) {
                                selected = i;
                                reload = 1;
                            }
                            break;
                        }
                        column += width + 3;
                    }
                    click_pending = 0;
                    continue;
                }
                if (mouse.y < 2 || mouse.y >= LINES - 1 || request != NULL || error[0] != '\0') {
                    click_pending = 0;
                    continue;
                }
                size_t clicked;
                if (!list_item_at(mouse.y, LINES, sections[selected].comments,
                                  offset, feed.count, &clicked)) {
                    click_pending = 0;
                    continue;
                }
                article = clicked;
                struct timespec now;
                if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
                    click_pending = 0;
                    continue;
                }
                double elapsed = (double)(now.tv_sec - last_click.tv_sec) +
                    (double)(now.tv_nsec - last_click.tv_nsec) / 1000000000.0;
                if (mouse.shift) {
                    key = KEY_DISCUSSION;
                    click_pending = 0;
                } else if (click_pending && last_clicked == clicked && elapsed <= 0.35) {
                    key = KEY_ENTER;
                    click_pending = 0;
                } else {
                    last_clicked = clicked;
                    last_click = now;
                    click_pending = 1;
                    continue;
                }
            } else {
                continue;
            }
        } else if (key != ERR) {
            pointer.x = pointer.y = -1;
            click_pending = 0;
        }
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
    theme_restore(&theme);
    if (endwin() == ERR) {
        result = 1;
    }
    if (result != 0) {
        fprintf(stderr, "hckr: %s\n", setup_error[0] != '\0' ? setup_error : "terminal operation failed");
    }
    if (signals_active) {
        sigaction(SIGINT, &previous_int, NULL);
        sigaction(SIGTERM, &previous_term, NULL);
    }
    return interrupted ? 128 + interrupted : result;
}
