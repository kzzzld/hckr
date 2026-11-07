#ifndef HCKR_INPUT_H
#define HCKR_INPUT_H

#include <curses.h>
#include "tui/mouse.h"

#define KEY_DISCUSSION (KEY_MAX + 1)

void input_start(void);
int input_read(struct mouse_event *mouse);
void input_stop(void);

#endif
