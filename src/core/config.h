#ifndef HCKR_CONFIG_H
#define HCKR_CONFIG_H

#include <stddef.h>

struct app_config {
    char hn_base_url[2048];
    char initial_tab[32];
    char theme[64];
    int foreground;
    int background;
};

struct app_config config_default(void);
int config_load(struct app_config *config, char *error, size_t error_size);

#endif
