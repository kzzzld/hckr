#include "core/html.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int collect_text(xmlNode *node, xmlBufferPtr buffer, int paragraphs)
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
            const xmlChar *separator = BAD_CAST (paragraphs
                ? (xmlStrEqual(node->name, BAD_CAST "br") ? "\n" : "\n\n") : " ");
            if ((block && xmlBufferCat(buffer, separator) != 0) ||
                collect_text(node->children, buffer, paragraphs) != 0 ||
                (block && !xmlStrEqual(node->name, BAD_CAST "br") &&
                 xmlBufferCat(buffer, separator) != 0)) {
                return -1;
            }
        }
    }
    return 0;
}

static char *extract_text(xmlNode *node, int paragraphs)
{
    xmlBufferPtr buffer = xmlBufferCreate();
    if (buffer == NULL) {
        return NULL;
    }
    if (collect_text(node->children, buffer, paragraphs) != 0) {
        xmlBufferFree(buffer);
        return NULL;
    }
    const xmlChar *content = xmlBufferContent(buffer);
    char *text = malloc(strlen((const char *)content) + 1);
    if (text != NULL) {
        size_t length = 0;
        int space = 0;
        int breaks = 0;
        for (const unsigned char *p = content; *p != '\0'; ++p) {
            if (paragraphs && *p == '\n') {
                if (breaks < 2) ++breaks;
                space = 0;
            } else if (isspace(*p) || *p < 32 || *p == 127) {
                space = length > 0 && breaks == 0;
            } else {
                if (breaks > 0 && length > 0) {
                    while (breaks-- > 0) text[length++] = '\n';
                }
                breaks = 0;
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

char *html_text(xmlNode *node)
{
    return extract_text(node, 0);
}

char *html_post_text(xmlNode *node)
{
    return extract_text(node, 1);
}

char *html_item_path(xmlNode *node)
{
    while (node != NULL && !xmlStrEqual(node->name, BAD_CAST "tr")) {
        node = node->parent;
    }
    xmlChar *id = node != NULL ? xmlGetProp(node, BAD_CAST "id") : NULL;
    if (id == NULL || id[0] == '\0') {
        xmlFree(id);
        return NULL;
    }
    for (const xmlChar *p = id; *p; ++p) {
        if (!isdigit(*p)) {
            xmlFree(id);
            return NULL;
        }
    }
    size_t size = strlen((const char *)id) + sizeof("/item?id=");
    char *path = malloc(size);
    if (path != NULL) {
        snprintf(path, size, "/item?id=%s", (const char *)id);
    }
    xmlFree(id);
    return path;
}
