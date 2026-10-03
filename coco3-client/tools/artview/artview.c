#include "gime.h"
#include "palette.h"
#include "art.h"
#include <coco.h>
#include <cmoc.h>

/* Art viewer: every image in the art block, doubled, with its name and use,
 * then a sample scene. Boots from its own disk (ARTVIEW.dsk); nothing is
 * saved. */

extern const unsigned char art_image[];

#define FONT ((const unsigned char *)0xF09D) /* ROM font copy, chars 32-127 */
#define WHITE 3
#define PER_PAGE 8
#define TILE_IMAGES 35
#define RECOLOR 0x80 /* entry img flag: draw as another player */
/* Every write through the window stays under line 200 ($FD00): above
 * $FE00 are the interrupt vectors and the I/O registers. */

struct entry {
    unsigned char img;
    const char *name;
    const char *use1;
    const char *use2;
};

/* Images 0-34 are the terrain tiles in id order (0, 2-17, 34-51), 35-42 the
 * player frames, 43-58 the creatures and pickups (tools/art_pack.py). */
static const struct entry entries[] = {
    { 0, "Grass", "Open ground", "(cave floor)" },
    { 1, "Tree", "Blocks you;", "shoot to chop" },
    { 2, "Herb", "Heals 1 HP", "when walked on" },
    { 3, "Damaged tree", "A tree being", "chopped down" },
    { 4, "Stump", "What a felled", "tree leaves" },
    { 5, "Shot (tile)", "Projectile", "on the map" },
    { 6, "Map edge", "Hedge and rock", "around the map" },
    { 7, "Beaver (tile)", "Beaver in the", "terrain stream" },
    { 8, "Snake (tile)", "Snake in the", "terrain stream" },
    { 9, "Road", "Dirt road", "" },
    { 10, "Water", "Blocks the way", "" },
    { 11, "Building", "Town houses,", "in blocks" },
    { 12, "Cave entrance", "Leads down to", "the cave map" },
    { 13, "Grave", "Respawn point", "after you die" },
    { 14, "Cave floor", "Open cave", "ground" },
    { 15, "Cave wall", "Solid rock", "" },
    { 16, "Cave exit", "Steps back up", "to daylight" },
    { 17, "Gold (tile)", "Coins in the", "terrain stream" },
    { 18, "Sticks (tile)", "Bridge timber", "for Wilhelm" },
    { 19, "Goblin (tile)", "Hostile goblin", "" },
    { 20, "Villager", "Townsperson", "" },
    { 21, "Grix", "Kind goblin", "with a warning" },
    { 22, "Warden Key", "Opens the way", "to the pump" },
    { 23, "Daniel", "Farmer; offers", "Road Trouble" },
    { 24, "Wilhelm", "Carpenter;", "fixes bridges" },
    { 25, "Lucian", "Ranger at the", "marsh lookout" },
    { 26, "Nerissa", "Town elder;", "starts story" },
    { 27, "Slime (tile)", "Frame 1", "" },
    { 28, "Slime (tile)", "Frame 2", "" },
    { 29, "Bat (tile)", "Frame 1", "" },
    { 30, "Bat (tile)", "Frame 2", "" },
    { 31, "Gorvak (tile)", "Pumpmaster,", "final boss" },
    { 32, "Deep Pump", "Floodworks", "machine" },
    { 33, "Pump controls", "Shut the Deep", "Pump down here" },
    { 34, "Wilhelm (tile)", "At work on", "the bridge" },
    { 35, "You: front", "Walking down,", "standing" },
    { 36, "You: front", "Walking down,", "stepping" },
    { 37, "You: right", "Standing", "" },
    { 38, "You: right", "Stepping", "" },
    { 39, "You: left", "Standing", "" },
    { 40, "You: left", "Stepping", "" },
    { 41, "You: back", "Walking up,", "standing" },
    { 42, "You: back", "Walking up,", "stepping" },
    { RECOLOR | 35, "Other player", "Same frames,", "red tunic" },
    { RECOLOR | 38, "Other player", "Facing right,", "stepping" },
    { 43, "Beaver", "Enemy: dams", "the streams" },
    { 44, "Snake", "Enemy: marsh", "snake" },
    { 45, "Goblin", "Enemy", "" },
    { 46, "Slime", "Enemy, frame 1", "" },
    { 47, "Slime", "Enemy, frame 2", "" },
    { 48, "Bat", "Enemy, frame 1", "(cave)" },
    { 49, "Bat", "Enemy, frame 2", "" },
    { 50, "Gorvak", "Pumpmaster,", "final boss" },
    { 51, "Wilhelm", "Walking to", "the bridge" },
    { 52, "Wilhelm", "Hammering at", "the bridge" },
    { 53, "Shot", "Arrows, yours", "and others'" },
    { 54, "Gold", "Dropped coins", "" },
    { 55, "Sticks", "Dropped sticks", "" },
    { 56, "Herb", "Dropped herb", "" },
    { 57, "Potion", "Heals you", "" },
    { 58, "Warden Key", "Dropped key", "" },
};
#define ENTRY_COUNT (sizeof(entries) / sizeof(entries[0]))
#define ENTRY_PAGES ((ENTRY_COUNT + PER_PAGE - 1) / PER_PAGE)

