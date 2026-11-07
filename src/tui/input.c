#include "tui/input.h"

#include <stdio.h>

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
    define_key("\033[13;2u", KEY_DISCUSSION);
    define_key("\033[27;2;13~", KEY_DISCUSSION);
    define_key("\033[13u", KEY_ENTER);
    define_key("\033[27u", KEY_EXIT);
    define_key("\033[99;5u", KEY_CANCEL);
    fputs("\033[>1u", stdout);
    fflush(stdout);
}

int input_read(void)
{
    int key = getch();
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
    fputs("\033[<u", stdout);
    fflush(stdout);
}
