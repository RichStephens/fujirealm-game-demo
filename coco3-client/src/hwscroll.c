#include "hwscroll.h"
#include "gime.h"
#include "tiles.h"
#include "player.h"
#include "terrain.h"
#include "hud.h"
#include "controls.h"
#include "ovl_api.h"
#include <coco.h>
#include <cmoc.h>

/* Two virtual screens (rings), each 15 physical blocks (464 lines of 256)
 * from its base block. Each step paints the hidden ring, HUD included, then
 * flips to it at vertical blank, so nothing is drawn on screen. */
#define RING_A_BLOCK 8
#define RING_B_BLOCK 23
#define RING_ROWS 15
#define RING_LINES (RING_ROWS * TILE_H)  /* 240 */
#define MIRROR_ROWS 14                   /* ring rows 0..13 are mirrored */
#define MIRROR_LINES (MIRROR_ROWS * TILE_H) /* 224 */
#define FRAME_HUD_TOP PLAYFIELD_LINES

#define MAX_MARKS 32
#define MAX_DIRTY 24

#define SHAPE_MARKER 0 /* 8x8 centered: entities */
#define SHAPE_BULLET 1 /* 4x4 centered: shots */
#define SHAPE_ITEM 2   /* 8x4 low in the tile: item drops */

struct mark {
    unsigned char x;
    unsigned char y;
    unsigned char color;
    unsigned char shape;
};

struct cell {
    unsigned char x;
    unsigned char y;
};

static const unsigned shape_offset[3] = { 4 * 256 + 2, 6 * 256 + 3, 11 * 256 + 2 };
static const unsigned char shape_lines[3] = { 8, 4, 4 };
static const unsigned char shape_words[3] = { 2, 1, 2 };

/* What one ring shows: it lags the other by a step, so each keeps its own
 * record. Only cells whose markers changed are repainted. */
struct ring {
    unsigned char base;
    unsigned char have;              /* painted at least once since hw_init */
    unsigned vx, vy;                 /* view it last showed */
    unsigned char painted[RING_ROWS * 16]; /* cell colors, two per byte */
    struct mark marks[MAX_MARKS];
    unsigned char nmarks;
    unsigned hud_line;               /* where its HUD was stamped */
    unsigned char hud_hoff;
    unsigned char hud_stale;         /* HUD text changed since stamped */
    unsigned char tiles_stale;       /* terrain changed since painted */
};

static struct ring rings[2];
static struct ring *cur;             /* the hidden ring being painted */
static unsigned char front;          /* index of the ring on screen */
static struct mark next[MAX_MARKS];
static struct cell dirty[MAX_DIRTY];
static unsigned char nnext;
static unsigned char ndirty;
static unsigned char dirty_overflow;

/* Fills one 16x16 tile at dst, 8 bytes wide with 256-byte rows. Inside the
 * GFX bracket. */
static void fill_tile256(unsigned char *dst, unsigned fill16)
{
    asm {
        ldx :dst
        ldd :fill16
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
        leax 256,x
        std ,x
        std 2,x
        std 4,x
        std 6,x
    }
}

/* Each 8K block holds 32 lines, and a tile row (16 lines) never straddles a
 * block, so painting a tile or a line needs exactly one window mapping. */
static void map_block(unsigned line)
{
    *(unsigned char *)0xFFAC = (unsigned char)(cur->base + (line >> 5));
}

static unsigned char painted_get(unsigned char row, unsigned char col)
{
    unsigned char b = cur->painted[(unsigned char)(row << 4) | (col >> 1)];

    if (col & 1) {
        return (unsigned char)(b & 0x0F);
    }
    return (unsigned char)(b >> 4);
}

static void painted_set(unsigned char row, unsigned char col,
                        unsigned char color)
{
    unsigned char *p = &cur->painted[(unsigned char)(row << 4) | (col >> 1)];

    if (col & 1) {
        *p = (unsigned char)((*p & 0xF0) | color);
    } else {
        *p = (unsigned char)((*p & 0x0F) | (color << 4));
    }
}

