#include "tui/discussion.h"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct writer {
    size_t line;
    size_t offset;
    const struct mouse_event *pointer;
    struct discussion_view *view;
    size_t comment;
    int render;
};

static void write_line(struct writer *writer, const char *text, size_t length, int indent)
{
    if (writer->render && writer->line >= writer->offset &&
        writer->line - writer->offset < (size_t)(LINES > 1 ? LINES - 1 : 0)) {
        int row = (int)(writer->line - writer->offset);
        int hovered = writer->comment != SIZE_MAX
            ? writer->comment == writer->view->hovered
            : mouse_over(writer->pointer, indent, row, COLS - indent);
        int selected = writer->comment != SIZE_MAX && writer->comment == writer->view->selected;
        if (selected) {
            attron(A_REVERSE);
        }
        if (hovered) {
            attron(A_UNDERLINE);
        }
        mvaddnstr(row, indent, text, (int)length);
        if (hovered) {
            attroff(A_UNDERLINE);
        }
        if (selected) {
            attroff(A_REVERSE);
        }
    }
    ++writer->line;
}

static void write_text(struct writer *writer, const char *text, unsigned depth)
{
    int indent = (int)depth * 2;
    if (indent > COLS / 3) {
        indent = COLS / 3;
    }
    int width = COLS - indent;
    if (*text == '\0') {
        write_line(writer, "", 0, indent);
    }
    while (*text != '\0' && width > 0) {
        if (*text == '\n') {
            write_line(writer, "", 0, indent);
            ++text;
            continue;
        }
        const char *end = text;
        const char *space = NULL;
        int cells = 0;
        mbstate_t state = {0};
        while (*end != '\0' && *end != '\n') {
            wchar_t character;
            size_t bytes = mbrtowc(&character, end, MB_CUR_MAX, &state);
            if (bytes == (size_t)-1 || bytes == (size_t)-2) {
                bytes = 1;
                character = L'?';
                memset(&state, 0, sizeof(state));
            }
            int columns = wcwidth(character);
            if (columns < 0) {
                columns = 1;
            }
            if (cells + columns > width) {
                break;
            }
            if (character == L' ') {
                space = end;
            }
            cells += columns;
            end += bytes;
        }
        if (*end != '\0' && *end != '\n' && space != NULL && space > text) {
            end = space;
        }
        if (end == text) {
            mbstate_t skip_state = {0};
            wchar_t character;
            size_t bytes = mbrtowc(&character, text, MB_CUR_MAX, &skip_state);
            if (bytes == (size_t)-1 || bytes == (size_t)-2 || bytes == 0) {
                bytes = 1;
            }
            write_line(writer, "?", 1, indent);
            text += bytes;
        } else {
            write_line(writer, text, (size_t)(end - text), indent);
            text = *end == '\n' ? end + 1 : end;
        }
        while (*text == ' ') {
            ++text;
        }
    }
}

static void write_discussion(struct writer *writer, const struct discussion *discussion)
{
    if (discussion->title == NULL) {
        return;
    }
    if (writer->render) attron(A_BOLD);
    write_text(writer, discussion->title, 0);
    if (writer->render) attroff(A_BOLD);
    write_text(writer, discussion->points, 0);
    write_text(writer, "Upvotes: not public | Downvotes: not public", 0);
    write_text(writer, "", 0);
    if (discussion->text[0] != '\0') {
        write_text(writer, discussion->text, 0);
        write_text(writer, "", 0);
    }
    char summary[100];
    snprintf(summary, sizeof(summary), "%zu comments on this page%s", discussion->count,
             discussion->more ? " (more available on Hacker News)" : "");
    write_text(writer, summary, 0);
    write_text(writer, "", 0);
    for (size_t i = 0; i < discussion->count; ++i) {
        const struct comment *comment = &discussion->comments[i];
        size_t start = writer->line;
        writer->comment = i;
        if (writer->render) attron(A_BOLD);
        write_text(writer, comment->author, comment->depth);
        if (writer->render) attroff(A_BOLD);
        write_text(writer, comment->text, comment->depth);
        if (i == writer->view->selected) {
            writer->view->start = start;
            writer->view->end = writer->line;
        }
        if (!writer->render && writer->pointer->y >= 0 &&
            writer->pointer->y < LINES - 1 && writer->pointer->x >= 0 &&
            writer->pointer->x < COLS) {
            size_t line = writer->offset + (size_t)writer->pointer->y;
            int indent = (int)comment->depth * 2;
            if (indent > COLS / 3) indent = COLS / 3;
            if (line >= start && line < writer->line && writer->pointer->x >= indent) {
                writer->view->hovered = i;
            }
        }
        writer->comment = SIZE_MAX;
        write_text(writer, "", 0);
    }
}

