#include "rt_state.h"
#include <cmoc.h>

/* CRC-16/CCITT-FALSE computed bit by bit (a table version needs 512 bytes
 * the CoCo game cannot spare), in assembly with the CRC held in D. */
unsigned rt_crc16(const unsigned char *data, unsigned char len)
{
    unsigned crc;

    asm {
        pshs y
        ldx :data
        ldd #$FFFF
        tst :len
        beq rt_crc_done
rt_crc_byte:
        eora ,x+
        ldy #8
rt_crc_bit:
        lslb
        rola
        bcc rt_crc_next
        eora #$10
        eorb #$21
rt_crc_next:
        leay -1,y
        bne rt_crc_bit
        dec :len
        bne rt_crc_byte
rt_crc_done:
        puls y
        std :crc
    }
    return crc;
}

unsigned char rt_cobs_decode(const unsigned char *in, unsigned char in_len,
                             unsigned char *out)
{
    unsigned char read = 0;
    unsigned char write = 0;
    unsigned char code;
    unsigned char count;

    while (read < in_len) {
        code = in[read++];
        if (code == 0 || (unsigned)read + code - 1 > in_len) {
            return 0;
        }
        count = code - 1;
        while (count-- != 0) {
            out[write++] = in[read++];
        }
        if (code != 0xFF && read < in_len) {
            out[write++] = 0;
        }
    }
    return write;
}

unsigned char rt_cobs_encode(const unsigned char *in, unsigned char in_len,
                             unsigned char *out)
{
    unsigned char read = 0;
    unsigned char write = 1;
    unsigned char code_index = 0;
    unsigned char code = 1;

    while (read < in_len) {
        if (in[read] == 0) {
            out[code_index] = code;
            code_index = write++;
            code = 1;
            ++read;
        } else {
            out[write++] = in[read++];
            ++code;
        }
    }
    out[code_index] = code;
    return write;
}

void rt_state_init(struct rt_state *state)
{
    unsigned i;
    unsigned char *bytes = (unsigned char *)state;

    for (i = 0; i < sizeof(struct rt_state); ++i) {
        bytes[i] = 0;
    }
}

static void apply_world_state(struct rt_state *state, unsigned char *terrain,
                              unsigned origin_x, unsigned origin_y,
                              const unsigned char *w)
{
    unsigned char i;
    unsigned char count = w[3];
    unsigned tile_x;
    unsigned tile_y;

    state->world_seen = 1;
    state->last_server_seq = (unsigned)w[4] | ((unsigned)w[5] << 8);
    state->player_x = w[6];
    state->player_y = w[7];
    state->health = w[8];
    state->correction_flags = w[9];
    state->echo_client_seq = (unsigned)w[10] | ((unsigned)w[11] << 8);
    if (count > RTS_MAX_BEAVERS) {
        count = RTS_MAX_BEAVERS;
    }
    state->beaver_count = count;
    for (i = 0; i < count; ++i) {
        unsigned char kind = w[18 + i * 4];

        state->beavers[i].x = w[15 + i * 4];
        state->beavers[i].y = w[16 + i * 4];
        state->beavers[i].hp = w[17 + i * 4];
        /* Bit 7 is a one-shot "damage just landed" pulse, not part of the
           species. The blink it arms is timed locally: the server sends the
           pulse once and never repeats it. */
        state->beavers[i].kind = kind & RTS_KIND_MASK;
        if (kind & RTS_KIND_HIT_PULSE) {
            state->beavers[i].hit_timer = RTS_HIT_FLASH_FRAMES;
        }
    }

    /* Live terrain cell update (chopped tree, item drop, ...). Absolute
     * coordinates; only cells inside the bootstrap window are ours. */
    if (terrain != 0) {
        tile_x = w[13];
        tile_y = w[14];
        if (tile_x >= origin_x && tile_x < origin_x + RTS_WINDOW_W &&
            tile_y >= origin_y && tile_y < origin_y + RTS_WINDOW_H) {
            unsigned offset = (tile_y - origin_y) * RTS_WINDOW_W +
                              (tile_x - origin_x);
            /* Only report a change when the cell really differs: every
               WORLD_STATE carries these fields, and treating each one as a
               change would force a full repaint ten times a second. */
            if (terrain[offset] != w[12]) {
                terrain[offset] = w[12];
                state->tile_changed = 1;
            }
        }
    }
}

