#include "tui/list.h"

#include <assert.h>
#include <curses.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    assert(list_capacity(24, 0) == 7);
    assert(list_capacity(23, 0) == 7);
    assert(list_capacity(22, 0) == 6);
    assert(list_capacity(24, 1) == 21);
    assert(list_capacity(4, 0) == 1);
    size_t item;
    assert(list_item_at(2, 24, 0, 5, 30, &item) && item == 5);
    assert(list_item_at(3, 24, 0, 5, 30, &item) && item == 5);
    assert(!list_item_at(4, 24, 0, 5, 30, &item));
    assert(list_item_at(5, 24, 0, 5, 30, &item) && item == 6);
    assert(!list_item_at(23, 24, 0, 5, 30, &item));
    assert(!list_item_at(0, 24, 0, 5, 30, &item));
    assert(!list_item_at(5, 24, 0, 5, 6, &item));
    assert(list_item_at(4, 24, 1, 5, 30, &item) && item == 7);

    FILE *source = tmpfile(), *sink = tmpfile();
    assert(source != NULL && sink != NULL);
    SCREEN *screen = newterm("xterm-256color", sink, source);
    assert(screen != NULL);
    struct feed_item items[] = {
        { .title = "First article", .points = 986, .comments = 174 },
        { .title = "Second article", .points = 1, .comments = 0 },
        { .title = "Unavailable", .points = -1, .comments = -1 },
    };
    struct feed feed = { .items = items, .count = 3 };
    struct mouse_event pointer = { .x = 4, .y = 6 };
    list_draw(&feed, 0, 0, 0, 0, "", "", &pointer);
    char text[80];
    mvwinnstr(stdscr, 3, 0, text, 79);
    assert(strncmp(text, "  986 points - 174 comments", 26) == 0);
    mvwinnstr(stdscr, 5, 0, text, 79);
    assert(strncmp(text, "Second article", 14) == 0);
    mvwinnstr(stdscr, 6, 0, text, 79);
    assert(strncmp(text, "  1 point - 0 comments", 22) == 0);
    assert(mvwinch(stdscr, 5, 0) & A_UNDERLINE);
    assert(mvwinch(stdscr, 2, 0) & A_REVERSE);
    assert((mvwinch(stdscr, 4, 0) & A_CHARTEXT) == ' ');
    mvwinnstr(stdscr, 9, 0, text, 79);
    assert(strstr(text, "points unavailable - comments unavailable") != NULL);
    endwin();
    delscreen(screen);
    fclose(source);
    fclose(sink);
    puts("Article metadata layout, selection, hover, and mouse hit testing passed");
    return 0;
}
