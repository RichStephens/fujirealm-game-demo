#include "tiles.h"
#include "gime.h"
#include "terrain.h"
#include "controls.h"
#include "player.h"
#include "ovl_api.h"

void ovl_region_start(void)
{
}

unsigned char item_mark_color(unsigned char item_id)
{
    switch (rt_item_art_index(item_id)) {
    case RTS_ART_ITEM_GOLD:
        return MARK_ITEM_GOLD;
    case RTS_ART_ITEM_KEY:
        return MARK_ITEM_KEY;
    default:
        return MARK_ITEM_OTHER;
    }
}

/* A lines x (words * 4)-pixel block at offset inside the tile at world
 * (ex, ey), if that tile is in view. Inside the GFX bracket. */
static void draw_rect(unsigned view_x, unsigned view_y, unsigned char ex,
                      unsigned char ey, unsigned char color, unsigned offset,
                      unsigned char lines, unsigned char words)
{
    unsigned dx = (unsigned)ex - view_x;
    unsigned dy = (unsigned)ey - view_y;
    unsigned fill16;
    unsigned char *dst;
    unsigned char r;

    if (dx >= PLAYFIELD_COLS || dy >= PLAYFIELD_ROWS) {
        return;
    }
    fill16 = (unsigned)((color << 4) | color) * 0x0101U;
    dst = GFX_WINDOW + dy * TILE_ROW_BYTES + dx * (TILE_W / 2) + offset;
    for (r = 0; r < lines; ++r) {
        *(unsigned *)dst = fill16;
        if (words == 2) {
            *(unsigned *)(dst + 2) = fill16;
        }
        dst += BYTES_PER_ROW;
    }
}

/* 8x8 centered: entities. */
#define MARKER_RECT (4 * BYTES_PER_ROW + 2), 8, 2
/* 4x4 centered: shots. */
#define BULLET_RECT (6 * BYTES_PER_ROW + 3), 4, 1
/* 8x4 low in the tile: item drops. */
#define ITEM_RECT (11 * BYTES_PER_ROW + 2), 4, 2

void draw_world(const unsigned char *terrain, unsigned origin_x,
                unsigned origin_y, unsigned char cam_x, unsigned char cam_y,
                unsigned char px, unsigned char py, const struct rt_state *st)
{
    unsigned char col, row, i, tile;
    unsigned view_x = origin_x + cam_x;
    unsigned view_y = origin_y + cam_y;
    const unsigned char *src;

    /* One bracket per tile row (~10 ms), not one for the frame: interrupts
     * stay masked well under a 60 Hz tick, so the timer the walk cadence
     * counts is not starved by drawing. */
    for (row = 0; row < PLAYFIELD_ROWS; ++row) {
        controls_poll();
        src = &terrain[(unsigned)(cam_y + row) * BOOTSTRAP_WINDOW_W + cam_x];
        GFX_ENTER();
        for (col = 0; col < PLAYFIELD_COLS; ++col) {
            tile = (unsigned char)(src[col] & 0x0F);
            fill_tile(GFX_WINDOW + (unsigned)row * TILE_ROW_BYTES
                          + (unsigned)col * (TILE_W / 2),
                      (unsigned)((tile << 4) | tile) * 0x0101U);
        }
        GFX_LEAVE();
    }

    GFX_ENTER();
    for (i = 0; i < st->item_count; ++i) {
        if (st->items[i].item_id != 0) {
            draw_rect(view_x, view_y, st->items[i].x, st->items[i].y,
                      item_mark_color(st->items[i].item_id), ITEM_RECT);
        }
    }
    for (i = 0; i < st->beaver_count; ++i) {
        if (st->beavers[i].hp != 0 && !(st->beavers[i].hit_timer & 1)) {
            draw_rect(view_x, view_y, st->beavers[i].x, st->beavers[i].y,
                      st->beavers[i].kind >= RTS_KIND_WILHELM ? MARK_NPC
                                                               : MARK_ENEMY,
                      MARKER_RECT);
        }
    }
    for (i = 0; i < st->remote_count; ++i) {
        if (st->remotes[i].state & RTS_REMOTE_ALIVE) {
            draw_rect(view_x, view_y, st->remotes[i].x, st->remotes[i].y,
                      MARK_REMOTE, MARKER_RECT);
        }
    }
    if (st->world_seen && !(player_hit_timer & 1)) {
        draw_rect(view_x, view_y, px, py, MARK_PLAYER, MARKER_RECT);
    }
    for (i = 0; i < RTS_MAX_TRACERS; ++i) {
        if (st->tracers[i].active) {
            draw_rect(view_x, view_y, st->tracers[i].x, st->tracers[i].y,
                      MARK_BULLET, BULLET_RECT);
        }
    }
    GFX_LEAVE();
}