static void apply_hud_update(struct rt_state *state, const unsigned char *w)
{
    state->hud_seen = 1;
    state->hud_hp = w[6];
    state->hud_max_hp = w[7];
    state->hud_level = w[8];
    /* Payload: hp, max_hp, level, xp(2), xp_next(2), gold(2), flags, kills(2).
     * gold is w[13..14]; flags w[15]; pvp kills w[16..17]. */
    state->hud_gold = (unsigned)w[13] | ((unsigned)w[14] << 8);
    state->hud_pvp_enabled = (w[15] & RTS_HUD_FLAG_PVP_ENABLED) != 0;
    state->hud_pvp_kills = (unsigned)w[16] | ((unsigned)w[17] << 8);
}

static void apply_message(struct rt_state *state, const unsigned char *w)
{
    unsigned char i;
    unsigned char len = w[7];

    if (len > RTS_TEXT_MAX) {
        len = RTS_TEXT_MAX;
    }
    for (i = 0; i < len; ++i) {
        state->message[i] = (char)w[8 + i];
    }
    state->message[len] = 0;
    state->message_len = len;
    state->message_id = w[6];
    state->message_dirty = 1;
    /* Deliberately not conditional on w[6] (message_id): id 0 is a real
       message. See message_seen in rt_state.h. */
    state->message_seen = 1;
}

static void apply_quest_update(struct rt_state *state, const unsigned char *w)
{
    unsigned char i;
    unsigned char len = w[8];

    if (len > RTS_TEXT_MAX) {
        len = RTS_TEXT_MAX;
    }
    for (i = 0; i < len; ++i) {
        state->quest_text[i] = (char)w[9 + i];
    }
    state->quest_text[len] = 0;
    state->quest_len = len;
    state->quest_id = w[6];
    state->quest_state = w[7];
    state->quest_seen = 1;
    state->quest_dirty = 1;
}

/* Fold one chunk into the pending display page. Mirrors the Atari client's
 * netstream_apply_dialogue_page exactly; the ordering
 * of these four rules is the contract, not an implementation detail. */
static void apply_dialogue_page(struct rt_state *state, const unsigned char *w)
{
    unsigned char chunk = w[11];
    unsigned char len = w[12];
    unsigned char i;

    /* 1. Chunk 0 restarts the page. This is also how a retransmit arrives, so
          it must be safe to replay a page we have already assembled. */
    if (chunk == 0) {
        state->dlg.len = 0;
        state->dlg.next_chunk = 0;
    }
    /* 2. A gap means we lost a chunk. Drop it silently and wait for the
          server's whole-page resend rather than assembling torn text. */
    if (chunk != state->dlg.next_chunk) {
        return;
    }

    state->dlg.id = w[6];
    state->dlg.speaker = w[7];
    state->dlg.page_index = w[8];
    state->dlg.page_count = w[9];

    /* 3. Append, clamped both per chunk and per page. */
    if (len > RTS_DLG_CHUNK_MAX) {
        len = RTS_DLG_CHUNK_MAX;
    }
    for (i = 0; i < len && state->dlg.len < RTS_DLG_PAGE_MAX; ++i) {
        state->dlg.text[state->dlg.len++] = (char)w[13 + i];
    }
    state->dlg.text[state->dlg.len] = 0;
    ++state->dlg.next_chunk;

    /* 4. Commit the flags only from the chunk that carries CHUNK_END. Every
          earlier chunk of the page repeats a partial flags byte, and a
          retransmit's chunk 0 carries no LAST_PAGE at all -- taking flags from
          those is what froze the Atari client on a quest turn-in, because the
          prompt lost its accept/decline state mid-page. */
    if ((w[10] & RTS_DLG_FLAG_CHUNK_END) == 0) {
        return;
    }
    state->dlg.flags = w[10];
    state->dlg.dirty = 1;
    if (!state->dlg.active) {
        state->dlg.request = 1;
    }
}

