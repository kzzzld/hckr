#include "core/color.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int color_parse(const char *value, int *color)
{
    static const char *names[] = {
        "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
    };
    if (strcmp(value, "default") == 0) {
        *color = -1;
        return 0;
    }
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        if (strcmp(value, names[i]) == 0) {
            *color = (int)i;
            return 0;
        }
    }
    if (value[0] == '#' && strlen(value) == 7) {
        for (size_t i = 1; i < 7; ++i) {
            if (!isxdigit((unsigned char)value[i])) {
                return -1;
            }
        }
        *color = APP_COLOR_RGB | (int)strtol(value + 1, NULL, 16);
        return 0;
    }
    if (!isdigit((unsigned char)value[0])) {
        return -1;
    }
    char *end;
    errno = 0;
    long index = strtol(value, &end, 10);
    if (errno != 0 || *end != '\0' || index < 0 || index > 255) {
        return -1;
    }
    *color = (int)index;
    return 0;
}
