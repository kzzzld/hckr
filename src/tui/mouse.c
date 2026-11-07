#include "tui/mouse.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

int mouse_parse(const char *report, struct mouse_event *event)
{
    unsigned long values[3];
    const char *cursor = report;
    *event = (struct mouse_event){0};
    for (size_t i = 0; i < 3; ++i) {
        if (!isdigit((unsigned char)*cursor)) {
            return -1;
        }
        char *end;
        errno = 0;
        values[i] = strtoul(cursor, &end, 10);
        if (errno != 0 || values[i] > INT_MAX ||
            (i < 2 && *end != ';')) {
            return -1;
        }
        cursor = i < 2 ? end + 1 : end;
    }
    if ((*cursor != 'M' && *cursor != 'm') || cursor[1] != '\0' ||
        values[1] == 0 || values[2] == 0 || values[0] > 127) {
        return -1;
    }
    unsigned button = (unsigned)values[0];
    event->x = (int)values[1] - 1;
    event->y = (int)values[2] - 1;
    event->released = *cursor == 'm';
    event->shift = (button & 4) != 0;
    if ((button & 32) != 0) {
        return 0;
    }
    switch (button & ~28U) {
    case 0: event->button = MOUSE_LEFT; break;
    case 2: event->button = MOUSE_RIGHT; break;
    case 64: event->button = MOUSE_UP; break;
    case 65: event->button = MOUSE_DOWN; break;
    default: break;
    }
    return 0;
}
