#include "core/discussion.h"
#include "core/feed.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char page[] =
    "<table><tr class='athing' id='100'><td><span class='titleline'>"
    "<a href='https://example.com/story'>A &amp; B</a></span></td></tr>"
    "<tr><td><span class='score'>42 points</span>"
    "<div class='toptext'>Story <i>body</i>.<p>Second paragraph.</p></div></td></tr></table>"
    "<table><tr class='athing comtr' id='101'><td class='ind' indent='0'>"
    "<img width='0'></td><td><a href='user?id=alice'>alice</a>"
    "<div class='commtext c00'>Hello &amp; hi.<p>Next paragraph.</p></div></td></tr>"
    "<tr class='athing comtr' id='102'><td class='ind'><img width='80'></td>"
    "<td><a class='hnuser'>bob</a><div class='commtext'>Reply</div></td></tr>"
    "<tr class='athing comtr' id='103'><td>[deleted]</td></tr></table>"
    "<a class='morelink' href='item?id=100&amp;p=2'>More</a>";

int main(void)
{
    char error[256];
    struct discussion discussion = {0};
    assert(discussion_parse(page, strlen(page), &discussion, error, sizeof(error)) == 0);
    assert(error[0] == '\0');
    assert(strcmp(discussion.title, "A & B") == 0);
    assert(strcmp(discussion.points, "42 points") == 0);
    assert(strcmp(discussion.text, "Story body.\n\nSecond paragraph.") == 0);
    assert(discussion.count == 3 && discussion.more);
    assert(strcmp(discussion.comments[0].author, "alice") == 0);
    assert(strcmp(discussion.comments[0].text, "Hello & hi. Next paragraph.") == 0);
    assert(discussion.comments[1].depth == 2);
    assert(strcmp(discussion.comments[0].path, "/item?id=101") == 0);
    assert(strcmp(discussion.comments[1].path, "/item?id=102") == 0);
    assert(strcmp(discussion.comments[2].text, "[deleted]") == 0);
    discussion_free(&discussion);

    const char *empty = "<table><tr class='athing' id='1'><td>Job</td></tr></table>";
    assert(discussion_parse(empty, strlen(empty), &discussion, error, sizeof(error)) == 0);
    assert(discussion.count == 0);
    assert(strcmp(discussion.points, "Points unavailable") == 0);
    discussion_free(&discussion);
    assert(discussion_parse("oops", 4, &discussion, error, sizeof(error)) == -1);
    assert(discussion.title == NULL && discussion.count == 0);
    assert(discussion_parse("", 0, &discussion, error, sizeof(error)) == -1);

    struct feed feed = {0};
    assert(feed_parse(page, strlen(page), 0, "https://example.com/custom", &feed,
                      error, sizeof(error)) == 0);
    assert(strcmp(feed.items[0].discussion_path, "/item?id=100") == 0);
    feed_free(&feed);
    assert(feed_parse(page, strlen(page), 1, "https://example.com/custom", &feed,
                      error, sizeof(error)) == 0);
    assert(strcmp(feed.items[0].discussion_path, "/item?id=101") == 0);
    feed_free(&feed);
    puts("Discussion parsing and feed links passed");
    return 0;
}
