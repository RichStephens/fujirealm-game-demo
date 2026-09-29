#ifndef PALETTE_H
#define PALETTE_H

/* One CLUT per (server palette_id, display target) pair -- matches
 * server/world.py's PALETTE_OVERWORLD/PALETTE_CAVE/PALETTE_PVP_REALM
 * (same count Lynx's ART_PALETTE_COUNT uses). Modeled on
 * atari8-client/fujirealm.asm's apply_palette: that client gives cave and
 * PvP their OWN hand-tuned colors even though it reuses the SAME tile
 * shapes for all three (no per-area art exists yet there either) --
 * recoloring alone is worth doing before real per-area art exists. Lynx's
 * art_clut[] duplicating one palette across ids is a shortcut its small
 * display/memory can get away with; CoCo3 doesn't need to take it. */
#define PALETTE_ID_COUNT 3
#define PALETTE_ID_OVERWORLD 0
#define PALETTE_ID_CAVE 1
#define PALETTE_ID_PVP_REALM 2

#define DISPLAY_RGB 0
#define DISPLAY_COMPOSITE 1

extern const unsigned char coco_clut[PALETTE_ID_COUNT][2][16];

#endif