size_t discussion_hit(const struct discussion *discussion, struct discussion_view *view,
                      const struct mouse_event *pointer)
{
    view->hovered = SIZE_MAX;
    struct writer writer = { .offset = view->offset, .pointer = pointer,
                             .view = view, .comment = SIZE_MAX };
    write_discussion(&writer, discussion);
    view->lines = writer.line;
    return view->hovered;
}

static void write_post(struct writer *writer, const struct discussion *discussion)
{
    if (discussion->title == NULL) {
        return;
    }
    if (writer->render) attron(A_BOLD);
    write_text(writer, discussion->title, 0);
    if (writer->render) attroff(A_BOLD);
    write_text(writer, discussion->points, 0);
    write_text(writer, "", 0);
    write_text(writer, discussion->text[0] != '\0' ? discussion->text :
               "This Hacker News post has no text. Press Enter to open its linked article.", 0);
}

size_t discussion_draw(const struct discussion *discussion, struct discussion_view *view,
                       int post_only, int loading, const char *error, const char *notice,
                       const struct mouse_event *pointer)
{
    erase();
    if (LINES < 2 || COLS < 1) {
        refresh();
        return 0;
    }
    size_t previous_offset = view->offset;
    if (post_only) {
        view->hovered = SIZE_MAX;
        struct writer layout = { .offset = view->offset, .pointer = pointer,
                                 .view = view, .comment = SIZE_MAX };
        write_post(&layout, discussion);
        view->lines = layout.line;
    } else {
        discussion_hit(discussion, view, pointer);
    }
    size_t height = (size_t)(LINES - 1);
    if (!post_only && view->follow && view->selected < discussion->count) {
        if (view->start < view->offset || view->end - view->start > height) {
            view->offset = view->start;
        } else if (view->end > view->offset + height) {
            view->offset = view->end - height;
        }
    }
    view->follow = 0;
    size_t maximum = view->lines > height ? view->lines - height : 0;
    if (view->offset > maximum) view->offset = maximum;
    if (!post_only && view->offset != previous_offset) {
        discussion_hit(discussion, view, pointer);
    }
    struct writer writer = { .offset = view->offset, .pointer = pointer,
                             .view = view, .comment = SIZE_MAX, .render = 1 };
    if (loading) {
        write_text(&writer, post_only ? "Loading post..." : "Loading discussion...", 0);
    } else if (*error != '\0') {
        write_text(&writer, error, 0);
    } else if (post_only) {
        write_post(&writer, discussion);
    } else {
        write_discussion(&writer, discussion);
    }
    int hovered = mouse_over(pointer, 0, LINES - 1, COLS);
    if (hovered) attron(A_UNDERLINE);
    mvaddnstr(LINES - 1, 0, notice[0] != '\0' ? notice :
              (post_only ? "j/k scroll | PgUp/PgDn | Enter browser | d comments | q home" :
               "j/k select | PgUp/PgDn scroll | Enter comment | c post | q home"), COLS);
    if (hovered) attroff(A_UNDERLINE);
    refresh();
    return writer.line;
}
