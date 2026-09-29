#include "ram.h"
#include "gime.h"

/* Block 22 only exists as independent RAM on 512K+ (needs 23 * 8KB =
 * 184KB, past 128K's 16-block ceiling); block 6 exists on every CoCo3 RAM
 * size and is unused by anything else here (not part of either RAM
 * size's own top-8 default, nor of gime.c's chosen graphics buffers). On
 * a 128K (16-block) machine the MMU can only decode 4 bits of block
 * number, so block 22 physically aliases block 6 (22 mod 16 == 6) --
 * writing through block 22 then reading back through the SAME register
 * value shows whether that write actually landed on independent RAM or
 * just clobbered block 6. */
#define PROBE_HIGH_BLOCK 22
#define PROBE_LOW_BLOCK 6

unsigned char ram_probe_is_512k(void)
{
    unsigned char result;

    GFX_ENTER();
    *(unsigned char *)0xFFAC = PROBE_HIGH_BLOCK;
    *GFX_WINDOW = 0xAA;
    *(unsigned char *)0xFFAC = PROBE_LOW_BLOCK;
    *GFX_WINDOW = 0x55;
    *(unsigned char *)0xFFAC = PROBE_HIGH_BLOCK;
    result = (unsigned char)(*GFX_WINDOW == 0xAA);
    GFX_LEAVE();

    return result;
}
