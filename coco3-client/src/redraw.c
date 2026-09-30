#include "tiles.h"
#include "gime.h"
#include "terrain.h"
#include "controls.h"
#include "player.h"
#include "ovl_api.h"
#include "art.h"
#include "live.h"

void ovl_region_start(void)
{
}

unsigned char sprite_anim;

/* Species (rt_state.h RTS_KIND_*) -> entity image; kind_alt is the second
 * animation frame, 0 for none. The Lynx client's enemy_art tables. */
static const unsigned char kind_image[RTS_KIND_MAX + 1] = {
    0, 0, 1, 5, 3, 2, 7, 8, 8
};
static const unsigned char kind_alt[RTS_KIND_MAX + 1] = {
    0, 0, 0, 6, 4, 0, 0, 0, 9
};
/* Facing -> player frame (front 0, right 2, left 4, back 6); +1 is the walk
 * frame. No diagonal art: diagonals face right when odd, left when even
 * (Atari select_remote_facing_base). */
static const unsigned char facing_frame[RTS_FACE_COUNT] = {
    6, 0, 4, 2, 4, 2, 4, 2
};

static struct sprite *sp_out;
static unsigned char sp_n;
static unsigned sp_vx;
static unsigned sp_vy;

static void add_sprite(unsigned char x, unsigned char y, unsigned char img)
{
    struct sprite *s;

    if ((unsigned)x - sp_vx >= PLAYFIELD_COLS ||
        (unsigned)y - sp_vy >= PLAYFIELD_ROWS || sp_n >= MAX_SPRITES) {
        return;
    }
    s = &sp_out[sp_n++];
    s->x = x;
    s->y = y;
    s->img = img;
}

unsigned char sprites_build(struct sprite *out, const struct rt_state *st,
                            unsigned vx, unsigned vy, unsigned char px,
                            unsigned char py, unsigned char facing)
{
    unsigned char i;
    unsigned char k;
    unsigned char img;

    sp_out = out;
    sp_n = 0;
    sp_vx = vx;
    sp_vy = vy;
    /* Items lie on the ground, under whatever stands on them. */
    for (i = 0; i < st->item_count; ++i) {
        k = rt_item_art_index(st->items[i].item_id);
        if (k != RTS_ART_ITEM_NONE) {
            add_sprite(st->items[i].x, st->items[i].y,
                       (unsigned char)(ART_FIRST_ENTITY + ART_ENT_BULLET + k));
        }
    }
    for (i = 0; i < st->beaver_count; ++i) {
        if (st->beavers[i].hp != 0 && !(st->beavers[i].hit_timer & 1)) {
            k = st->beavers[i].kind;
            if (k > RTS_KIND_MAX) {
                k = RTS_KIND_BEAVER;
            }
            img = kind_image[k];
            if (sprite_anim && kind_alt[k] != 0) {
                img = kind_alt[k];
            }
            add_sprite(st->beavers[i].x, st->beavers[i].y,
                       (unsigned char)(ART_FIRST_ENTITY + img));
        }
    }
    for (i = 0; i < st->remote_count; ++i) {
        if (st->remotes[i].state & RTS_REMOTE_ALIVE) {
            add_sprite(st->remotes[i].x, st->remotes[i].y,
                       (unsigned char)(ART_RECOLOR | (ART_FIRST_PLAYER +
                                       facing_frame[st->remotes[i].facing & 7] +
                                       (st->remotes[i].anim & 1))));
        }
    }
    if (st->world_seen && !(player_hit_timer & 1)) {
        add_sprite(px, py,
                   (unsigned char)(ART_FIRST_PLAYER + facing_frame[facing & 7] +
                                   (player_anim & 1)));
    }
    for (i = 0; i < RTS_MAX_TRACERS; ++i) {
        if (st->tracers[i].active) {
            add_sprite(st->tracers[i].x, st->tracers[i].y,
                       ART_FIRST_ENTITY + ART_ENT_BULLET);
        }
    }
    return sp_n;
}

static unsigned char spr_rows;
static unsigned spr_skip;
static unsigned char recolor_buf[ART_IMAGE_BYTES];
/* Other players: the light blue tunic (12) turns red (11), its dark blue
 * trim (10) maroon (7). */
static const unsigned char remote_color[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 11, 11, 13, 14, 15
};