/* A sample corner of the world: tile images by letter, then sprites. */
#define SCENE_ROWS 10
static const char *scene[SCENE_ROWS] = {
    "EEEEEEEEEEEEEEEEEEEE",
    "EggTTggggggggHgggTTE",
    "EgTTTggSggRRRRRgTTTE",
    "EggTggggggRggggggTgE",
    "EgggggBBBgRggWWWgggE",
    "ERRRRRBBBRRggWWWWggE",
    "EggggRggtgRgggWWgCgE",
    "EgXgGRggggggHggggggE",
    "EggHgRRRRRRRRRggggDE",
    "EEEEEEEEEEEEEEEEEEEE",
};
static const char scene_letter[] = "gTHSRBWXGCEtD";
static const unsigned char scene_image[] = {
    0, 1, 2, 4, 9, 11, 10, 13, 17, 12, 6, 3, 23
};
struct scene_sprite {
    unsigned char x, y, img;
};
static const struct scene_sprite scene_sprites[] = {
    { 7, 6, 35 }, { 12, 3, RECOLOR | 37 }, { 4, 3, 43 }, { 15, 7, 45 },
    { 2, 5, 55 }, { 9, 7, 54 }, { 6, 6, 53 }, { 11, 1, 46 },
};

static unsigned char palette_id;
static unsigned char target; /* 0 RGB, 1 composite */
static unsigned char page;
static const unsigned char remote_color[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 11, 11, 13, 14, 15
};

static const unsigned char *image(unsigned char img)
{
    return art_image + ART_IMAGES + (unsigned)(img & 0x7F) * ART_IMAGE_BYTES;
}

static void apply_palette(void)
{
    gime_set_palette(coco_clut[palette_id][target]);
}

/* Byte blitters over 16 rows of 8-byte image data, set up through these. */
static const unsigned char *b_src;
static unsigned char *b_dst;
static unsigned b_stride;
static const unsigned char *b_table;
static unsigned char b_rows;
static unsigned char b_cnt;
static const unsigned char doubled[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};
static unsigned char buf[ART_IMAGE_BYTES];
static unsigned char spr[ART_IMAGE_BYTES];

/* b_src to b_dst, rows b_stride apart. */
static void blit_copy(void)
{
    asm {
        pshs y
        ldx :b_src
        ldy :b_dst
        lda #16
        sta :b_rows
bc_row
        ldd ,x
        std ,y
        ldd 2,x
        std 2,y
        ldd 4,x
        std 4,y
        ldd 6,x
        std 6,y
        leax 8,x
        ldd :b_stride
        leay d,y
        dec :b_rows
        bne bc_row
        puls y
    }
}

/* As blit_copy, but color 0 pixels leave b_dst as it was. */
static void blit_sprite(void)
{
    b_stride -= 8;
    asm {
        pshs y
        ldx :b_src
        ldy :b_dst
        lda #16
        sta :b_rows
bs_row
        ldb #8
bs_byte
        lda ,x+
        beq bs_next
        bita #$F0
        beq bs_low
        bita #$0F
        beq bs_high
        sta ,y
        bra bs_next
bs_low
        pshs a
        lda ,y
        anda #$F0
        ora ,s+
        sta ,y
        bra bs_next
bs_high
        pshs a
        lda ,y
        anda #$0F
        ora ,s+
        sta ,y
bs_next
        leay 1,y
        decb
        bne bs_byte
        ldd :b_stride
        leay d,y
        dec :b_rows
        bne bs_row
        puls y
    }
}