unsigned char rt_item_art_index(unsigned char item_id)
{
    switch (item_id) {
    case 0:
        return RTS_ART_ITEM_NONE;
    case RTS_ITEM_GOLD:
        return RTS_ART_ITEM_GOLD;
    case RTS_ITEM_HERB:
        return RTS_ART_ITEM_HERB;
    case RTS_ITEM_POTION:
        return RTS_ART_ITEM_POTION;
    case RTS_ITEM_WARDEN_KEY:
        return RTS_ART_ITEM_KEY;
    default:
        /* Sticks stands in for anything we have no art for yet (oil and rust
           samples today), so an unknown drop is still visible and walkable-
           over rather than invisible. */
        return RTS_ART_ITEM_STICKS;
    }
}

void rt_spawn_tracer(struct rt_state *state, unsigned char x, unsigned char y,
                     unsigned char dir)
{
    unsigned char slot = state->tracer_next;

    state->tracers[slot].active = 1;
    state->tracers[slot].x = x;
    state->tracers[slot].y = y;
    state->tracers[slot].dir = dir;
    state->tracers[slot].steps = 0;
    ++slot;
    if (slot >= RTS_MAX_TRACERS) {
        slot = 0;
    }
    state->tracer_next = slot;
}

static void apply_remote_players(struct rt_state *state,
                                 const unsigned char *w)
{
    unsigned char i;
    unsigned char old_count = state->remote_count;
    unsigned char count = w[3];
    unsigned char nx;
    unsigned char ny;
    unsigned char nfacing;
    unsigned char nstate;
    unsigned char fire;

    if (count > RTS_MAX_REMOTE_PLAYERS) {
        count = RTS_MAX_REMOTE_PLAYERS;
    }
    for (i = 0; i < count; ++i) {
        nx = w[6 + i * 4];
        ny = w[7 + i * 4];
        nfacing = w[8 + i * 4];
        nstate = w[9 + i * 4];
        fire = nstate & RTS_REMOTE_STATE_FIRE_MASK;
        /* A change in the fire bits on a slot that was already occupied means
         * that remote fired: draw a cosmetic tracer from its cell and facing.
         * A freshly occupied slot only establishes a baseline (no tracer).
         * Facings 4-7 are followed too now that the stepper walks diagonals --
         * without that a remote's diagonal shots were silently invisible. */
        if (i < old_count && (state->remotes[i].state & RTS_REMOTE_ALIVE) &&
            (nstate & RTS_REMOTE_ALIVE) &&
            fire != state->remote_prev_fire[i] &&
            nfacing < RTS_FACE_COUNT) {
            rt_spawn_tracer(state, nx, ny, nfacing);
        }
        state->remote_prev_fire[i] = fire;
        state->remotes[i].x = nx;
        state->remotes[i].y = ny;
        state->remotes[i].facing = nfacing;
        state->remotes[i].state = nstate;
    }
    /* Slots that emptied this frame must forget their baseline so the next
     * occupant is treated as new rather than inheriting a stale fire count. */
    for (i = count; i < old_count; ++i) {
        state->remote_prev_fire[i] = 0;
    }
    state->remote_count = count;
}

static void apply_item_drops(struct rt_state *state, const unsigned char *w)
{
    unsigned char i;
    unsigned char count = w[3];

    if (count > RTS_MAX_ITEMS) {
        count = RTS_MAX_ITEMS;
    }
    state->item_count = count;
    for (i = 0; i < count; ++i) {
        state->items[i].x = w[6 + i * 4];
        state->items[i].y = w[7 + i * 4];
        state->items[i].item_id = w[8 + i * 4];
        state->items[i].quantity = w[9 + i * 4];
    }
}

/* The wire's count, gold and slots match rt_inventory's layout from count on.
 * Sets valid, clamps count, and ORs each slot's item bit into seen. */
