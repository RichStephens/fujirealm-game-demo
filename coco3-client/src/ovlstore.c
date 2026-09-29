#include "ovl_api.h"
#include "gime.h"
#include <cmoc.h>

extern const unsigned char ovl_image[];
extern const unsigned ovl_image_len;

void ovl_store(unsigned char items_seen)
{
    gime_init_task1();
    *(unsigned char *)0xFFAC = OVL_BLOCK;
    GFX_ENTER();
    memcpy(GFX_WINDOW, ovl_image, ovl_image_len);
    GFX_WINDOW[OVL_ITEMS_SEEN_OFS] = items_seen;
    GFX_LEAVE();
}