static void put_tile(unsigned line, unsigned char col, unsigned fill16)
{
    map_block(line);
    GFX_ENTER();
    fill_tile256(GFX_WINDOW + (line & 31) * 256 + col * (TILE_W / 2), fill16);
    GFX_LEAVE();
}

static unsigned fill_word(unsigned char color)
{
    return (unsigned)((color << 4) | color) * 0x0101U;
}

static unsigned char tile_color(const unsigned char *terrain, unsigned ox,
                                unsigned oy, unsigned wx, unsigned wy)
{
    unsigned rx = wx - ox;
    unsigned ry = wy - oy;

    if (rx >= BOOTSTRAP_WINDOW_W || ry >= BOOTSTRAP_WINDOW_H) {
        return 0;
    }
    return terrain[ry * BOOTSTRAP_WINDOW_W + rx] & 0x0F;
}

static void paint_tile(unsigned wx, unsigned wy, unsigned char color)
{
    unsigned char row = (unsigned char)(wy % RING_ROWS);
    unsigned char col = (unsigned char)(wx & 31);
    unsigned line = (unsigned)row * TILE_H;
    unsigned fill16 = fill_word(color);

    painted_set(row, col, color);
    put_tile(line, col, fill16);
    if (row < MIRROR_ROWS) {
        put_tile(line + RING_LINES, col, fill16);
    }
}

static unsigned char blast_n;
static unsigned blast_s;

/* Copies len bytes (a multiple of 4) from src to dst, twelve at a time
 * through the stack pointer, back to front. Inside the GFX bracket. */
static void blast(unsigned char *dst, const unsigned char *src,
                  unsigned char len)
{
    asm {
        pshs u,y
        ldx :dst
        ldy :src
        lda :len
        clrb
blast_div:
        cmpa #12
        blo blast_rem
        suba #12
        incb
        bra blast_div
blast_rem:
        tsta
        beq blast_chunks
blast_rloop:
        ldu ,y++
        stu ,x++
        suba #2
        bne blast_rloop
blast_chunks:
        tstb
        beq blast_done
        stb :blast_n
        lda #12
        mul
        leax d,x
        leay d,y
        sts :blast_s
        leau -6,y
        tfr x,s
blast_loop:
        pulu d,x,y
        pshs d,x,y
        leau -12,u
        pulu d,x,y
        pshs d,x,y
        leau -12,u
        dec :blast_n
        bne blast_loop
        lds :blast_s
blast_done:
        puls u,y
    }
}

/* Stamps the HUD into the hidden ring below the frame whose top-left is
 * world (vx, vy), writing only the copy of each line that frame shows. Only
 * text lines (hud.c rows at 3, 13, 23) move with a sideways scroll; blanks
 * != 0 also clears the empty lines (across the whole 256-byte row), needed
 * when the frame moved vertically. The image is in slot 4; the HUD's 33 ring
 * lines span at most two blocks, mapped in slots 5 and 6. */
static void paint_hud(unsigned vx, unsigned vy, unsigned char blanks)
{
    unsigned line = (vy % RING_ROWS) * TILE_H + FRAME_HUD_TOP;
    unsigned char hoff = (unsigned char)((vx & 31) * (TILE_W / 2));
    unsigned room = 256 - (unsigned)hoff;
    unsigned char seg1 = HUD_IMAGE_STRIDE;
    unsigned char block = (unsigned char)(cur->base + (line >> 5));
    unsigned char *row = GFX_WINDOW + 0x2000 + (line & 31) * 256;
    const unsigned char *src = GFX_WINDOW;
    unsigned char i;

    if (room < HUD_IMAGE_STRIDE) { /* not a ternary: CMOC sign-extends 160 */
        seg1 = (unsigned char)room;
    }
    *(unsigned char *)0xFFAC = HUD_IMAGE_BLOCK;
    *(unsigned char *)0xFFAD = block;
    *(unsigned char *)0xFFAE = (unsigned char)(block + 1);
    for (i = 0; i < HUD_LINES; ++i, row += 256, src += HUD_IMAGE_STRIDE) {
        if (i >= 3 && (unsigned char)((i - 3) % 10) < 8) {
            GFX_ENTER();
            blast(row + hoff, src, seg1);
            if (seg1 < HUD_IMAGE_STRIDE) {
                blast(row, src + seg1, (unsigned char)(HUD_IMAGE_STRIDE - seg1));
            }
            GFX_LEAVE();
        } else if (blanks) {
            GFX_ENTER();
            blast(row, GFX_WINDOW, HUD_IMAGE_STRIDE);
            blast(row + HUD_IMAGE_STRIDE, GFX_WINDOW, 256 - HUD_IMAGE_STRIDE);
            GFX_LEAVE();
        }
    }
}

