#ifndef PALETTE_H
#define PALETTE_H

/* One CLUT per (server palette_id, display target) pair -- matches
 * server/world.py's PALETTE_OVERWORLD/PALETTE_CAVE/PALETTE_PVP_REALM. As on
 * the Atari (apply_palette), each area recolors the same art. */
#define PALETTE_ID_COUNT 3
#define PALETTE_ID_OVERWORLD 0
#define PALETTE_ID_CAVE 1
#define PALETTE_ID_PVP_REALM 2

#define DISPLAY_RGB 0
#define DISPLAY_COMPOSITE 1

extern const unsigned char coco_clut[PALETTE_ID_COUNT][2][16];

#endif
