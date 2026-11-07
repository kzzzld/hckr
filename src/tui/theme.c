#include "tui/theme.h"

#include <curses.h>
#include <stdio.h>

int theme_apply(const struct app_config *config, char *error, size_t error_size)
{
    if (config->foreground == -1 && config->background == -1) {
        return 0;
    }
    if (!has_colors() || start_color() == ERR) {
        snprintf(error, error_size, "this terminal does not support configured colors");
        return -1;
    }
    if (config->foreground >= COLORS || config->background >= COLORS) {
        snprintf(error, error_size, "this terminal supports color indexes 0 through %d", COLORS - 1);
        return -1;
    }
    if ((config->foreground == -1 || config->background == -1) && use_default_colors() == ERR) {
        snprintf(error, error_size, "this terminal cannot mix custom and default colors");
        return -1;
    }
    if (init_pair(1, config->foreground, config->background) == ERR) {
        snprintf(error, error_size, "could not initialize configured colors");
        return -1;
    }
    bkgd(COLOR_PAIR(1));
    attrset(COLOR_PAIR(1));
    return 0;
}
