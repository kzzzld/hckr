#include "tui/theme.h"
#include "core/color.h"

#include <assert.h>
#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    FILE *source = tmpfile(), *sink = tmpfile(), *protocol = tmpfile();
    assert(source != NULL && sink != NULL && protocol != NULL);
    int saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0 && dup2(fileno(protocol), STDOUT_FILENO) >= 0);
    SCREEN *screen = newterm("xterm-256color", sink, source);
    assert(screen != NULL);
    struct app_config config = { .foreground = APP_COLOR_RGB | 0xD8DEE9,
                                 .background = APP_COLOR_RGB | 0x2E3440 };
    struct theme_state theme = {0};
    char error[256];
    assert(theme_apply(&config, &theme, error, sizeof(error)) == 0);
    assert(theme.count == 2);
    short foreground, background, r, g, b;
    assert(pair_content(1, &foreground, &background) == OK);
    assert(foreground != background);
    assert(color_content(foreground, &r, &g, &b) == OK);
    assert(r == 848 && g == 871 && b == 914);
    assert(color_content(background, &r, &g, &b) == OK);
    assert(r == 181 && g == 204 && b == 251);
    theme_restore(&theme);
    fflush(protocol);
    rewind(protocol);
    char restored[128] = "";
    assert(fread(restored, 1, sizeof(restored) - 1, protocol) > 0);
    assert(strstr(restored, "\033]104;") != NULL);

    theme = (struct theme_state){0};
    config.foreground = 255;
    assert(theme_apply(&config, &theme, error, sizeof(error)) == 0);
    assert(theme.count == 1 && theme.slots[0] != 255);
    assert(pair_content(1, &foreground, &background) == OK && foreground == 255);
    theme_restore(&theme);
    endwin();
    delscreen(screen);

    screen = newterm("xterm", sink, source);
    assert(screen != NULL);
    theme = (struct theme_state){0};
    config.foreground = APP_COLOR_RGB | 0xD8DEE9;
    assert(theme_apply(&config, &theme, error, sizeof(error)) == 0);
    assert(theme.count == 0);
    assert(pair_content(1, &foreground, &background) == OK);
    assert(foreground >= 0 && foreground < COLORS && background >= 0 && background < COLORS);
    endwin();
    delscreen(screen);
    fflush(stdout);
    assert(dup2(saved_stdout, STDOUT_FILENO) >= 0);
    close(saved_stdout);
    fclose(source);
    fclose(sink);
    fclose(protocol);
    puts("Nord RGB colors, palette restoration, overrides, and fixed-palette fallback passed");
    return 0;
}
