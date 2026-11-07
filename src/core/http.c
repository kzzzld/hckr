#include "core/http.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BODY (2 * 1024 * 1024)

struct http_request {
    CURL *easy;
    CURLM *multi;
    char *body;
    size_t length;
    int added;
};

int http_init(void)
{
    return curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK ? 0 : -1;
}

void http_cleanup(void)
{
    curl_global_cleanup();
}

static size_t receive_body(char *data, size_t size, size_t count, void *context)
{
    struct http_request *request = context;
    if (size != 0 && count > MAX_BODY / size) {
        return 0;
    }
    size_t bytes = size * count;
    if (bytes > MAX_BODY - request->length) {
        return 0;
    }
    char *body = realloc(request->body, request->length + bytes + 1);
    if (body == NULL) {
        return 0;
    }
    request->body = body;
    memcpy(body + request->length, data, bytes);
    request->length += bytes;
    body[request->length] = '\0';
    return bytes;
}

void http_free(struct http_request *request)
{
    if (request == NULL) {
        return;
    }
    if (request->added) {
        curl_multi_remove_handle(request->multi, request->easy);
    }
    if (request->easy != NULL) {
        curl_easy_cleanup(request->easy);
    }
    if (request->multi != NULL) {
        curl_multi_cleanup(request->multi);
    }
    free(request->body);
    free(request);
}

struct http_request *http_start(const char *base_url, const char *path,
                                char *error, size_t error_size)
{
    size_t base_length = strlen(base_url);
    while (base_length > 0 && base_url[base_length - 1] == '/') {
        --base_length;
    }
    size_t size = base_length + strlen(path) + 1;
    char *url = malloc(size);
    struct http_request *request = calloc(1, sizeof(*request));
    if (url == NULL || request == NULL) {
        free(url);
        free(request);
        snprintf(error, error_size, "could not allocate request");
        return NULL;
    }
    snprintf(url, size, "%.*s%s", (int)base_length, base_url, path);
    request->easy = curl_easy_init();
    request->multi = curl_multi_init();
    CURLcode code = CURLE_FAILED_INIT;
    if (request->easy != NULL && request->multi != NULL) {
        code = CURLE_OK;
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_URL, url);
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_WRITEFUNCTION, receive_body);
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_WRITEDATA, request);
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_USERAGENT, "hckr/0.1");
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_FOLLOWLOCATION, 1L);
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_MAXREDIRS, 5L);
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_CONNECTTIMEOUT, 10L);
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_TIMEOUT, 20L);
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_NOSIGNAL, 1L);
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_FAILONERROR, 1L);
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_ACCEPT_ENCODING, "");
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_PROTOCOLS_STR, "http,https");
        }
        if (code == CURLE_OK) {
            code = curl_easy_setopt(request->easy, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
        }
    }
    free(url);
    if (code != CURLE_OK) {
        snprintf(error, error_size, "%s", curl_easy_strerror(code));
        http_free(request);
        return NULL;
    }
    CURLMcode multi_code = curl_multi_add_handle(request->multi, request->easy);
    if (multi_code != CURLM_OK) {
        snprintf(error, error_size, "%s", curl_multi_strerror(multi_code));
        http_free(request);
        return NULL;
    }
    request->added = 1;
    return request;
}

int http_poll(struct http_request *request, char *error, size_t error_size)
{
    int running;
    CURLMcode code = curl_multi_perform(request->multi, &running);
    if (code != CURLM_OK) {
        snprintf(error, error_size, "%s", curl_multi_strerror(code));
        return -1;
    }
    int remaining;
    CURLMsg *message;
    while ((message = curl_multi_info_read(request->multi, &remaining)) != NULL) {
        if (message->msg == CURLMSG_DONE) {
            if (message->data.result != CURLE_OK) {
                snprintf(error, error_size, "%s", curl_easy_strerror(message->data.result));
                return -1;
            }
            return 1;
        }
    }
    return 0;
}

const char *http_body(const struct http_request *request, size_t *length)
{
    *length = request->length;
    return request->body != NULL ? request->body : "";
}
