#include "tui/input.h"

#include <stdio.h>

enum { KEY_MOUSE_REPORT = KEY_MAX + 2, KEY_MOUSE_LEGACY };

void input_start(void)
{
    const int arrows[] = { KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT };
    for (size_t i = 0; i < sizeof(arrows) / sizeof(arrows[0]); ++i) {
        char sequence[20];
        char direction = "ABCD"[i];
        snprintf(sequence, sizeof(sequence), "\033[%c", direction);
        define_key(sequence, arrows[i]);
        snprintf(sequence, sizeof(sequence), "\033O%c", direction);
        define_key(sequence, arrows[i]);
        for (int modifier = 1; modifier <= 8; ++modifier) {
            snprintf(sequence, sizeof(sequence), "\033[1;%d%c", modifier, direction);
            define_key(sequence, arrows[i]);
        }
    }
    define_key("\033[<", KEY_MOUSE_REPORT);
    define_key("\033[M", KEY_MOUSE_LEGACY);
    define_key("\033[13;2u", KEY_DISCUSSION);
    define_key("\033[27;2;13~", KEY_DISCUSSION);
    define_key("\033[13u", KEY_ENTER);
    define_key("\033[27u", KEY_EXIT);
    define_key("\033[99;5u", KEY_CANCEL);
    fputs("\033[>1u\033[?1000h\033[?1006h\033[?1003h", stdout);
    fflush(stdout);
}

int input_read(struct mouse_event *mouse)
{
    int key = getch();
    if (key == KEY_MOUSE_LEGACY) {
        int bytes[3];
        for (size_t i = 0; i < 3; ++i) {
            bytes[i] = getch();
            if (bytes[i] < 32 || bytes[i] > 255) {
                return ERR;
            }
        }
        unsigned button = (unsigned)(bytes[0] - 32);
        struct mouse_event ignored;
        struct mouse_event *event = mouse != NULL ? mouse : &ignored;
        *event = (struct mouse_event){
            .x = bytes[1] - 33, .y = bytes[2] - 33,
            .shift = (button & 4) != 0,
            .motion = (button & 32) != 0,
        };
        if ((button & 32) != 0) {
            return KEY_MOUSE;
        }
        switch (button & ~28U) {
        case 0: event->button = MOUSE_LEFT; event->released = 1; break;
        case 2: event->button = MOUSE_RIGHT; event->released = 1; break;
        case 64: event->button = MOUSE_UP; break;
        case 65: event->button = MOUSE_DOWN; break;
        default: break;
        }
        return KEY_MOUSE;
    }
    if (key == KEY_MOUSE_REPORT) {
        char report[64];
        for (size_t i = 0; i < sizeof(report) - 1; ++i) {
            int byte = getch();
            if (byte == ERR || byte < 0 || byte > 127) {
                return ERR;
            }
            report[i] = (char)byte;
            if (byte == 'M' || byte == 'm') {
                report[i + 1] = '\0';
                struct mouse_event ignored;
                return mouse_parse(report, mouse != NULL ? mouse : &ignored) == 0
                    ? KEY_MOUSE : ERR;
            }
        }
        return ERR;
    }
    if (key != 27) {
        return key;
    }
    int next = getch();
    if (next == ERR) {
        return 27;
    }
    if (next == '[' || next == 'O') {
        for (int i = 0; i < 64; ++i) {
            int byte = getch();
            if (byte == ERR || (byte >= 0x40 && byte <= 0x7e)) {
                break;
            }
        }
    }
    return ERR;
}

void input_stop(void)
{
    fputs("\033[?1003l\033[?1006l\033[?1000l\033[<u", stdout);
    fflush(stdout);
}
