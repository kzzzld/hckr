#include "core/config.h"
#include "core/color.h"
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
        .theme = "default",
        .foreground = -1,
        .background = -1,
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

static int load_file(struct app_config *config, const char *path, int theme_file,
                     unsigned *overrides, char *error, size_t error_size)
{
    FILE *file = fopen(path, "r");
    if (file == NULL && errno == ENOENT && !theme_file) {
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
        if (!theme_file && section == HACKERNEWS && strcmp(key, "base_url") == 0) {
            if (set_value(config->hn_base_url, sizeof(config->hn_base_url), value) != 0) {
                problem = "base_url must contain 1–2047 bytes";
            }
        } else if (!theme_file && section == UI && strcmp(key, "initial_tab") == 0) {
            if (!valid_tab(value)) {
                problem = "initial_tab must be news, new, past, comments, ask, show, or jobs";
            } else {
                set_value(config->initial_tab, sizeof(config->initial_tab), value);
            }
        } else if (!theme_file && section == UI && strcmp(key, "theme") == 0) {
            int valid = value[0] != '\0';
            for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
                if (!isalnum(*p) && *p != '-' && *p != '_') {
                    valid = 0;
                }
            }
            if (!valid || set_value(config->theme, sizeof(config->theme), value) != 0) {
                problem = "theme must be a name containing letters, numbers, hyphens, or underscores";
            }
        } else if (section == UI && (strcmp(key, "foreground") == 0 || strcmp(key, "background") == 0)) {
            int *color = strcmp(key, "foreground") == 0 ? &config->foreground : &config->background;
            if (color_parse(value, color) != 0) {
                problem = "color must be default, black, red, green, yellow, blue, magenta, cyan, white, 0–255, or #RRGGBB";
            } else {
                *overrides |= strcmp(key, "foreground") == 0 ? 1U : 2U;
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
    unsigned overrides = 0;
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
        int result = load_file(&loaded, path, 0, &overrides, error, error_size);
        free(path);
        if (result != 0) {
            return -1;
        }
    }
    if (strcmp(loaded.theme, "default") != 0) {
        const char *directory = getenv("HCKR_THEME_DIR");
        if (directory == NULL || directory[0] == '\0') {
            directory = HCKR_THEME_DIR;
        }
        size_t size = strlen(directory) + strlen(loaded.theme) + sizeof("/.ini");
        char *path = malloc(size);
        if (path == NULL) {
            snprintf(error, error_size, "could not allocate theme path");
            return -1;
        }
        snprintf(path, size, "%s/%s.ini", directory, loaded.theme);
        struct app_config themed = config_default();
        unsigned theme_colors = 0;
        int result = load_file(&themed, path, 1, &theme_colors, error, error_size);
        if (result == 0 && theme_colors != 3) {
            snprintf(error, error_size, "%s: theme requires foreground and background under [ui]", path);
            result = -1;
        }
        free(path);
        if (result != 0) {
            return -1;
        }
        if (!(overrides & 1)) {
            loaded.foreground = themed.foreground;
        }
        if (!(overrides & 2)) {
            loaded.background = themed.background;
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
