#include "core/discussion.h"
#include "core/html.h"

#include <limits.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static xmlXPathObjectPtr find(xmlXPathContextPtr context, const char *expression)
{
    return xmlXPathEvalExpression(BAD_CAST expression, context);
}

static char *text_at(xmlXPathContextPtr context, const char *expression,
                     const char *fallback)
{
    xmlXPathObjectPtr nodes = find(context, expression);
    char *text = NULL;
    if (nodes != NULL && nodes->nodesetval != NULL && nodes->nodesetval->nodeNr > 0) {
        text = html_text(nodes->nodesetval->nodeTab[0]);
    } else if (nodes != NULL) {
        text = strdup(fallback);
    }
    xmlXPathFreeObject(nodes);
    return text;
}

void discussion_free(struct discussion *discussion)
{
    free(discussion->title);
    free(discussion->points);
    free(discussion->text);
    for (size_t i = 0; i < discussion->count; ++i) {
        free(discussion->comments[i].author);
        free(discussion->comments[i].text);
        free(discussion->comments[i].path);
    }
    free(discussion->comments);
    *discussion = (struct discussion){0};
}

int discussion_parse(const char *html, size_t length, struct discussion *discussion,
                     char *error, size_t error_size)
{
    if (length == 0 || length > INT_MAX) {
        snprintf(error, error_size, "empty or oversized discussion");
        return -1;
    }
    htmlDocPtr doc = htmlReadMemory(html, (int)length, NULL, "UTF-8",
        HTML_PARSE_RECOVER | HTML_PARSE_NONET | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    xmlXPathContextPtr context = doc != NULL ? xmlXPathNewContext(doc) : NULL;
    xmlXPathObjectPtr rows = NULL;
    int result = -1;
    snprintf(error, error_size, "could not parse discussion");
    if (context == NULL) {
        goto cleanup;
    }
    rows = find(context, "//tr[contains(concat(' ',normalize-space(@class),' '),' athing ')]");
    if (rows == NULL || rows->nodesetval == NULL || rows->nodesetval->nodeNr == 0) {
        snprintf(error, error_size, "discussion not found or page format changed");
        goto cleanup;
    }
    xmlXPathFreeObject(rows);
    rows = NULL;
    discussion->title = text_at(context,
        "//*[contains(concat(' ',normalize-space(@class),' '),' titleline ')]/a[1]", "Discussion");
    discussion->points = text_at(context,
        "//span[contains(concat(' ',normalize-space(@class),' '),' score ')]", "Points unavailable");
    discussion->text = text_at(context,
        "//*[contains(concat(' ',normalize-space(@class),' '),' toptext ')]", "");
    if (discussion->title == NULL || discussion->points == NULL || discussion->text == NULL) {
        goto cleanup;
    }
    rows = find(context, "//tr[contains(concat(' ',normalize-space(@class),' '),' comtr ')]");
    if (rows == NULL || rows->nodesetval == NULL) {
        goto cleanup;
    }
    size_t count = (size_t)rows->nodesetval->nodeNr;
    if (count > 0) {
        discussion->comments = calloc(count, sizeof(*discussion->comments));
        if (discussion->comments == NULL) {
            goto cleanup;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        context->node = rows->nodesetval->nodeTab[i];
        struct comment *comment = &discussion->comments[discussion->count++];
        comment->path = html_item_path(context->node);
        comment->author = text_at(context, ".//a[starts-with(@href,'user?id=') or contains(concat(' ',normalize-space(@class),' '),' hnuser ')]", "[deleted]");
        comment->text = text_at(context,
            ".//*[contains(concat(' ',normalize-space(@class),' '),' commtext ')]", "[deleted]");
        xmlXPathObjectPtr indent = find(context, ".//td[@class='ind']/@indent | .//td[@class='ind']/img/@width");
        if (indent != NULL && indent->nodesetval != NULL && indent->nodesetval->nodeNr > 0) {
            xmlNode *attribute = indent->nodesetval->nodeTab[0];
            xmlChar *value = xmlNodeGetContent(attribute);
            if (value != NULL) {
                unsigned long depth = strtoul((const char *)value, NULL, 10);
                if (xmlStrEqual(attribute->name, BAD_CAST "width")) {
                    depth /= 40;
                }
                comment->depth = depth > 20 ? 20 : (unsigned)depth;
            }
            xmlFree(value);
        }
        xmlXPathFreeObject(indent);
        if (comment->author == NULL || comment->text == NULL) {
            goto cleanup;
        }
    }
    context->node = xmlDocGetRootElement(doc);
    xmlXPathObjectPtr more = find(context, "//a[@class='morelink']");
    discussion->more = more != NULL && more->nodesetval != NULL && more->nodesetval->nodeNr > 0;
    xmlXPathFreeObject(more);
    error[0] = '\0';
    result = 0;
cleanup:
    xmlXPathFreeObject(rows);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    if (result != 0) {
        discussion_free(discussion);
    }
    return result;
}