static void apply_inventory(struct rt_state *state, const unsigned char *w)
{
    struct rt_inventory *inv = &state->inv;

    memcpy(&inv->count, w + 6, 3 + RTS_INV_SLOTS * 2);
    asm {
        pshs y
        ldx :inv
        lda #1
        sta ,x
        ldb 2,x
        cmpb #8
        bls inv_ok
        ldb #8
        stb 2,x
inv_ok
        leay 5,x
inv_loop
        tstb
        beq inv_done
        lda ,y++
        anda #7
        pshs b
        ldb #1
inv_shift
        tsta
        beq inv_or
        aslb
        deca
        bra inv_shift
inv_or
        orb 1,x
        stb 1,x
        puls b
        decb
        bra inv_loop
inv_done
        puls y
    }
}

static void apply_map_summary(struct rt_state *state, const unsigned char *w)
{
    unsigned n = (unsigned)w[9] * w[10];
    unsigned char i;

    if (n > RTS_MAP_CELLS) {
        return;
    }
    state->map.width = w[9];
    state->map.height = w[10];
    for (i = 0; i < n; ++i) {
        state->map.cells[i] = w[12 + i];
    }
    state->map.valid = 1;
}

static void apply_terrain_edge(struct rt_state *state, const unsigned char *w)
{
    unsigned char i;
    unsigned char count = w[3];

    if (count > RTS_EDGE_MAX_TILES) {
        count = RTS_EDGE_MAX_TILES;
    }
    state->edge.origin_x = w[6];
    state->edge.origin_y = w[7];
    state->edge.width = w[8];
    state->edge.height = w[9];
    state->edge.revision = (unsigned)w[10] | ((unsigned)w[11] << 8);
    state->edge.tile_count = count;
    for (i = 0; i < count; ++i) {
        state->edge.tiles[i] = w[12 + i];
    }
}

static void apply_window_row(struct rt_state *state, const unsigned char *w)
{
    unsigned char i;

    state->window_row.origin_x = w[6];
    state->window_row.origin_y = w[7];
    state->window_row.row_index = w[8];
    state->window_row.fill_id = w[9];
    for (i = 0; i < RTS_WINDOW_W; ++i) {
        state->window_row.tiles[i] = w[10 + i];
    }
}

static void apply_window_commit_ack(struct rt_state *state,
                                    const unsigned char *w)
{
    state->window_commit_ack.fill_id = w[6];
    state->window_commit_ack.origin_x = w[7];
    state->window_commit_ack.origin_y = w[8];
}

static void apply_map_change(struct rt_state *state, const unsigned char *w)
{
    state->map_change.map_id = w[6];
    state->map_change.spawn_x = w[7];
    state->map_change.spawn_y = w[8];
    state->map_change.tileset_id = w[9];
    state->map_change.palette_id = w[10];
    state->map_change.flags = w[11];
}

