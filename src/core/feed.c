#include "core/feed.h"

#include <ctype.h>
#include <limits.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void feed_free(struct feed *feed)
{
    for (size_t i = 0; i < feed->count; ++i) {
        free(feed->items[i]);
    }
    free(feed->items);
    *feed = (struct feed){0};
}

static int collect_text(xmlNode *node, xmlBufferPtr buffer)
{
    for (; node != NULL; node = node->next) {
        if (node->type == XML_TEXT_NODE) {
            if (xmlBufferCat(buffer, node->content) != 0) {
                return -1;
            }
        } else if (node->type == XML_ELEMENT_NODE) {
            int block = xmlStrEqual(node->name, BAD_CAST "p") ||
                        xmlStrEqual(node->name, BAD_CAST "div") ||
                        xmlStrEqual(node->name, BAD_CAST "br") ||
                        xmlStrEqual(node->name, BAD_CAST "pre");
            if ((block && xmlBufferCat(buffer, BAD_CAST " ") != 0) ||
                collect_text(node->children, buffer) != 0 ||
                (block && xmlBufferCat(buffer, BAD_CAST " ") != 0)) {
                return -1;
            }
        }
    }
    return 0;
}

static char *plain_text(xmlNode *node)
{
    xmlBufferPtr buffer = xmlBufferCreate();
    if (buffer == NULL) {
        return NULL;
    }
    if (collect_text(node->children, buffer) != 0) {
        xmlBufferFree(buffer);
        return NULL;
    }
    const xmlChar *content = xmlBufferContent(buffer);
    char *text = malloc(strlen((const char *)content) + 1);
    if (text != NULL) {
        size_t length = 0;
        int space = 0;
        for (const unsigned char *p = content; *p != '\0'; ++p) {
            if (isspace(*p) || *p < 32 || *p == 127) {
                space = length > 0;
            } else {
                if (space) {
                    text[length++] = ' ';
                    space = 0;
                }
                text[length++] = (char)*p;
            }
        }
        text[length] = '\0';
    }
    xmlBufferFree(buffer);
    return text;
}

int feed_parse(const char *html, size_t length, int comments, struct feed *feed,
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
                char *text = plain_text(nodes->nodesetval->nodeTab[i]);
                if (text == NULL) {
                    result = -1;
                    snprintf(error, error_size, "could not allocate feed items");
                    break;
                }
                feed->items[feed->count++] = text;
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
