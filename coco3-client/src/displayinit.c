#include "display.h"
#include "palette.h"
#include <coco.h>
#include <cmoc.h>

static unsigned char prompt_display_target(void)
{
    unsigned char key;

    printf("Display target?\n");
    printf("  R = RGB monitor\n");
    printf("  C = Composite monitor\n");

    for (;;) {
        key = waitkey(1);
        if (key == 'R' || key == 'r')
            return DISPLAY_RGB;
        if (key == 'C' || key == 'c')
            return DISPLAY_COMPOSITE;
    }
}

unsigned char display_target_init(void)
{
    unsigned char t = display_target_saved();

    if (t == DISPLAY_UNSET) {
        t = prompt_display_target();
        display_target_set(t);
    }
    return t;
}
