#ifndef OVL_API_H
#define OVL_API_H

#include "host.h"

/* Full-screen windows (help, map, inventory, dialogue, quest offer) run as an
 * overlay: code built separately at the address of the renderers' code (the
 * region between ovl_region_start in redraw.c and ovl_region_end in
 * hwscroll.c), which is idle while a window is open. FRLOGIN stores the
 * overlay image in OVL_BLOCK; the game copies it over the region, calls its
 * entry, then puts the renderers back. */

#define OVL_BLOCK 5

/* What FRLOGIN hands the game, at OVL_HANDOFF_OFS in OVL_BLOCK. */
#define OVL_HANDOFF_OFS 0x1F00
#define OVL_HANDOFF_MAGIC 0x4C

struct ovl_handoff {
    unsigned char magic;
    unsigned char items_seen; /* pref_items_seen_load() */
    unsigned long token;
    char host[HOST_MAX_LEN + 1];
};
#define OVL_MAGIC0 'F'
#define OVL_MAGIC1 'O'

/* Which window the overlay should run. */
#define OVL_HELP 0
#define OVL_DIALOGUE 1
#define OVL_QUEST_OFFER 2
#define OVL_MAP 3
#define OVL_INVENTORY 4

struct rt_state;

/* At the start of OVL_BLOCK, followed by len bytes to load at load. */
struct ovl_header {
    unsigned char magic[2];
    unsigned load;
    unsigned entry;
    unsigned len;
};

/* What the game lends the overlay. The window draws on a black playfield
 * (40 x 24 characters) and must call service() on every pass. */
struct ovl_api {
    void (*service)(void);
    void (*clear)(void);
    void (*text)(unsigned char col, unsigned char row, const char *s);
    void (*show)(void);
    unsigned char fire_mask; /* selected stick's button 1, or 0 */
    struct rt_state *game;
    unsigned char *pickup;   /* live_pickup_counter */
    unsigned char *decline;  /* live_dlg_decline */
};

void ovl_region_start(void);
void ovl_region_end(void);

/* FRLOGIN: copies its built-in overlay image and h into OVL_BLOCK. */
void ovl_store(const struct ovl_handoff *h);

#endif
