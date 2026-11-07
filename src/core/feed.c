#include "core/feed.h"
#include "core/html.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void feed_free(struct feed *feed)
{
    for (size_t i = 0; i < feed->count; ++i) {
        free(feed->items[i].title);
        free(feed->items[i].url);
        free(feed->items[i].discussion_path);
    }
    free(feed->items);
    *feed = (struct feed){0};
}

static char *item_url(xmlNode *node, int comments, const char *base_url)
{
    xmlChar *href = NULL;
    if (comments) {
        xmlNode *row = node;
        while (row != NULL && !xmlStrEqual(row->name, BAD_CAST "tr")) {
            row = row->parent;
        }
        xmlChar *id = row != NULL ? xmlGetProp(row, BAD_CAST "id") : NULL;
        if (id != NULL) {
            size_t size = strlen((const char *)id) + sizeof("item?id=");
            href = xmlMalloc(size);
            if (href != NULL) {
                snprintf((char *)href, size, "item?id=%s", (const char *)id);
            }
            xmlFree(id);
        }
    } else {
        href = xmlGetProp(node, BAD_CAST "href");
    }
    if (href == NULL) {
        return NULL;
    }
    size_t size = strlen(base_url) + 2;
    char *base = malloc(size);
    if (base == NULL) {
        xmlFree(href);
        return NULL;
    }
    snprintf(base, size, "%s%s", base_url,
             base_url[0] != '\0' && base_url[strlen(base_url) - 1] == '/' ? "" : "/");
    xmlChar *resolved = xmlBuildURI(href, BAD_CAST base);
    char *url = resolved != NULL ? strdup((const char *)resolved) : NULL;
    xmlFree(resolved);
    xmlFree(href);
    free(base);
    return url;
}

static long metric_number(const char *text)
{
    if (!isdigit((unsigned char)*text)) {
        return -1;
    }
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    return errno == 0 && end != text ? value : -1;
}

static void read_metrics(xmlNode *title, xmlXPathContextPtr context, struct feed_item *item)
{
    item->points = -1;
    item->comments = -1;
    xmlNode *row = title;
    while (row != NULL && !xmlStrEqual(row->name, BAD_CAST "tr")) {
        row = row->parent;
    }
    if (row == NULL) {
        return;
    }
    row = row->next;
    while (row != NULL && row->type != XML_ELEMENT_NODE) {
        row = row->next;
    }
    if (row == NULL || !xmlStrEqual(row->name, BAD_CAST "tr")) {
        return;
    }
    context->node = row;
    xmlXPathObjectPtr nodes = xmlXPathEvalExpression(BAD_CAST
        ".//span[contains(concat(' ',normalize-space(@class),' '),' score ')] | .//a", context);
    if (nodes != NULL && nodes->nodesetval != NULL) {
        for (int i = 0; i < nodes->nodesetval->nodeNr; ++i) {
            xmlNode *node = nodes->nodesetval->nodeTab[i];
            char *text = html_text(node);
            if (text == NULL) {
                continue;
            }
            if (xmlStrEqual(node->name, BAD_CAST "span")) {
                item->points = metric_number(text);
            } else if (item->discussion_path != NULL) {
                xmlChar *href = xmlGetProp(node, BAD_CAST "href");
                const char *path = item->discussion_path + 1;
                if (href != NULL && strcmp((const char *)href, path) == 0) {
                    if (strcmp(text, "discuss") == 0) {
                        item->comments = 0;
                    } else if (strstr(text, "comment") != NULL) {
                        item->comments = metric_number(text);
                    }
                }
                xmlFree(href);
            }
            free(text);
        }
    }
    xmlXPathFreeObject(nodes);
}

int feed_parse(const char *html, size_t length, int comments, const char *base_url,
               struct feed *feed,
               char *error, size_t error_size)
{
    if (length == 0 || length > INT_MAX) {
        snprintf(error, error_size, "empty or oversized response");
        return -1;
    }
    htmlDocPtr document = htmlReadMemory(html, (int)length, NULL, "UTF-8",
        HTML_PARSE_RECOVER | HTML_PARSE_NONET | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    xmlXPathContextPtr context = document != NULL ? xmlXPathNewContext(document) : NULL;
    const char *expression = comments
        ? "//*[contains(concat(' ',normalize-space(@class),' '),' commtext ')]"
        : "//*[contains(concat(' ',normalize-space(@class),' '),' titleline ')]/a[1]";
    xmlXPathObjectPtr nodes = context != NULL
        ? xmlXPathEvalExpression((const xmlChar *)expression, context) : NULL;
    int result = -1;
    snprintf(error, error_size, "could not parse Hacker News page");
    if (nodes != NULL && nodes->nodesetval != NULL && nodes->nodesetval->nodeNr > 0) {
        size_t count = (size_t)nodes->nodesetval->nodeNr;
        feed->items = calloc(count, sizeof(*feed->items));
        if (feed->items != NULL) {
            result = 0;
            for (size_t i = 0; i < count; ++i) {
                char *text = html_text(nodes->nodesetval->nodeTab[i]);
                if (text == NULL) {
                    result = -1;
                    snprintf(error, error_size, "could not allocate feed items");
                    break;
                }
                feed->items[feed->count].title = text;
                feed->items[feed->count].url = item_url(nodes->nodesetval->nodeTab[i],
                                                       comments, base_url);
                feed->items[feed->count].discussion_path =
                    html_item_path(nodes->nodesetval->nodeTab[i]);
                struct feed_item *item = &feed->items[feed->count];
                item->points = item->comments = -1;
                if (!comments) {
                    read_metrics(nodes->nodesetval->nodeTab[i], context, item);
                }
                ++feed->count;
            }
        }
    } else if (nodes != NULL) {
        snprintf(error, error_size, "no items found; page format may have changed");
    }
    xmlXPathFreeObject(nodes);
    xmlXPathFreeContext(context);
    xmlFreeDoc(document);
    if (result != 0) {
        feed_free(feed);
    } else {
        error[0] = '\0';
    }
    return result;
}
