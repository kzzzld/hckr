#ifndef HCKR_TUI_H
#define HCKR_TUI_H

#include "core/sections.h"
#include "core/config.h"

int tui_run(const struct hn_section *sections, size_t count, size_t selected,
            const struct app_config *config);

#endif
