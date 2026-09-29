#include "testpat.h"
#include "tiles.h"
#include "gime.h"
#include <cmoc.h>

static void draw_hud_bars(void)
{
    /* Three solid color bars standing in for real HUD text/icons, each
     * spanning the full 320-pixel (160-byte) width. */
    static const unsigned char bar_colors[3] = {1, 4, 9};
    unsigned char i, r;
    unsigned char *dst;
    unsigned char fill;
    unsigned char bar_lines = HUD_LINES / 3;

    gime_window_hud();
    GFX_ENTER();
    for (i = 0; i < 3; ++i) {
        fill = (unsigned char)((bar_colors[i] << 4) | bar_colors[i]);
        for (r = 0; r < bar_lines; ++r) {
            dst = HUD_WINDOW + (unsigned int)(i * bar_lines + r) * BYTES_PER_ROW;
            memset(dst, fill, BYTES_PER_ROW);
        }
    }
    GFX_LEAVE();
    gime_window_playfield();
}

static void draw_playfield_pattern(unsigned char phase)
{
    unsigned char col, row;
    unsigned char color;
    unsigned char shift = (unsigned char)(phase * 8);

    /* One bracket per row keeps interrupts masked only briefly. */
    for (row = 0; row < PLAYFIELD_ROWS; ++row) {
        GFX_ENTER();
        for (col = 0; col < PLAYFIELD_COLS; ++col) {
            color = (unsigned char)((row * PLAYFIELD_COLS + col + shift) & 0x0F);
            fill_tile(GFX_WINDOW + (unsigned)row * TILE_ROW_BYTES
                          + (unsigned)col * (TILE_W / 2),
                      (unsigned)((color << 4) | color) * 0x0101U);
        }
        GFX_LEAVE();
    }
}

void draw_test_screen(unsigned char phase)
{
    draw_playfield_pattern(phase);
    draw_hud_bars();
}
