#include "core/sections.h"

const struct hn_section *hn_sections(size_t *count)
{
    static const struct hn_section sections[] = {
        { "news", "/news", 0 },
        { "new", "/newest", 0 },
        { "past", "/front", 0 },
        { "comments", "/newcomments", 1 },
        { "ask", "/ask", 0 },
        { "show", "/show", 0 },
        { "jobs", "/jobs", 0 },
    };

    *count = sizeof(sections) / sizeof(sections[0]);
    return sections;
}