/* b_src doubled both ways onto the screen at b_dst (160-byte rows). U is
 * the frame pointer, so everything is loaded before it is borrowed. */
static void blit_double(void)
{
    b_table = doubled;
    asm {
        pshs u,y
        ldx :b_src
        ldy :b_dst
        ldu :b_table
        lda #16
        sta :b_rows
bd_row
        lda #8
        sta :b_cnt
bd_byte
        lda ,x
        lsra
        lsra
        lsra
        lsra
        lda a,u
        sta ,y
        sta 160,y
        lda ,x+
        anda #$0F
        lda a,u
        sta 1,y
        sta 161,y
        leay 2,y
        dec :b_cnt
        bne bd_byte
        leay 304,y
        dec :b_rows
        bne bd_row
        puls u,y
    }
}

/* img's data, with other-player colors when it has RECOLOR. */
static const unsigned char *pixels(unsigned char img)
{
    const unsigned char *src = image(img);
    unsigned char i;
    unsigned char b;

    if (!(img & RECOLOR)) {
        return src;
    }
    for (i = 0; i < ART_IMAGE_BYTES; ++i) {
        b = src[i];
        spr[i] = (unsigned char)((remote_color[b >> 4] << 4) |
                                 remote_color[b & 0x0F]);
    }
    return spr;
}

/* Draws img at byte x, line y. Doubled (scale 2), sprites (over) are shown
 * on grass; at scale 1 they go over whatever is already on screen. */
static void draw(unsigned char x, unsigned char y, unsigned char img,
                 unsigned char scale, unsigned char over)
{
    b_src = pixels(img);
    if (scale == 2 && over) {
        memcpy(buf, image(0), ART_IMAGE_BYTES);
        b_dst = buf;
        b_stride = 8;
        blit_sprite();
        b_src = buf;
    }
    b_dst = GFX_WINDOW + (unsigned)y * BYTES_PER_ROW + x;
    GFX_ENTER();
    if (scale == 2) {
        blit_double();
    } else {
        b_stride = BYTES_PER_ROW;
        if (over) {
            blit_sprite();
        } else {
            blit_copy();
        }
    }
    GFX_LEAVE();
}

static void text(unsigned char col, unsigned char line, const char *s)
{
    unsigned char g[8];
    unsigned char tbl[4];
    unsigned char *dst;
    unsigned char r, b;

    tbl[0] = 0;
    tbl[1] = WHITE;
    tbl[2] = WHITE << 4;
    tbl[3] = (WHITE << 4) | WHITE;
    for (; *s; ++s, ++col) {
        memcpy(g, FONT + (unsigned)(*s - 32) * 8, 8);
        dst = GFX_WINDOW + (unsigned)line * BYTES_PER_ROW + col * 4;
        GFX_ENTER();
        for (r = 0; r < 8; ++r, dst += BYTES_PER_ROW) {
            b = g[r];
            dst[0] = tbl[b >> 6];
            dst[1] = tbl[(b >> 4) & 3];
            dst[2] = tbl[(b >> 2) & 3];
            dst[3] = tbl[b & 3];
        }
        GFX_LEAVE();
    }
}

static void clear(void)
{
    unsigned line;

    for (line = 0; line < 200; ++line) {
        GFX_ENTER();
        memset(GFX_WINDOW + line * BYTES_PER_ROW, 0, BYTES_PER_ROW);
        GFX_LEAVE();
    }
}

/* Clears the whole 225-line buffer, past what the window may touch: its
 * 5 blocks one at a time through slot 4. */
static void clear_buffer(void)
{
    unsigned char base = gime_draw_block();
    unsigned char k;
    unsigned ofs;

    for (k = 0; k < 5; ++k) {
        *(unsigned char *)0xFFAC = (unsigned char)(base + k);
        for (ofs = 0; ofs < GFX_BLOCK_BYTES; ofs += 1024) {
            GFX_ENTER();
            memset(GFX_WINDOW + ofs, 0, 1024);
            GFX_LEAVE();
        }
    }
    gime_window_playfield();
}

static const char *palette_names[3] = { "Overworld", "Cave", "PvP realm" };

static void footer(void)
{
    text(1, 172, "<- ->  change page");
    text(28, 172, "BREAK quit");
    text(1, 182, "1 2 3  area colors:");
    text(21, 182, palette_names[palette_id]);
    text(1, 192, "F1     display:");
    text(17, 192, target ? "composite" : "RGB");
}