unsigned char rt_apply(struct rt_state *state, unsigned char *terrain,
                       unsigned origin_x, unsigned origin_y,
                       const unsigned char *raw, unsigned char raw_len)
{
    static unsigned char w[RTS_WORKSPACE];
    unsigned char i;
    unsigned expected_crc;

    if (raw_len < 8 || raw_len > RTS_MAX_RAW || raw[1] != RTS_VERSION ||
        raw[0] + 8 != raw_len) {
        return RTS_INVALID;
    }
    expected_crc = (unsigned)raw[raw_len - 2] |
                   ((unsigned)raw[raw_len - 1] << 8);
    if (rt_crc16(raw, raw_len - 2) != expected_crc) {
        return RTS_INVALID;
    }
    /* Re-pad the stripped trailing zeros into a fixed workspace. */
    for (i = 0; i < raw_len - 2; ++i) {
        w[i] = raw[i];
    }
    for (i = raw_len - 2; i < RTS_WORKSPACE; ++i) {
        w[i] = 0;
    }

    switch (w[2]) {
    case RTS_WORLD_STATE:
        apply_world_state(state, terrain, origin_x, origin_y, w);
        return RTS_WORLD_STATE;
    case RTS_HUD_UPDATE:
        apply_hud_update(state, w);
        return RTS_HUD_UPDATE;
    case RTS_MESSAGE:
        apply_message(state, w);
        return RTS_MESSAGE;
    case RTS_QUEST_UPDATE:
        apply_quest_update(state, w);
        return RTS_QUEST_UPDATE;
    case RTS_DIALOGUE_PAGE:
        apply_dialogue_page(state, w);
        return RTS_DIALOGUE_PAGE;
    case RTS_REMOTE_PLAYERS:
        apply_remote_players(state, w);
        return RTS_REMOTE_PLAYERS;
    case RTS_ITEM_DROPS:
        apply_item_drops(state, w);
        return RTS_ITEM_DROPS;
    case RTS_TERRAIN_EDGE:
        apply_terrain_edge(state, w);
        return RTS_TERRAIN_EDGE;
    case RTS_WINDOW_ROW:
        apply_window_row(state, w);
        return RTS_WINDOW_ROW;
    case RTS_WINDOW_COMMIT_ACK:
        apply_window_commit_ack(state, w);
        return RTS_WINDOW_COMMIT_ACK;
    case RTS_MAP_CHANGE:
        apply_map_change(state, w);
        return RTS_MAP_CHANGE;
    case RTS_INVENTORY_UPDATE:
        apply_inventory(state, w);
        return RTS_INVENTORY_UPDATE;
    case RTS_MAP_SUMMARY:
        apply_map_summary(state, w);
        return RTS_MAP_SUMMARY;
    default:
        return 0;
    }
}

static unsigned char finish_frame(unsigned char *out, unsigned char *raw,
                                  unsigned char payload_len)
{
    unsigned crc;
    unsigned char encoded_len;

    while (payload_len != 0 && raw[5 + payload_len] == 0) {
        --payload_len;
    }
    raw[0] = payload_len;
    crc = rt_crc16(raw, payload_len + 6);
    raw[payload_len + 6] = (unsigned char)(crc & 0xFF);
    raw[payload_len + 7] = (unsigned char)(crc >> 8);
    encoded_len = rt_cobs_encode(raw, payload_len + 8, out);
    out[encoded_len++] = 0;
    return encoded_len;
}

unsigned char rt_build_auth(unsigned char *out, unsigned long token)
{
    unsigned char raw[12];

    raw[1] = RTS_VERSION;
    raw[2] = RTS_AUTH;
    raw[3] = 0;
    raw[4] = 0;
    raw[5] = 0;
    raw[6] = (unsigned char)(token & 0xFF);
    raw[7] = (unsigned char)((token >> 8) & 0xFF);
    raw[8] = (unsigned char)((token >> 16) & 0xFF);
    raw[9] = (unsigned char)((token >> 24) & 0xFF);
    return finish_frame(out, raw, 4);
}

unsigned char rt_build_player_state(unsigned char *out, unsigned seq,
                                    unsigned char x, unsigned char y,
                                    unsigned char facing,
                                    unsigned char buttons,
                                    unsigned char fire_counter,
                                    unsigned char pickup_counter,
                                    unsigned last_server_seq,
                                    unsigned char pvp_toggle_counter)
{
    unsigned char raw[23];

    raw[1] = RTS_VERSION;
    raw[2] = RTS_PLAYER_STATE;
    raw[3] = 0;
    raw[4] = (unsigned char)(seq & 0xFF);
    raw[5] = (unsigned char)(seq >> 8);
    raw[6] = x;
    raw[7] = y;
    raw[8] = facing;
    raw[9] = buttons;
    raw[10] = fire_counter;
    raw[11] = pickup_counter;
    raw[12] = (unsigned char)(last_server_seq & 0xFF);
    raw[13] = (unsigned char)(last_server_seq >> 8);
    raw[14] = 0; /* rx_drops */
    raw[15] = pvp_toggle_counter;
    return finish_frame(out, raw, 10);
}

