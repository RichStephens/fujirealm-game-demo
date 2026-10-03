#include "bootstrap.h"
#include "bf_proto.h"
#include "server_host_default.h"
#include <coco.h>
#include <fujinet-network.h>

#define PT_HELLO 0x01
#define PT_WELCOME 0x81
#define PT_WINDOW 0x80

#define BOOTSTRAP_ALL_ROWS 0x00FFFFFFUL
#define BOOTSTRAP_TICKS_TIMEOUT 1200      /* ~20s at 60Hz, matches intv-client's bs_run */
#define BOOTSTRAP_HELLO_RETRY_TICKS 120   /* ~2s quiet before re-sending HELLO */
#define BOOTSTRAP_MAX_TRIES 10
#define BOOTSTRAP_NET_MAX_READ 192

static unsigned char send_hello(const char *devicespec, unsigned long token)
{
    unsigned char payload[7];
    unsigned char frame[13];
    unsigned char len;

    payload[0] = 0; /* flags: default link profile */
    payload[1] = 1; /* seed lo */
    payload[2] = 0; /* seed hi */
    payload[3] = (unsigned char)(token & 0xFF);
    payload[4] = (unsigned char)((token >> 8) & 0xFF);
    payload[5] = (unsigned char)((token >> 16) & 0xFF);
    payload[6] = (unsigned char)((token >> 24) & 0xFF);
    len = bf_build(frame, PT_HELLO, payload, 7);
    return network_write(devicespec, frame, len) == FN_ERR_OK;
}

unsigned char bootstrap_fetch_window(const char *host, unsigned long token,
                                     unsigned char *terrain,
                                     unsigned char *origin_x,
                                     unsigned char *origin_y,
                                     unsigned char *player_id)
{
    char devicespec[64];
    struct bf_parser parser;
    struct bf_packet packet;
    unsigned char buf[BOOTSTRAP_NET_MAX_READ];
    unsigned char got_welcome = 0;
    unsigned char have_tick = 0;
    unsigned long rows_have = 0;
    unsigned tick = 0;
    uint16_t bytes_waiting;
    uint8_t conn_status;
    uint8_t err;
    int16_t got;
    unsigned int start_tick;
    unsigned int quiet_tick;
    unsigned char tries;
    unsigned char i;

    sprintf(devicespec, "N1:TCP://%s:%u/", host, HYBRID_SERVER_PORT);

    if (network_open(devicespec, 12, 0) != FN_ERR_OK) {
        network_close(devicespec);
        return 0;
    }

    bf_parser_init(&parser);
    if (!send_hello(devicespec, token)) {
        network_close(devicespec);
        return 0;
    }

    tries = 1;
    start_tick = getTimer();
    quiet_tick = start_tick;

    while (!got_welcome || rows_have != BOOTSTRAP_ALL_ROWS) {
        if ((unsigned int)(getTimer() - start_tick) >= BOOTSTRAP_TICKS_TIMEOUT) {
            network_close(devicespec);
            return 0;
        }
        if ((unsigned int)(getTimer() - quiet_tick) >= BOOTSTRAP_HELLO_RETRY_TICKS) {
            if (tries >= BOOTSTRAP_MAX_TRIES) {
                network_close(devicespec);
                return 0;
            }
            ++tries;
            send_hello(devicespec, token);
            quiet_tick = getTimer();
        }

        if (network_status(devicespec, &bytes_waiting, &conn_status, &err) !=
            FN_ERR_OK) {
            continue;
        }
        if (bytes_waiting == 0) {
            continue;
        }

        got = network_read_nb(devicespec, buf, BOOTSTRAP_NET_MAX_READ);
        if (got < 0) {
            network_close(devicespec);
            return 0;
        }
        quiet_tick = getTimer();

        for (i = 0; i < (unsigned char)got; ++i) {
            if (!bf_parser_feed(&parser, buf[i], &packet)) {
                continue;
            }

            if (packet.type == PT_WELCOME && packet.payload_len == 5) {
                *player_id = packet.payload[0];
                got_welcome = 1;
            } else if (packet.type == PT_WINDOW && packet.payload_len >= 12) {
                const unsigned char *p = packet.payload;
                unsigned this_tick = (unsigned)p[0] | ((unsigned)p[1] << 8);
                unsigned ox = (unsigned)p[2] | ((unsigned)p[3] << 8);
                unsigned oy = (unsigned)p[4] | ((unsigned)p[5] << 8);
                unsigned char width = p[6];
                unsigned char height = p[7];
                unsigned char chunk_y = p[8];
                unsigned char chunk_h = p[9];
                unsigned tile_count = (unsigned)p[10] | ((unsigned)p[11] << 8);
                unsigned char row;
                unsigned char col;
                unsigned src;
                unsigned dst;

                if (width != BOOTSTRAP_WINDOW_W || height != BOOTSTRAP_WINDOW_H ||
                    chunk_h == 0 || (unsigned)(chunk_y + chunk_h) > height ||
                    tile_count != (unsigned)width * chunk_h ||
                    packet.payload_len != 12 + tile_count) {
                    continue;
                }
                if (have_tick && tick != this_tick) {
                    /* A re-sent HELLO restarted the transfer server-side
                     * with a new generation -- drop the partial old rows. */
                    rows_have = 0;
                }
                tick = this_tick;
                have_tick = 1;
                *origin_x = (unsigned char)ox;
                *origin_y = (unsigned char)oy;

                src = 12;
                for (row = 0; row < chunk_h; ++row) {
                    dst = (unsigned)(chunk_y + row) * BOOTSTRAP_WINDOW_W;
                    for (col = 0; col < width; ++col) {
                        terrain[dst + col] = p[src++];
                    }
                    rows_have |= 1UL << (chunk_y + row);
                }
            }
        }
    }

    return 1;
}
