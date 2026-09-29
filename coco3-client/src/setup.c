#include "gime.h"
#include "palette.h"
#include "testpat.h"
#include "ram.h"
#include "display.h"
#include "host.h"
#include "net.h"
#include "login.h"
#include "runm.h"
#include "ovl_api.h"
#include "prefs.h"
#include "identity.h"
#include "server_host_default.h"
#include <coco.h>
#include <cmoc.h>

/* Setup (stage 1): display target, host, and login/resume, all in 80-column
 * text mode. Once a login exists it launches the game (PLAY_PROGRAM, play.c). If
 * login fails, F2 edits the host and any other key opens the graphics test
 * screen, where F1 toggles the palette and F2 edits the host. */

/* Border color doubles as the Phase 2 on-screen readout: dark green for
 * 512K/double-buffered, red for 128K/single-buffered. $FF9A takes a
 * direct 6-bit hardware color code (0-63), NOT an index into
 * coco_clut[]'s slot numbers -- these are the raw codes the overworld
 * CLUT happens to hold at slots 15 and 11 (see palette.c). */
#define BORDER_512K 2  /* dark green -- coco_clut[PALETTE_ID_OVERWORLD][*][15] */
#define BORDER_128K 36 /* red -- coco_clut[PALETTE_ID_OVERWORLD][*][11] */

/* ~2 flips/sec at the 60Hz tick in getTimer()/setTimer() (coco.h). */
#define FLIP_TICKS 30

static unsigned char f1_was_down;
static unsigned char f2_was_down;
static unsigned char g_is_512k;
static unsigned char g_display_target;
static char g_host_buf[HOST_MAX_LEN + 1];

static void apply_palette(void)
{
    gime_set_palette(coco_clut[PALETTE_ID_OVERWORLD][g_display_target]);
}

/* F1 is the dedicated display-target toggle key. Only the palette needs
 * reapplying -- tile data references palette indices, not raw colors. */
static void check_display_toggle(void)
{
    unsigned char down = isKeyPressed(KEY_PROBE_F1, KEY_BIT_F1) ? 1 : 0;

    if (down && !f1_was_down) {
        g_display_target = display_target_toggle();
        apply_palette();
    }
    f1_was_down = down;
}

/* F2 is the dedicated host-edit key, same pattern as F1. Plain text I/O
 * (the prompt, the throughput readout) only works outside graphics mode,
 * so this leaves it temporarily and re-enters afterward.
 * gime_init_mode()/gime_select_buffers() don't depend on prior state, so
 * calling them again is safe. */
static void check_host_edit(void)
{
    unsigned char down = isKeyPressed(KEY_PROBE_F2, KEY_BIT_F2) ? 1 : 0;

    if (down && !f2_was_down) {
        *(unsigned char *)0xFF98 = 0; /* graphics mode off */
        width(80);
        display_text_colors();

        if (host_edit_prompt(g_host_buf))
            net_measure_throughput(g_host_buf, HYBRID_SERVER_PORT);

        gime_init_mode();
        apply_palette();
        gime_select_buffers(g_is_512k);
        *(unsigned char *)0xFF9A = g_is_512k ? BORDER_512K : BORDER_128K;
    }
    f2_was_down = down;
}

int main(void)
{
    char username[LOGIN_USERNAME_MAX + 1];
    char token_ascii[LOGIN_TOKEN_MAX + 1];
    unsigned char phase;
    unsigned int next_tick;

    initCoCoSupport();
    if (!isCoCo3) {
        printf("This program requires a CoCo 3.\n");
        return 0;
    }
    *(unsigned char *)0xFFD9 = 0; /* 1.79 MHz */
    width(80);
    display_text_colors();

    g_display_target = display_target_init();
    display_text_colors();
    host_init(g_host_buf);

    if (login_identity(g_host_buf, username, token_ascii)) {
        ovl_store(pref_items_seen_load(identity_token(token_ascii)));
        runm(PLAY_PROGRAM);
    }

    printf("Login failed.\n");
    printf("F2: edit server host. Any other key: test screen.\n");
    while (!isKeyPressed(KEY_PROBE_F2, KEY_BIT_F2) && !inkey()) {
    }
    if (isKeyPressed(KEY_PROBE_F2, KEY_BIT_F2)) {
        width(80); /* clears the screen */
        display_text_colors();
        if (host_edit_prompt(g_host_buf)) {
            net_measure_throughput(g_host_buf, HYBRID_SERVER_PORT);
        }
        printf("Press a key to restart.\n");
        waitkey(0);
        coldStart();
    }

    gime_init_mode();
    apply_palette();

    g_is_512k = ram_probe_is_512k();
    gime_select_buffers(g_is_512k);
    *(unsigned char *)0xFF9A = g_is_512k ? BORDER_512K : BORDER_128K;

    if (!g_is_512k) {
        draw_test_screen(0);
        while (1) {
            check_display_toggle();
            check_host_edit();
        }
    }

    phase = 0;
    next_tick = getTimer();
    while (1) {
        draw_test_screen(phase);
        gime_flip_display();
        phase = (unsigned char)(phase ^ 1);

        next_tick = (unsigned int)(next_tick + FLIP_TICKS);
        while ((int)(getTimer() - next_tick) < 0) {
            check_display_toggle();
            check_host_edit();
        }
    }

    return 0;
}
