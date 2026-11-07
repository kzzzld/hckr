#ifndef HCKR_THEME_H
#define HCKR_THEME_H

#include "core/config.h"

struct theme_state {
    short slots[2];
    short original[2][3];
    int count;
};

void theme_restore(const struct theme_state *state);
int theme_apply(const struct app_config *config, struct theme_state *state, char *error, size_t error_size);

#endif
