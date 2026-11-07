#include "core/config.h"
#include "core/http.h"
#include "tui/tui.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct app_config config;
    char error[4096];
    size_t section_count;
    const struct hn_section *sections = hn_sections(&section_count);

    setlocale(LC_ALL, "");

    if (config_load(&config, error, sizeof(error)) != 0) {
        fprintf(stderr, "hckr: %s\n", error);
        return 1;
    }

    size_t selected = 0;
    for (size_t i = 0; i < section_count; ++i) {
        if (strcmp(config.initial_tab, sections[i].label) == 0) {
            selected = i;
            break;
        }
    }
    if (http_init() != 0) {
        fputs("hckr: could not initialize networking\n", stderr);
        return 1;
    }
    int result = tui_run(sections, section_count, selected, &config);
    http_cleanup();
    return result;
}
