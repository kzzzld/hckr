#ifndef HCKR_INPUT_H
#define HCKR_INPUT_H

#include <curses.h>

#define KEY_DISCUSSION (KEY_MAX + 1)

void input_start(void);
int input_read(void);
void input_stop(void);

#endif
