#ifndef HCKR_COLOR_H
#define HCKR_COLOR_H

enum { APP_COLOR_DEFAULT = -1, APP_COLOR_RGB = 0x1000000 };

int color_parse(const char *value, int *color);

#endif
