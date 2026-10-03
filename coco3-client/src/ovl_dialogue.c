#include "ovl_api.h"
#include "rt_state.h"
#include <coco.h>

/* The Atari's show_dialogue_modal and show_quest_offer_modal. */

#define DLG_LEFT_MARGIN 3
#define DLG_WRAP_WIDTH 34
#define DLG_SPEAKER_ROW 4
#define DLG_BODY_ROW 8
#define DLG_PROMPT_ROW 20
#define DLG_PAGE_ROW 22

#define ACT_NONE 0
#define ACT_ACCEPT 1
#define ACT_DECLINE 2

static unsigned char prev_action;

/* Fire, ENTER or SPACE accepts; BREAK declines. A held stick button reads as
 * keys on its PIA row, so SPACE (same row as left button 2) needs the check. */
static unsigned char read_action(const struct ovl_api *api)
{
    unsigned char buttons = readJoystickButtons();

    if (isKeyPressed(KEY_PROBE_BREAK, KEY_BIT_BREAK)) {
        return ACT_DECLINE;
    }
    if ((buttons & api->fire_mask) != api->fire_mask ||
        isKeyPressed(KEY_PROBE_ENTER, KEY_BIT_ENTER) ||
        ((buttons & 0x0F) == 0x0F &&
         isKeyPressed(KEY_PROBE_SPACE, KEY_BIT_SPACE))) {
        return ACT_ACCEPT;
    }
    return ACT_NONE;
}

/* Returns an action once per press. */
static unsigned char poll_action(const struct ovl_api *api)
{
    unsigned char a = read_action(api);

    if (a == prev_action) {
        return ACT_NONE;
    }
    prev_action = a;
    return a;
}

/* Returns only once nothing is held, so the game does not act on the key
 * that closed the window (BREAK would exit to BASIC). */
static void wait_release(const struct ovl_api *api)
{
    while (read_action(api) != ACT_NONE) {
        api->service();
    }
}

static const char *speaker_name(unsigned char speaker)
{
    switch (speaker) {
    case RTS_SPEAKER_NERISSA:
        return "Nerissa";
    case RTS_SPEAKER_DANIEL:
        return "Daniel";
    case RTS_SPEAKER_WILHELM:
        return "Wilhelm";
    case RTS_SPEAKER_LUCIAN:
        return "Lucian";
    case RTS_SPEAKER_GRIX:
        return "Grix";
    }
    return "?";
}

/* Word-wraps text at DLG_WRAP_WIDTH columns, as dialogue_draw_body does. */
static void draw_body(const struct ovl_api *api, const char *text)
{
    char line[DLG_WRAP_WIDTH + 1];
    unsigned char row = DLG_BODY_ROW;
    unsigned char col = 0;
    unsigned char len;

    while (*text) {
        if (*text == ' ') {
            ++text;
            if (col != 0 && col < DLG_WRAP_WIDTH) {
                line[col++] = ' ';
            }
            continue;
        }
        for (len = 0; text[len] != 0 && text[len] != ' '; ++len) {
        }
        if (col != 0 && col + len > DLG_WRAP_WIDTH) {
            line[col] = 0;
            api->text(DLG_LEFT_MARGIN, row++, line);
            col = 0;
        }
        while (*text && *text != ' ') {
            if (col == DLG_WRAP_WIDTH) {
                line[col] = 0;
                api->text(DLG_LEFT_MARGIN, row++, line);
                col = 0;
            }
            line[col++] = *text++;
        }
    }
    if (col != 0) {
        line[col] = 0;
        api->text(DLG_LEFT_MARGIN, row, line);
    }
}

static void render_page(const struct ovl_api *api, const struct rt_dialogue *d)
{
    char count[4];
    const char *prompt = "ENTER or fire - Next";

    if (d->flags & RTS_DLG_FLAG_LAST_PAGE) {
        prompt = "ENTER or fire - Done";
        if (d->flags & RTS_DLG_FLAG_QUEST_OFFER) {
            prompt = "ENTER/fire - Yes  BREAK - No";
        }
    }
    count[0] = (char)('1' + d->page_index);
    count[1] = '/';
    count[2] = (char)('0' + d->page_count);
    count[3] = 0;

    api->clear();
    api->text(DLG_LEFT_MARGIN, DLG_SPEAKER_ROW, speaker_name(d->speaker));
    draw_body(api, d->text);
    api->text(DLG_LEFT_MARGIN, DLG_PROMPT_ROW, prompt);
    api->text(DLG_LEFT_MARGIN, DLG_PAGE_ROW, count);
    api->show();
}

/* Movement is frozen but the connection keeps running. The server paces the
 * pages: each is acked with a pickup bump, then the next is awaited. BREAK
 * works even while waiting, in case the next page never comes. */
void show_dialogue(const struct ovl_api *api)
{
    struct rt_dialogue *d = &api->game->dlg;
    unsigned char shown = RTS_DLG_SHOWN_NONE;
    unsigned char waiting = 0;
    unsigned char a;

    *api->decline = 0;
    d->active = 1;
    prev_action = read_action(api);
    for (;;) {
        api->service();
        if (read_action(api) == ACT_DECLINE) {
            *api->decline = RTS_BUTTON_DIALOGUE_DECLINE;
            ++*api->pickup;
            break;
        }
        if (d->dirty) {
            d->dirty = 0;
            if (d->page_index != shown) {
                shown = d->page_index;
                waiting = 0;
                render_page(api, d);
            }
        }
        if (waiting || shown == RTS_DLG_SHOWN_NONE) {
            continue;
        }
        a = poll_action(api);
        if (a == ACT_ACCEPT) {
            ++*api->pickup;
            if (d->flags & RTS_DLG_FLAG_LAST_PAGE) {
                break;
            }
            waiting = 1;
        }
    }
    d->active = 0;
    d->closed = 1;
    d->closed_id = d->id;
    d->closed_page = d->page_index;
    wait_release(api);
}

/* The server's "NEW QUEST" message: accepting bumps the pickup counter. */
void show_quest_offer(const struct ovl_api *api)
{
    api->clear();
    api->text(15, 3, "New Quest");
    api->text(0, 7, api->game->quest_text);
    api->text(0, 11, api->game->message);
    api->text(4, 16, "Press ENTER or fire to accept");
    api->show();

    prev_action = read_action(api);
    while (poll_action(api) != ACT_ACCEPT) {
        api->service();
    }
    ++*api->pickup;
    wait_release(api);
}
