#ifndef HCKR_MOUSE_H
#define HCKR_MOUSE_H

#include <stddef.h>

enum mouse_button {
    MOUSE_NONE,
    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_UP,
    MOUSE_DOWN,
};

struct mouse_event {
    enum mouse_button button;
    int x;
    int y;
    int released;
    int shift;
};

int mouse_parse(const char *report, struct mouse_event *event);

#endif
