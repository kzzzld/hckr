#include "tui/mouse.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    struct mouse_event mouse;
    assert(mouse_parse("0;1;1M", &mouse) == 0);
    assert(mouse.button == MOUSE_LEFT && !mouse.released && mouse.x == 0 && mouse.y == 0);
    assert(mouse_parse("0;8;4m", &mouse) == 0);
    assert(mouse.button == MOUSE_LEFT && mouse.released && mouse.x == 7 && mouse.y == 3);
    assert(mouse_parse("4;8;4m", &mouse) == 0);
    assert(mouse.button == MOUSE_LEFT && mouse.shift);
    assert(mouse_parse("64;8;4M", &mouse) == 0 && mouse.button == MOUSE_UP);
    assert(mouse_parse("65;8;4M", &mouse) == 0 && mouse.button == MOUSE_DOWN);
    assert(mouse_parse("2;8;4m", &mouse) == 0 && mouse.button == MOUSE_RIGHT);
    assert(mouse_parse("32;8;4M", &mouse) == 0 && mouse.button == MOUSE_NONE && mouse.motion);
    assert(mouse_parse("35;8;4M", &mouse) == 0 && mouse.motion && !mouse.released);
    assert(mouse_over(&mouse, 0, 3, 8));
    assert(mouse_over(&mouse, 7, 3, 1));
    assert(!mouse_over(&mouse, 0, 2, 8));
    assert(!mouse_over(&mouse, 8, 3, 2));
    assert(!mouse_over(&mouse, 0, 3, 7));
    assert(!mouse_over(&mouse, 0, 3, 0));
    assert(mouse_parse("0;8;4m", &mouse) == 0 && !mouse.motion);
    const char *invalid[] = {
        "", "0;0;1M", "0;1;0M", "0;-1;1M", "0;1;1X", "0;1;1Mextra",
        "128;1;1M", "0;99999999999999999999999;1M", "0;1M", "0;;1M",
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        assert(mouse_parse(invalid[i], &mouse) == -1);
    }
    puts("Mouse buttons, wheel, modifiers, and invalid reports passed");
    return 0;
}