void hw_hud_refresh(void)
{
    rings[0].hud_stale = 1;
    rings[1].hud_stale = 1;
}

static void draw_mark(const struct mark *m)
{
    unsigned char row = (unsigned char)(m->y % RING_ROWS);
    unsigned char col = (unsigned char)(m->x & 31);
    unsigned fill16 = fill_word(m->color);
    unsigned offset = shape_offset[m->shape];
    unsigned char lines = shape_lines[m->shape];
    unsigned char words = shape_words[m->shape];
    unsigned line;
    unsigned char *dst;
    unsigned char r;
    unsigned char pass;

    for (pass = 0; pass < 2; ++pass) {
        if (pass == 1 && row >= MIRROR_ROWS) {
            break;
        }
        line = (unsigned)row * TILE_H;
        if (pass) {
            line += RING_LINES; /* not `pass ? 240 : 0`: CMOC sign-extends the 240 */
        }
        map_block(line);
        GFX_ENTER();
        dst = GFX_WINDOW + (line & 31) * 256 + col * (TILE_W / 2) + offset;
        for (r = 0; r < lines; ++r) {
            *(unsigned *)dst = fill16;
            if (words == 2) {
                *(unsigned *)(dst + 2) = fill16;
            }
            dst += 256;
        }
        GFX_LEAVE();
    }
}

static void add_mark(unsigned vx, unsigned vy, unsigned char x, unsigned char y,
                     unsigned char color, unsigned char shape)
{
    struct mark *m;

    if ((unsigned)x - vx >= VIEW_COLS || (unsigned)y - vy >= VIEW_ROWS ||
        nnext >= MAX_MARKS) {
        return;
    }
    m = &next[nnext++];
    m->x = x;
    m->y = y;
    m->color = color;
    m->shape = shape;
}

static unsigned char has_mark(const struct mark *list, unsigned char n,
                              const struct mark *m)
{
    unsigned char i;

    for (i = 0; i < n; ++i) {
        if (list[i].x == m->x && list[i].y == m->y &&
            list[i].color == m->color && list[i].shape == m->shape) {
            return 1;
        }
    }
    return 0;
}

static void add_dirty(unsigned char x, unsigned char y)
{
    unsigned char i;

    for (i = 0; i < ndirty; ++i) {
        if (dirty[i].x == x && dirty[i].y == y) {
            return;
        }
    }
    if (ndirty >= MAX_DIRTY) {
        dirty_overflow = 1;
        return;
    }
    dirty[ndirty].x = x;
    dirty[ndirty].y = y;
    ++ndirty;
}

/* Draws every marker of this frame that sits on cell (x, y), in list order. */
static void draw_cell_marks(unsigned char x, unsigned char y)
{
    unsigned char i;

    for (i = 0; i < nnext; ++i) {
        if (next[i].x == x && next[i].y == y) {
            draw_mark(&next[i]);
        }
    }
}

/* Shows the hidden ring at (view_x, view_y), at vertical blank: the GIME
 * latches the start address at the top of the frame. */
