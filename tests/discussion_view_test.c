#include "tui/discussion.h"

#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    setlocale(LC_ALL, "");
    FILE *source = tmpfile(), *sink = tmpfile();
    assert(source != NULL && sink != NULL);
    SCREEN *screen = newterm("xterm-256color", sink, source);
    assert(screen != NULL);
    assert(resizeterm(12, 40) == OK);
    char long_text[2000];
    memset(long_text, 'x', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = '\0';
    struct comment comments[] = {
        { .author = "alice", .text = "First comment", .depth = 0 },
        { .author = "bob", .text = "Reply", .depth = 1 },
        { .author = "carol", .text = long_text, .depth = 2 },
    };
    struct discussion discussion = { .title = "Article", .points = "10 points", .text = "",
                                     .comments = comments, .count = 3 };
    struct discussion_view view = {0};
    struct mouse_event pointer = { .x = -1, .y = -1 };
    discussion_draw(&discussion, &view, 0, 0, "", "", &pointer);
    assert(view.end > view.start && view.start < 11);
    assert(mvwinch(stdscr, (int)view.start, 0) & A_REVERSE);
    pointer = (struct mouse_event){ .x = 2, .y = (int)view.start + 1 };
    assert(discussion_hit(&discussion, &view, &pointer) == 0);
    pointer.y = (int)view.end;
    assert(discussion_hit(&discussion, &view, &pointer) == SIZE_MAX);
    pointer.y = 0;
    assert(discussion_hit(&discussion, &view, &pointer) == SIZE_MAX);

    view.selected = 1;
    view.follow = 1;
    discussion_draw(&discussion, &view, 0, 0, "", "", &pointer);
    assert(view.start >= view.offset && view.end <= view.offset + 11);
    int row = (int)(view.start - view.offset);
    assert(mvwinch(stdscr, row, 2) & A_REVERSE);
    pointer = (struct mouse_event){ .x = 3, .y = row + 1 };
    assert(discussion_hit(&discussion, &view, &pointer) == 1);
    pointer.x = 0;
    assert(discussion_hit(&discussion, &view, &pointer) == SIZE_MAX);

    view.selected = 2;
    view.follow = 1;
    discussion_draw(&discussion, &view, 0, 0, "", "", &pointer);
    assert(view.offset == view.start);
    view.offset += 11;
    size_t offset = view.offset;
    discussion_draw(&discussion, &view, 0, 0, "", "", &pointer);
    assert(view.offset == offset);
    pointer = (struct mouse_event){ .x = 5, .y = 2 };
    assert(discussion_hit(&discussion, &view, &pointer) == 2);
    assert(resizeterm(6, 20) == OK);
    view.follow = 1;
    discussion_draw(&discussion, &view, 0, 0, "", "", &pointer);
    assert(view.offset == view.start);

    assert(resizeterm(12, 40) == OK);
    discussion.text = "First paragraph.\n\nSecond paragraph.";
    view = (struct discussion_view){0};
    discussion_draw(&discussion, &view, 1, 0, "", "", &pointer);
    char text[80];
    mvwinnstr(stdscr, 3, 0, text, 39);
    assert(strncmp(text, "First paragraph.", 16) == 0);
    assert((mvwinch(stdscr, 4, 0) & A_CHARTEXT) == ' ');
    mvwinnstr(stdscr, 5, 0, text, 39);
    assert(strncmp(text, "Second paragraph.", 17) == 0);
    assert(view.lines == 6);
    discussion.text = "";
    discussion_draw(&discussion, &view, 1, 0, "", "", &pointer);
    mvwinnstr(stdscr, 3, 0, text, 39);
    assert(strstr(text, "This Hacker News post has no text.") != NULL);
    discussion.text = long_text;
    view.offset = 10;
    discussion_draw(&discussion, &view, 1, 0, "", "", &pointer);
    assert(view.offset == 10 && view.lines > 11);
    discussion.text = "";

    discussion.count = 0;
    view = (struct discussion_view){0};
    discussion_draw(&discussion, &view, 0, 0, "", "", &pointer);
    assert(discussion_hit(&discussion, &view, &pointer) == SIZE_MAX);
    endwin();
    delscreen(screen);
    fclose(source);
    fclose(sink);
    puts("Comment selection, hit testing, long text, resizing, and empty discussions passed");
    return 0;
}