unsigned char rt_build_resync_request(unsigned char *out, unsigned seq,
                                      unsigned char origin_x,
                                      unsigned char origin_y,
                                      unsigned char fill_origin_x,
                                      unsigned char fill_origin_y,
                                      unsigned long rows_have,
                                      unsigned char fill_id,
                                      unsigned char flags)
{
    unsigned char raw[17];

    raw[1] = RTS_VERSION;
    raw[2] = RTS_RESYNC_REQUEST;
    raw[3] = 0;
    raw[4] = (unsigned char)(seq & 0xFF);
    raw[5] = (unsigned char)(seq >> 8);
    raw[6] = origin_x;
    raw[7] = origin_y;
    raw[8] = fill_origin_x;
    raw[9] = fill_origin_y;
    raw[10] = (unsigned char)(rows_have & 0xFF);
    raw[11] = (unsigned char)((rows_have >> 8) & 0xFF);
    raw[12] = (unsigned char)((rows_have >> 16) & 0xFF);
    raw[13] = fill_id;
    raw[14] = flags;
    return finish_frame(out, raw, 9);
}

unsigned char rt_build_cache_step_ack(unsigned char *out, unsigned seq,
                                      unsigned revision,
                                      unsigned char origin_x,
                                      unsigned char origin_y)
{
    unsigned char raw[12];

    raw[1] = RTS_VERSION;
    raw[2] = RTS_CACHE_STEP_ACK;
    raw[3] = 0;
    raw[4] = (unsigned char)(seq & 0xFF);
    raw[5] = (unsigned char)(seq >> 8);
    raw[6] = (unsigned char)(revision & 0xFF);
    raw[7] = (unsigned char)(revision >> 8);
    raw[8] = origin_x;
    raw[9] = origin_y;
    return finish_frame(out, raw, 4);
}

unsigned char rt_build_window_commit(unsigned char *out, unsigned seq,
                                     unsigned char fill_id,
                                     unsigned char origin_x,
                                     unsigned char origin_y,
                                     unsigned char map_id,
                                     unsigned char flags)
{
    unsigned char raw[13];

    raw[1] = RTS_VERSION;
    raw[2] = RTS_WINDOW_COMMIT;
    raw[3] = 0;
    raw[4] = (unsigned char)(seq & 0xFF);
    raw[5] = (unsigned char)(seq >> 8);
    raw[6] = fill_id;
    raw[7] = origin_x;
    raw[8] = origin_y;
    raw[9] = map_id;
    raw[10] = flags;
    return finish_frame(out, raw, 5);
}

unsigned char rt_build_map_ready(unsigned char *out, unsigned seq,
                                 unsigned char map_id,
                                 unsigned char origin_x,
                                 unsigned char origin_y)
{
    unsigned char raw[11];

    raw[1] = RTS_VERSION;
    raw[2] = RTS_MAP_READY;
    raw[3] = 0;
    raw[4] = (unsigned char)(seq & 0xFF);
    raw[5] = (unsigned char)(seq >> 8);
    raw[6] = map_id;
    raw[7] = origin_x;
    raw[8] = origin_y;
    return finish_frame(out, raw, 3);
}

unsigned char rt_camera(unsigned char pos, unsigned char span,
                        unsigned char view)
{
    unsigned char half = view / 2;

    if (pos <= half) {
        return 0;
    }
    if (pos - half >= span - view) {
        return span - view;
    }
    return pos - half;
}

unsigned char rt_camera_track(unsigned char cam, unsigned char pos,
                              unsigned char span, unsigned char view,
                              unsigned char margin)
{
    unsigned char max_cam = span - view;
    unsigned char span_far = view - 1 - margin; /* 'far' is a cc65 keyword */
    unsigned char low = pos > span_far ? pos - span_far : 0;
    unsigned char high = pos > margin ? pos - margin : 0;

    if (cam < low) {
        cam = low;
    } else if (cam > high) {
        cam = high;
    }
    /* At the cache edge the camera cannot scroll further, so pos leaves the
       band and simply approaches the screen edge -- the same behavior the
       centering camera already had there. */
    if (cam > max_cam) {
        cam = max_cam;
    }
    return cam;
}
