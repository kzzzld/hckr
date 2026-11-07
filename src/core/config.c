#include "core/config.h"
#include "core/sections.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct app_config config_default(void)
{
    return (struct app_config){
        .hn_base_url = "https://news.ycombinator.com",
        .initial_tab = "new",
    };
}

static char *trim(char *text)
{
    while (isspace((unsigned char)*text)) {
        ++text;
    }
    size_t length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[--length] = '\0';
    }
    return text;
}

static int set_value(char *destination, size_t capacity, const char *value)
{
    size_t length = strlen(value);
    if (length == 0 || length >= capacity) {
        return -1;
    }
    memcpy(destination, value, length + 1);
    return 0;
}

static int valid_tab(const char *label)
{
    size_t count;
    const struct hn_section *sections = hn_sections(&count);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(label, sections[i].label) == 0) {
            return 1;
        }
    }
    return 0;
}

static int create_file(const char *path)
{
    char *directory = strdup(path);
    if (directory == NULL) {
        return -1;
    }
    for (char *p = directory + 1; *p != '\0'; ++p) {
        if (*p != '/') {
            continue;
        }
        *p = '\0';
        if (mkdir(directory, 0700) != 0 && errno != EEXIST) {
            int saved_errno = errno;
            free(directory);
            errno = saved_errno;
            return -1;
        }
        *p = '/';
    }
    free(directory);

    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        return errno == EEXIST ? 0 : -1;
    }
    FILE *file = fdopen(fd, "w");
    if (file == NULL) {
        int saved_errno = errno;
        close(fd);
        unlink(path);
        errno = saved_errno;
        return -1;
    }
    struct app_config defaults = config_default();
    int result = fprintf(file, "[hackernews]\nbase_url = %s\n\n[ui]\ninitial_tab = %s\n",
                         defaults.hn_base_url, defaults.initial_tab);
    int saved_errno = result < 0 ? errno : 0;
    if (fclose(file) != 0 && saved_errno == 0) {
        saved_errno = errno;
    }
    if (saved_errno != 0) {
        unlink(path);
        errno = saved_errno;
        return -1;
    }
    return 0;
}

static int load_file(struct app_config *config, const char *path,
                     char *error, size_t error_size)
{
    FILE *file = fopen(path, "r");
    if (file == NULL && errno == ENOENT) {
        if (create_file(path) != 0) {
            snprintf(error, error_size, "%s: could not create configuration: %s",
                     path, strerror(errno));
            return -1;
        }
        file = fopen(path, "r");
    }
    if (file == NULL) {
        snprintf(error, error_size, "%s: %s", path, strerror(errno));
        return -1;
    }

    char *line = NULL;
    size_t capacity = 0;
    size_t number = 0;
    enum { NONE, HACKERNEWS, UI } section = NONE;
    const char *problem = NULL;
    ssize_t length;

    while ((length = getline(&line, &capacity, file)) >= 0) {
        ++number;
        if (memchr(line, '\0', (size_t)length) != NULL) {
            problem = "unexpected NUL byte";
            break;
        }
        char *text = trim(line);
        if (*text == '\0' || *text == '#' || *text == ';') {
            continue;
        }
        if (*text == '[') {
            if (strcmp(text, "[hackernews]") == 0) {
                section = HACKERNEWS;
            } else if (strcmp(text, "[ui]") == 0) {
                section = UI;
            } else {
                problem = "unknown or malformed section";
                break;
            }
            continue;
        }
        char *equals = strchr(text, '=');
        if (equals == NULL) {
            problem = "expected key = value";
            break;
        }
        *equals = '\0';
        char *key = trim(text);
        char *value = trim(equals + 1);
        if (section == HACKERNEWS && strcmp(key, "base_url") == 0) {
            if (set_value(config->hn_base_url, sizeof(config->hn_base_url), value) != 0) {
                problem = "base_url must contain 1–2047 bytes";
            }
        } else if (section == UI && strcmp(key, "initial_tab") == 0) {
            if (!valid_tab(value)) {
                problem = "initial_tab must be new, past, comments, ask, show, or jobs";
            } else {
                set_value(config->initial_tab, sizeof(config->initial_tab), value);
            }
        } else {
            problem = "unknown setting or setting outside its section";
        }
        if (problem != NULL) {
            break;
        }
    }
    if (problem == NULL && ferror(file)) {
        problem = "could not read configuration";
    }
    free(line);
    if (fclose(file) != 0 && problem == NULL) {
        problem = "could not close configuration";
    }
    if (problem != NULL) {
        snprintf(error, error_size, "%s:%zu: %s", path, number, problem);
        return -1;
    }
    return 0;
}

int config_load(struct app_config *config, char *error, size_t error_size)
{
    struct app_config loaded = config_default();
    const char *root = getenv("XDG_CONFIG_HOME");
    const char *suffix = "/hckr/config.ini";
    if (root == NULL || root[0] == '\0') {
        root = getenv("HOME");
        suffix = "/.config/hckr/config.ini";
    } else if (root[0] != '/') {
        snprintf(error, error_size, "XDG_CONFIG_HOME must be an absolute path");
        return -1;
    }
    if (root == NULL || root[0] == '\0') {
        snprintf(error, error_size, "HOME or XDG_CONFIG_HOME is required to locate configuration");
        return -1;
    }
    if (root != NULL && root[0] != '\0') {
        size_t size = strlen(root) + strlen(suffix) + 1;
        char *path = malloc(size);
        if (path == NULL) {
            snprintf(error, error_size, "could not allocate configuration path");
            return -1;
        }
        snprintf(path, size, "%s%s", root, suffix);
        int result = load_file(&loaded, path, error, error_size);
        free(path);
        if (result != 0) {
            return -1;
        }
    }
    const char *url = getenv("HCKR_HN_URL");
    if (url != NULL && set_value(loaded.hn_base_url, sizeof(loaded.hn_base_url), url) != 0) {
        snprintf(error, error_size, "HCKR_HN_URL must contain 1–2047 bytes");
        return -1;
    }
    *config = loaded;
    return 0;
}
