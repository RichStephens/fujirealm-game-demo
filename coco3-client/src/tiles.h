#ifndef TILES_H
#define TILES_H

#include "rt_state.h"
#include "gime.h"

#define TILE_ROW_BYTES ((unsigned)TILE_H * BYTES_PER_ROW)

/* Items, creatures, other players, the local player and shots in view. */
#define MAX_SPRITES 32

struct sprite {
    unsigned char x;
    unsigned char y;
    unsigned char img; /* art image number (art.h) */
};

/* Creature animation phase (0 or 1), set by the game loop. */
extern unsigned char sprite_anim;

/* Fills out with what is in the view whose top-left is world (vx, vy), in
 * drawing order, and returns the count. (px, py) and facing are the local
 * player's. In redraw.c (game only), as are the rest. */
unsigned char sprites_build(struct sprite *out, const struct rt_state *st,
                            unsigned vx, unsigned vy, unsigned char px,
                            unsigned char py, unsigned char facing);

/* Copies a 16x16 art image (8-byte rows) to dst, rows stride bytes apart.
 * Inside the GFX bracket. */
void copy_tile(unsigned char *dst, const unsigned char *src, unsigned stride);

/* As copy_tile, but color 0 pixels leave dst as it was. */
void draw_sprite(unsigned char *dst, const unsigned char *src, unsigned stride);

/* draw_sprite of sprite image img from the art mapped at window address art,
 * in the other-player colors when img has ART_RECOLOR. */
void put_sprite(unsigned char *dst, const unsigned char *art, unsigned char img,
                unsigned stride);

/* Draws the live playfield: a PLAYFIELD_COLS x PLAYFIELD_ROWS viewport whose
 * top-left is (cam_x, cam_y) inside the terrain cache (origin_x/origin_y
 * are the cache's world origin) from the art block, with the sprites
 * sprites_build() lists. Each tile row brackets itself with
 * GFX_ENTER()/GFX_LEAVE(). */
void draw_world(const unsigned char *terrain, unsigned origin_x,
                unsigned origin_y, unsigned char cam_x, unsigned char cam_y,
                unsigned char px, unsigned char py, const struct rt_state *st);

#endif
