#include "core/config.h"

#include <stdio.h>

int main(void)
{
    struct app_config config;
    char error[4096];
    if (config_load(&config, error, sizeof(error)) != 0) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    printf("%d %d\n", config.foreground, config.background);
    return 0;
}
