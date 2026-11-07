#include "platform/browser.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

static int valid_url(const char *url)
{
    if (url == NULL || *url == '\0') {
        return 0;
    }
    for (const unsigned char *p = (const unsigned char *)url; *p; ++p) {
        if (*p <= 32 || *p == 127) {
            return 0;
        }
    }
    CURLU *parsed = curl_url();
    char *scheme = NULL;
    int valid = parsed != NULL && curl_url_set(parsed, CURLUPART_URL, url, 0) == CURLUE_OK &&
                curl_url_get(parsed, CURLUPART_SCHEME, &scheme, 0) == CURLUE_OK &&
                (strcmp(scheme, "http") == 0 || strcmp(scheme, "https") == 0);
    curl_free(scheme);
    curl_url_cleanup(parsed);
    return valid;
}

int browser_open(struct browser *browser, const char *url, char *error, size_t error_size)
{
    if (browser->child != 0) {
        snprintf(error, error_size, "browser is still opening");
        return -1;
    }
    if (!valid_url(url)) {
        snprintf(error, error_size, "this item has no valid HTTP or HTTPS link");
        return -1;
    }
#ifdef _WIN32
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1, NULL, 0);
    wchar_t *wide = length > 0 ? malloc((size_t)length * sizeof(*wide)) : NULL;
    if (wide == NULL) {
        snprintf(error, error_size, "could not convert browser URL");
        return -1;
    }
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1, wide, length);
    HRESULT com = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    INT_PTR result = (INT_PTR)ShellExecuteW(NULL, L"open", wide, NULL, NULL, SW_SHOWNORMAL);
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    free(wide);
    if (result <= 32) {
        snprintf(error, error_size, "could not open default browser (Windows error %lld)",
                 (long long)result);
        return -1;
    }
    return 0;
#else
#ifdef __APPLE__
    const char *command = "open";
#else
    const char *command = "xdg-open";
#endif
    char *argv[] = { (char *)command, (char *)url, NULL };
    posix_spawn_file_actions_t actions;
    int code = posix_spawn_file_actions_init(&actions);
    if (code != 0) {
        snprintf(error, error_size, "could not prepare browser: %s", strerror(code));
        return -1;
    }
    code = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    if (code == 0) {
        code = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    }
    if (code == 0) {
        code = posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    }
    pid_t child;
    if (code == 0) {
        code = posix_spawnp(&child, command, &actions, NULL, argv, environ);
    }
    posix_spawn_file_actions_destroy(&actions);
    if (code != 0) {
        snprintf(error, error_size, "could not run %s: %s", command, strerror(code));
        return -1;
    }
    browser->child = child;
    return 0;
#endif
}

int browser_poll(struct browser *browser, char *error, size_t error_size)
{
#ifndef _WIN32
    if (browser->child != 0) {
        int status;
        pid_t result = waitpid((pid_t)browser->child, &status, WNOHANG);
        if (result == 0 || (result < 0 && errno == EINTR)) {
            return 0;
        }
        browser->child = 0;
        if (result < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            snprintf(error, error_size, "default browser could not be opened");
            return -1;
        }
    }
#else
    (void)browser;
    (void)error;
    (void)error_size;
#endif
    return 1;
}
