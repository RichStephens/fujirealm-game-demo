#ifndef TILES_H
#define TILES_H

#include "rt_state.h"
#include "gime.h"

#define TILE_ROW_BYTES ((unsigned)TILE_H * BYTES_PER_ROW)

/* Placeholder marker colors: CLUT slots (palette.c). */
#define MARK_PLAYER 3   /* white */
#define MARK_REMOTE 14  /* yellow */
#define MARK_ENEMY 11   /* red */
#define MARK_NPC 9      /* blue */
#define MARK_BULLET 12  /* orange */
/* Item drops: the Atari's three looks (gold, Warden Key, sticks for the rest).
 * Yellow and tan in the overworld and PvP palettes; the cave remaps them. */
#define MARK_ITEM_GOLD 14  /* yellow */
#define MARK_ITEM_KEY 3    /* white */
#define MARK_ITEM_OTHER 13 /* tan */

/* Marker color for an item drop. In redraw.c (game only). */
unsigned char item_mark_color(unsigned char item_id);

/* Fills one 16x16 tile (8 bytes wide, 16 lines at the 160-byte stride) at dst
 * inside the graphics window with a single color pair (fill16 = the 4bpp byte
 * doubled). Call inside GFX_ENTER()/GFX_LEAVE(). */
void fill_tile(unsigned char *dst, unsigned fill16);

/* Draws the live playfield: a PLAYFIELD_COLS x PLAYFIELD_ROWS viewport whose
 * top-left is (cam_x, cam_y) inside the terrain cache (origin_x/origin_y
 * are the cache's world origin), then item drops, enemies, remote players,
 * the local player at (px, py) and shots as small colored blocks over
 * flat-color tiles (color = tile id & 0x0F) -- placeholders until real art. Each tile row brackets
 * itself with GFX_ENTER()/GFX_LEAVE(). In redraw.c (game only). */
void draw_world(const unsigned char *terrain, unsigned origin_x,
                unsigned origin_y, unsigned char cam_x, unsigned char cam_y,
                unsigned char px, unsigned char py, const struct rt_state *st);

#endif