/* ART_RECOLOR images go through recolor_buf, with remote_color. */
void put_sprite(unsigned char *dst, const unsigned char *art,
                       unsigned char img, unsigned stride)
{
    const unsigned char *src = ART_IMAGE(art, img);
    unsigned char i;
    unsigned char b;

    if (img & ART_RECOLOR) {
        for (i = 0; i < ART_IMAGE_BYTES; ++i) {
            b = src[i];
            recolor_buf[i] = (unsigned char)((remote_color[b >> 4] << 4) |
                                             remote_color[b & 0x0F]);
        }
        src = recolor_buf;
    }
    draw_sprite(dst, src, stride);
}

void draw_sprite(unsigned char *dst, const unsigned char *src, unsigned stride)
{
    spr_skip = stride - 8;
    asm {
        pshs y
        ldx :src
        ldy :dst
        lda #16
        sta :spr_rows
spr_row
        ldb #8
spr_byte
        lda ,x+
        beq spr_next
        bita #$F0
        beq spr_low
        bita #$0F
        beq spr_high
        sta ,y
        bra spr_next
spr_low
        pshs a
        lda ,y
        anda #$F0
        ora ,s+
        sta ,y
        bra spr_next
spr_high
        pshs a
        lda ,y
        anda #$0F
        ora ,s+
        sta ,y
spr_next
        leay 1,y
        decb
        bne spr_byte
        ldd :spr_skip
        leay d,y
        dec :spr_rows
        bne spr_row
        puls y
    }
}

static unsigned char copy_n;
static struct sprite sprites[MAX_SPRITES];

void copy_tile(unsigned char *dst, const unsigned char *src, unsigned stride)
{
    asm {
        pshs y
        ldx :src
        ldy :dst
        lda #16
        sta :copy_n
copy_row
        ldd ,x
        std ,y
        ldd 2,x
        std 2,y
        ldd 4,x
        std 4,y
        ldd 6,x
        std 6,y
        leax 8,x
        ldd :stride
        leay d,y
        dec :copy_n
        bne copy_row
        puls y
    }
}

void draw_world(const unsigned char *terrain, unsigned origin_x,
                unsigned origin_y, unsigned char cam_x, unsigned char cam_y,
                unsigned char px, unsigned char py, const struct rt_state *st)
{
    unsigned char col, row, i, block, n, row_y;
    unsigned view_x = origin_x + cam_x;
    unsigned view_y = origin_y + cam_y;
    unsigned ofs;
    unsigned char base = gime_draw_block();
    const unsigned char *src;
    unsigned char *dst;
    unsigned char *row_start;

    n = sprites_build(sprites, st, view_x, view_y, px, py, live_facing);
    /* A tile row's screen lines span at most two blocks: those go in slots 4
     * and 5, the art in slot 6. Each row's sprites follow its tiles at once,
     * so on the single 128K buffer they are not seen missing. One bracket per
     * tile row, not one for the frame: interrupts stay masked well under a
     * 60 Hz tick, so the timer the walk cadence counts is not starved. */
    for (row = 0; row < PLAYFIELD_ROWS; ++row) {
        controls_poll();
        src = &terrain[(unsigned)(cam_y + row) * BOOTSTRAP_WINDOW_W + cam_x];
        ofs = (unsigned)row * TILE_ROW_BYTES;
        block = (unsigned char)(base + (ofs >> 13));
        *(unsigned char *)0xFFAC = block;
        *(unsigned char *)0xFFAD = (unsigned char)(block + 1);
        *(unsigned char *)0xFFAE = ART_BLOCK;
        row_start = GFX_WINDOW + (ofs & 0x1FFF);
        dst = row_start;
        row_y = (unsigned char)(view_y + row);
        GFX_ENTER();
        for (col = 0; col < PLAYFIELD_COLS; ++col, dst += TILE_W / 2) {
            copy_tile(dst, ART_TILE_IMAGE(GFX_WINDOW + 0x4000, src[col]),
                      BYTES_PER_ROW);
        }
        for (i = 0; i < n; ++i) {
            if (sprites[i].y == row_y) {
                put_sprite(row_start + (sprites[i].x - view_x) * (TILE_W / 2),
                           GFX_WINDOW + 0x4000, sprites[i].img, BYTES_PER_ROW);
            }
        }
        GFX_LEAVE();
    }
    gime_window_playfield();
}