static void set_scroll(unsigned view_x, unsigned view_y)
{
    unsigned start = (view_y % RING_ROWS) * TILE_H;
    unsigned video = ((unsigned)cur->base << 10) + start * 32;
    unsigned char hoff = (unsigned char)((view_x & 31) * 4);

    INTS_OFF();
    asm { lda $FF92 } /* acknowledge a pending GIME IRQ */
    asm { sync }
    *(unsigned *)0xFF9D = video;
    *(unsigned char *)0xFF9F = (unsigned char)(0x80 | hoff);
    INTS_RESTORE();
}

void hw_init(void)
{
    rings[0].base = RING_A_BLOCK;
    rings[1].base = RING_B_BLOCK;
    rings[0].have = 0;
    rings[1].have = 0;
    front = 0;
    *(unsigned *)0xFF9D = (unsigned)RING_A_BLOCK << 10;
    *(unsigned char *)0xFF9F = 0x80;
}

void hw_leave(void)
{
    *(unsigned char *)0xFF9F = 0;
}

static void paint_view(const unsigned char *terrain, unsigned ox, unsigned oy,
                       unsigned vx, unsigned vy, unsigned char skip_painted)
{
    unsigned char row, col;
    unsigned wx, wy;

    for (row = 0; row < VIEW_ROWS; ++row) {
        controls_poll();
        wy = vy + row;
        for (col = 0; col < VIEW_COLS; ++col) {
            wx = vx + col;
            if (skip_painted && wx - cur->vx < VIEW_COLS &&
                wy - cur->vy < VIEW_ROWS) {
                continue;
            }
            paint_tile(wx, wy, tile_color(terrain, ox, oy, wx, wy));
        }
    }
}

/* Repaints visible cells whose terrain color differs from what was painted. */
static void paint_changed(const unsigned char *terrain, unsigned ox,
                          unsigned oy, unsigned vx, unsigned vy)
{
    unsigned char row, col;
    unsigned wy;
    unsigned char color;
    const unsigned char *src;
    unsigned char ring_row;

    if (vx - ox > BOOTSTRAP_WINDOW_W - VIEW_COLS ||
        vy - oy > BOOTSTRAP_WINDOW_H - VIEW_ROWS) {
        return;
    }
    for (row = 0; row < VIEW_ROWS; ++row) {
        controls_poll();
        wy = vy + row;
        src = terrain + (wy - oy) * BOOTSTRAP_WINDOW_W + (vx - ox);
        ring_row = (unsigned char)(wy % RING_ROWS);
        for (col = 0; col < VIEW_COLS; ++col) {
            color = src[col] & 0x0F;
            if (painted_get(ring_row, (unsigned char)((vx + col) & 31)) != color) {
                paint_tile(vx + col, wy, color);
                add_dirty((unsigned char)(vx + col), (unsigned char)wy);
            }
        }
    }
}

/* Brings the hidden ring up to the view (vx, vy) with this frame's markers
 * in next[], then shows it. */
