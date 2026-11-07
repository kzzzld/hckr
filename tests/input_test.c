#include "tui/input.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif

struct terminal {
    int master;
    atomic_int stop;
};

static void *drain_output(void *context)
{
    struct terminal *terminal = context;
    char bytes[4096];
    while (!atomic_load(&terminal->stop)) {
        read(terminal->master, bytes, sizeof(bytes));
        struct timespec pause = { .tv_nsec = 1000000 };
        nanosleep(&pause, NULL);
    }
    return NULL;
}

int main(void)
{
    int saved_stdout = dup(STDOUT_FILENO);
    FILE *protocol = tmpfile();
    assert(saved_stdout >= 0 && protocol != NULL);
    assert(dup2(fileno(protocol), STDOUT_FILENO) >= 0);
    const struct {
        const char *bytes;
        int expected;
    } cases[] = {
        { "\033[A", KEY_UP }, { "\033[B", KEY_DOWN },
        { "\033[C", KEY_RIGHT }, { "\033[D", KEY_LEFT },
        { "\033OA", KEY_UP }, { "\033OB", KEY_DOWN },
        { "\033OC", KEY_RIGHT }, { "\033OD", KEY_LEFT },
        { "\033[1;1A", KEY_UP }, { "\033[1;2B", KEY_DOWN },
        { "\033[1;5C", KEY_RIGHT }, { "\033[1;8D", KEY_LEFT },
        { "\033[13;2u", KEY_DISCUSSION },
        { "\033[27;2;13~", KEY_DISCUSSION },
        { "\033[27u", KEY_EXIT }, { "\033[99;5u", KEY_CANCEL },
        { "\033[999~", ERR }, { "\033q", ERR },
        { "\033", 27 }, { "q", 'q' },
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        int master, slave;
        assert(openpty(&master, &slave, NULL, NULL, NULL) == 0);
        assert(fcntl(master, F_SETFL, O_NONBLOCK) == 0);
        struct terminal terminal = { .master = master };
        pthread_t thread;
        assert(pthread_create(&thread, NULL, drain_output, &terminal) == 0);
        FILE *source = fdopen(slave, "r");
        FILE *sink = fdopen(dup(slave), "w");
        assert(source != NULL && sink != NULL);
        SCREEN *screen = newterm("xterm-256color", sink, source);
        assert(screen != NULL);
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        timeout(100);
        input_start();
        assert(write(master, cases[i].bytes, strlen(cases[i].bytes)) == (ssize_t)strlen(cases[i].bytes));
        int key = input_read();
        if (key != cases[i].expected) {
            fprintf(stderr, "case %zu: got %d, expected %d\n", i, key, cases[i].expected);
            return 1;
        }
        input_stop();
        endwin();
        delscreen(screen);
        fclose(source);

        fclose(sink);
        atomic_store(&terminal.stop, 1);
        pthread_join(thread, NULL);
        close(master);
    }
    fflush(stdout);
    assert(dup2(saved_stdout, STDOUT_FILENO) >= 0);
    close(saved_stdout);
    fclose(protocol);
    puts("Arrow keys, Shift-Enter, unknown sequences, and Escape passed");
    return 0;
}
