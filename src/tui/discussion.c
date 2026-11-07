#include "tui/discussion.h"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct writer {
    size_t line;
    size_t offset;
};

static void write_line(struct writer *writer, const char *text, size_t length, int indent)
{
    if (writer->line >= writer->offset &&
        writer->line - writer->offset < (size_t)(LINES > 1 ? LINES - 1 : 0)) {
        mvaddnstr((int)(writer->line - writer->offset), indent, text, (int)length);
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
        const char *end = text;
        const char *space = NULL;
        int cells = 0;
        mbstate_t state = {0};
        while (*end != '\0') {
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
        if (*end != '\0' && space != NULL && space > text) {
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
            text = end;
        }
        while (*text == ' ') {
            ++text;
        }
    }
}

size_t discussion_draw(const struct discussion *discussion, size_t offset,
                       int loading, const char *error, const char *notice)
{
    erase();
    if (LINES < 2 || COLS < 1) {
        refresh();
        return 0;
    }
    struct writer writer = { .offset = offset };
    if (loading) {
        write_text(&writer, "Loading discussion...", 0);
    } else if (*error != '\0') {
        write_text(&writer, error, 0);
    } else if (discussion->title != NULL) {
        attron(A_BOLD);
        write_text(&writer, discussion->title, 0);
        attroff(A_BOLD);
        write_text(&writer, discussion->points, 0);
        write_text(&writer, "Upvotes: not public | Downvotes: not public", 0);
        write_text(&writer, "", 0);
        if (discussion->text[0] != '\0') {
            write_text(&writer, discussion->text, 0);
            write_text(&writer, "", 0);
        }
        char summary[100];
        snprintf(summary, sizeof(summary), "%zu comments on this page%s", discussion->count,
                 discussion->more ? " (more available on Hacker News)" : "");
        write_text(&writer, summary, 0);
        write_text(&writer, "", 0);
        for (size_t i = 0; i < discussion->count; ++i) {
            const struct comment *comment = &discussion->comments[i];
            attron(A_BOLD);
            write_text(&writer, comment->author, comment->depth);
            attroff(A_BOLD);
            write_text(&writer, comment->text, comment->depth);
            write_text(&writer, "", 0);
        }
    }
    mvaddnstr(LINES - 1, 0, notice[0] != '\0' ? notice :
              "j/k scroll | Enter browser | r reload | q home", COLS);
    refresh();
    return writer.line;
}
