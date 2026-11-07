#include "platform/browser.h"

#include <stdio.h>
#include <time.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    struct browser browser = {0};
    char error[256] = "";
    if (browser_open(&browser, argv[1], error, sizeof(error)) != 0) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    for (int i = 0; i < 500; ++i) {
        int status = browser_poll(&browser, error, sizeof(error));
        if (status != 0) {
            if (status < 0) {
                fprintf(stderr, "%s\n", error);
            }
            return status < 0 ? 1 : 0;
        }
        struct timespec pause = { .tv_nsec = 10000000 };
        nanosleep(&pause, NULL);
    }
    return 2;
}
