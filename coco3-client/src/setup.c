#include "display.h"
#include "host.h"
#include "net.h"
#include "login.h"
#include "runm.h"
#include "ovl_api.h"
#include "art.h"
#include "prefs.h"
#include "identity.h"
#include "server_host_default.h"
#include <coco.h>
#include <cmoc.h>

/* Setup (stage 1): display target, host, and login/resume, all in 80-column
 * text mode. Once a login exists it launches the game (PLAY_PROGRAM, play.c).
 * If login fails, F2 edits the host; either way the machine restarts to try
 * again. */

static char g_host_buf[HOST_MAX_LEN + 1];
static struct ovl_handoff g_handoff;

/* Prompts for the host. A changed host is checked before the restart. */
static void edit_host(void)
{
    width(80); /* clears the screen */
    display_text_colors();
    if (host_edit_prompt(g_host_buf)) {
        net_check_host(g_host_buf, LOGIN_SERVER_PORT);
        printf("Press a key to restart.\n");
        waitkey(0);
    }
}

int main(void)
{
    char username[LOGIN_USERNAME_MAX + 1];
    char token_ascii[LOGIN_TOKEN_MAX + 1];

    initCoCoSupport();
    if (!isCoCo3) {
        printf("This program requires a CoCo 3.\n");
        return 0;
    }
    *(unsigned char *)0xFFD9 = 0; /* 1.79 MHz */
    width(80);
    display_text_colors();

    display_target_init();
    display_text_colors();
    host_init(g_host_buf);

    if (login_identity(g_host_buf, username, token_ascii)) {
        g_handoff.magic = OVL_HANDOFF_MAGIC;
        g_handoff.token = identity_token(token_ascii);
        g_handoff.items_seen = pref_items_seen_load(g_handoff.token);
        strcpy(g_handoff.host, g_host_buf);
        ovl_store(&g_handoff);
        art_store();
        runm(PLAY_PROGRAM);
    }

    printf("Login failed.\n");
    printf("F2: edit server host. Any other key: try again.\n");
    while (!isKeyPressed(KEY_PROBE_F2, KEY_BIT_F2) && !inkey()) {
    }
    if (isKeyPressed(KEY_PROBE_F2, KEY_BIT_F2)) {
        edit_host();
    }
    coldStart();
    return 0;
}
