#include "core/feed.h"
#include "core/http.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    assert(argc == 3);
    assert(http_init() == 0);
    char error[256] = "";
    struct http_request *request = http_start(argv[1], argv[2], error, sizeof(error));
    assert(request != NULL);
    int status = 0;
    for (int i = 0; i < 1000 && status == 0; ++i) {
        status = http_poll(request, error, sizeof(error));
        struct timespec wait = { .tv_nsec = 10000000 };
        nanosleep(&wait, NULL);
    }
    if (status < 0) {
        fprintf(stderr, "%s\n", error);
        http_free(request);
        http_cleanup();
        return 2;
    }
    assert(status == 1);
    size_t length;
    const char *body = http_body(request, &length);
    struct feed feed = {0};
    int result = feed_parse(body, length, strcmp(argv[2], "/newcomments") == 0,
                            &feed, error, sizeof(error));
    if (result == 0) {
        assert(error[0] == '\0');
        assert(feed.count > 0);
        printf("%zu items\n%s\n", feed.count, feed.items[0]);
    } else {
        fprintf(stderr, "%s\n", error);
    }
    feed_free(&feed);
    http_free(request);
    http_cleanup();
    return result == 0 ? 0 : 3;
}