static void show_page(void)
{
    const struct entry *e;
    unsigned char i, slot, x, y, col;
    char num[3];

    clear();
    if (page == ENTRY_PAGES) {
        text(1, 0, "Sample scene");
        for (y = 0; y < SCENE_ROWS; ++y) {
            for (x = 0; x < 20; ++x) {
                i = (unsigned char)(strchr(scene_letter, scene[y][x]) - scene_letter);
                draw((unsigned char)(x * 8), (unsigned char)(12 + y * 16),
                     scene_image[i], 1, 0);
            }
        }
        for (i = 0; i < sizeof(scene_sprites) / sizeof(scene_sprites[0]); ++i) {
            draw((unsigned char)(scene_sprites[i].x * 8),
                 (unsigned char)(12 + scene_sprites[i].y * 16),
                 scene_sprites[i].img, 1, 1);
        }
    } else {
        num[0] = (char)('1' + page);
        num[1] = 0;
        text(1, 0, "Art, page");
        text(11, 0, num);
        for (slot = 0; slot < PER_PAGE; ++slot) {
            i = (unsigned char)(page * PER_PAGE + slot);
            if (i >= ENTRY_COUNT) {
                break;
            }
            e = &entries[i];
            x = (unsigned char)((slot & 1) * 76);
            col = (unsigned char)((slot & 1) * 19 + 6);
            y = (unsigned char)(12 + (slot >> 1) * 42);
            draw((unsigned char)(x + 4), y, e->img, 2,
                 (unsigned char)((e->img & 0x7F) >= TILE_IMAGES));
            text(col, (unsigned char)(y + 4), e->name);
            text(col, (unsigned char)(y + 14), e->use1);
            text(col, (unsigned char)(y + 22), e->use2);
        }
    }
    footer();
}

/* A warm start to BASIC's OK prompt, as the game's BREAK does: the loaded
 * program overwrote the BASIC program area, so that is emptied first. */
static void exit_to_basic(void)
{
    unsigned txttab = *(unsigned *)0x19;

    *(unsigned char *)0xFF98 = 0; /* graphics mode off */
    *(unsigned *)txttab = 0;
    *(unsigned *)0x1B = txttab + 2; /* VARTAB */
    *(unsigned *)0x1D = txttab + 2; /* ARYTAB */
    *(unsigned *)0x1F = txttab + 2; /* ARYEND */
    asm {
        orcc #$50
        jmp [$FFFE]
    }
}

static unsigned char pressed(unsigned char probe, unsigned char bit)
{
    return isKeyPressed(probe, bit) ? 1 : 0;
}

int main(void)
{
    unsigned char keys, last = 0;

    initCoCoSupport();
    *(unsigned char *)0xFFD9 = 0; /* 1.79 MHz */
    gime_init_mode();
    gime_select_buffers(0);
    clear_buffer();
    apply_palette();
    show_page();

    for (;;) {
        keys = (unsigned char)(pressed(KEY_PROBE_RIGHT, KEY_BIT_RIGHT) |
                               pressed(KEY_PROBE_SPACE, KEY_BIT_SPACE) |
                               pressed(KEY_PROBE_LEFT, KEY_BIT_LEFT) << 1 |
                               pressed(KEY_PROBE_1, KEY_BIT_1) << 2 |
                               pressed(KEY_PROBE_2, KEY_BIT_2) << 3 |
                               pressed(KEY_PROBE_3, KEY_BIT_3) << 4 |
                               pressed(KEY_PROBE_F1, KEY_BIT_F1) << 5 |
                               pressed(KEY_PROBE_BREAK, KEY_BIT_BREAK) << 6);
        if (keys != last && keys != 0) {
            if (keys & 1) {
                page = (unsigned char)((page + 1) % (ENTRY_PAGES + 1));
                show_page();
            } else if (keys & 2) {
                page = (unsigned char)((page + ENTRY_PAGES) % (ENTRY_PAGES + 1));
                show_page();
            } else if (keys & 0x1C) {
                palette_id = (unsigned char)((keys & 4) ? 0 : (keys & 8) ? 1 : 2);
                apply_palette();
                show_page();
            } else if (keys & 0x20) {
                target ^= 1;
                apply_palette();
                show_page();
            } else if (keys & 0x40) {
                exit_to_basic();
            }
        }
        last = keys;
    }
    return 0;
}
