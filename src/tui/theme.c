#include "tui/theme.h"
#include "core/color.h"

#include <curses.h>
#include <limits.h>
#include <stdio.h>

static short resolve_color(int color)
{
    if (color < APP_COLOR_RGB) {
        return (short)color;
    }
    int rgb[3] = { (color >> 16) & 255, (color >> 8) & 255, color & 255 };
    long best_distance = LONG_MAX;
    short best = 0;
    int count = COLORS < 256 ? COLORS : 256;
    for (int slot = 0; slot < count; ++slot) {
        int palette[3];
        if (slot >= 232) {
            palette[0] = palette[1] = palette[2] = 8 + (slot - 232) * 10;
        } else if (slot >= 16) {
            static const int levels[] = { 0, 95, 135, 175, 215, 255 };
            int cube = slot - 16;
            palette[0] = levels[cube / 36];
            palette[1] = levels[cube / 6 % 6];
            palette[2] = levels[cube % 6];
        } else {
            short r, g, b;
            if (color_content((short)slot, &r, &g, &b) != OK) {
                continue;
            }
            palette[0] = r * 255 / 1000;
            palette[1] = g * 255 / 1000;
            palette[2] = b * 255 / 1000;
        }
        long dr = rgb[0] - palette[0], dg = rgb[1] - palette[1], db = rgb[2] - palette[2];
        long distance = dr * dr + dg * dg + db * db;
        if (distance < best_distance) {
            best_distance = distance;
            best = (short)slot;
        }
    }
    return best;
}

int theme_apply(const struct app_config *config, char *error, size_t error_size)
{
    if (config->foreground == APP_COLOR_DEFAULT && config->background == APP_COLOR_DEFAULT) {
        return 0;
    }
    if (!has_colors() || start_color() == ERR) {
        snprintf(error, error_size, "this terminal does not support configured colors");
        return -1;
    }
    if ((config->foreground < APP_COLOR_RGB && config->foreground >= COLORS) ||
        (config->background < APP_COLOR_RGB && config->background >= COLORS)) {
        snprintf(error, error_size, "this terminal supports color indexes 0 through %d", COLORS - 1);
        return -1;
    }
    if ((config->foreground == APP_COLOR_DEFAULT || config->background == APP_COLOR_DEFAULT) &&
        use_default_colors() == ERR) {
        snprintf(error, error_size, "this terminal cannot mix custom and default colors");
        return -1;
    }
    short foreground = resolve_color(config->foreground);
    short background = resolve_color(config->background);
    if (init_pair(1, foreground, background) == ERR) {
        snprintf(error, error_size, "could not initialize configured colors");
        return -1;
    }
    bkgd(COLOR_PAIR(1));
    attrset(COLOR_PAIR(1));
    return 0;
}
