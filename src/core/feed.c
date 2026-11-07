#include "core/feed.h"
#include "core/html.h"

#include <ctype.h>
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
