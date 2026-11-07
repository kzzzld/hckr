#ifndef HCKR_HTML_H
#define HCKR_HTML_H

#include <libxml/tree.h>

char *html_text(xmlNode *node);
char *html_post_text(xmlNode *node);
char *html_item_path(xmlNode *node);

#endif
