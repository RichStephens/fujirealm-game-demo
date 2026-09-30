#ifndef ART_H
#define ART_H

/* The art block: 16x16 4bpp images, 128 bytes each (8 per 16-line row, 16
 * rows), color 0 transparent where drawn over terrain. FRLOGIN installs it.
 * 512K uses block 38; a 128K MMU decodes only 4 block bits, so there it is
 * block 6, BASIC's 80-column screen. */
#define ART_BLOCK 38
#define ART_MAGIC0 'F'
#define ART_MAGIC1 'A'
#define ART_TILE_MAP 0x10 /* 64 entries: tile id -> image (dead ids: grass) */
#define ART_IMAGES 0x80   /* image 0 */
#define ART_IMAGE_BYTES 128
/* 8 player frames: front, right, left, back, each standing then stepping.
 * Other players use them too, with the tunic recolored (redraw.c). */
#define ART_FIRST_PLAYER 35
#define ART_FIRST_ENTITY 43 /* 16 images, tools/art_pack.py ENTITIES order */
#define ART_ENT_BULLET 10   /* then the items, by rt_item_art_index() */
#define ART_RECOLOR 0x80    /* sprite img flag: draw with the remote colors */

/* FRLOGIN: copies its built-in art into ART_BLOCK. */
void art_store(void);

/* Image img, in art mapped at window address art. */
#define ART_IMAGE(art, img) \
    ((art) + ART_IMAGES + (unsigned)((img) & 0x7F) * ART_IMAGE_BYTES)

/* The image for tile id, in art mapped at window address art. */
#define ART_TILE_IMAGE(art, id) \
    ((art) + ART_IMAGES + (unsigned)(art)[ART_TILE_MAP + ((id) & 63)] * ART_IMAGE_BYTES)

#endif