static void paint_ring(const unsigned char *terrain, unsigned ox, unsigned oy,
                       unsigned vx, unsigned vy, unsigned char full)
{
    unsigned char i;
    int ddx;
    int ddy;
    unsigned char x;
    unsigned char y;
    unsigned line = (vy % RING_ROWS) * TILE_H + FRAME_HUD_TOP;
    unsigned char hoff = (unsigned char)((vx & 31) * (TILE_W / 2));

    cur = &rings[front ^ 1];
    ddx = (int)(vx - cur->vx);
    ddy = (int)(vy - cur->vy);
    if (full || !cur->have || ddx >= VIEW_COLS || ddx <= -VIEW_COLS ||
        ddy >= VIEW_ROWS || ddy <= -VIEW_ROWS) {
        paint_view(terrain, ox, oy, vx, vy, 0);
        for (i = 0; i < nnext; ++i) {
            draw_mark(&next[i]);
        }
        paint_hud(vx, vy, 1);
    } else {
        ndirty = 0;
        dirty_overflow = 0;
        for (i = 0; i < cur->nmarks; ++i) {
            if (!has_mark(next, nnext, &cur->marks[i])) {
                add_dirty(cur->marks[i].x, cur->marks[i].y);
            }
        }
        for (i = 0; i < nnext; ++i) {
            if (!has_mark(cur->marks, cur->nmarks, &next[i])) {
                add_dirty(next[i].x, next[i].y);
            }
        }
        paint_view(terrain, ox, oy, vx, vy, 1);
        if (cur->tiles_stale) {
            paint_changed(terrain, ox, oy, vx, vy);
        }
        if (dirty_overflow) {
            for (i = 0; i < cur->nmarks; ++i) {
                x = cur->marks[i].x;
                y = cur->marks[i].y;
                if ((unsigned)x - vx < VIEW_COLS && (unsigned)y - vy < VIEW_ROWS) {
                    paint_tile(x, y, tile_color(terrain, ox, oy, x, y));
                }
            }
            for (i = 0; i < nnext; ++i) {
                draw_mark(&next[i]);
            }
        } else {
            for (i = 0; i < ndirty; ++i) {
                x = dirty[i].x;
                y = dirty[i].y;
                if ((unsigned)x - vx < VIEW_COLS && (unsigned)y - vy < VIEW_ROWS) {
                    paint_tile(x, y, tile_color(terrain, ox, oy, x, y));
                    draw_cell_marks(x, y);
                }
            }
        }
        if (cur->hud_line != line) {
            paint_hud(vx, vy, 1);
        } else if (cur->hud_hoff != hoff || cur->hud_stale) {
            paint_hud(vx, vy, 0);
        }
    }
    cur->vx = vx;
    cur->vy = vy;
    cur->have = 1;
    cur->hud_line = line;
    cur->hud_hoff = hoff;
    cur->hud_stale = 0;
    cur->tiles_stale = 0;
    memcpy(cur->marks, next, sizeof(struct mark) * nnext);
    cur->nmarks = nnext;

    set_scroll(vx, vy);
    front ^= 1;
}

void hw_present(const unsigned char *terrain, unsigned ox, unsigned oy,
                unsigned vx, unsigned vy, unsigned char px, unsigned char py,
                const struct rt_state *st, unsigned char mode)
{
    unsigned char i;

    if (mode == HW_PRESENT_TILES) {
        rings[0].tiles_stale = 1;
        rings[1].tiles_stale = 1;
    }
    nnext = 0;
    for (i = 0; i < st->item_count; ++i) {
        if (st->items[i].item_id != 0) {
            add_mark(vx, vy, st->items[i].x, st->items[i].y,
                     item_mark_color(st->items[i].item_id), SHAPE_ITEM);
        }
    }
    for (i = 0; i < st->beaver_count; ++i) {
        if (st->beavers[i].hp != 0 && !(st->beavers[i].hit_timer & 1)) {
            add_mark(vx, vy, st->beavers[i].x, st->beavers[i].y,
                     st->beavers[i].kind >= RTS_KIND_WILHELM ? MARK_NPC
                                                              : MARK_ENEMY,
                     SHAPE_MARKER);
        }
    }
    for (i = 0; i < st->remote_count; ++i) {
        if (st->remotes[i].state & RTS_REMOTE_ALIVE) {
            add_mark(vx, vy, st->remotes[i].x, st->remotes[i].y, MARK_REMOTE,
                     SHAPE_MARKER);
        }
    }
    if (st->world_seen && !(player_hit_timer & 1)) {
        add_mark(vx, vy, px, py, MARK_PLAYER, SHAPE_MARKER);
    }
    for (i = 0; i < RTS_MAX_TRACERS; ++i) {
        if (st->tracers[i].active) {
            add_mark(vx, vy, st->tracers[i].x, st->tracers[i].y, MARK_BULLET,
                     SHAPE_BULLET);
        }
    }

    paint_ring(terrain, ox, oy, vx, vy, mode == HW_PRESENT_FULL);
    /* A full repaint does both rings, so the next step is not a full one. */
    if (mode == HW_PRESENT_FULL) {
        paint_ring(terrain, ox, oy, vx, vy, 1);
    }
}

void ovl_region_end(void)
{
}
