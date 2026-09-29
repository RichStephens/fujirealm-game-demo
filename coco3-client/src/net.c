#include "net.h"
#include <coco.h>
#include <cmoc.h>
#include <fujinet-fuji.h>
#include <fujinet-network.h>

#define NET_MAX_READ 512
#define NET_TEST_TICKS 180 /* ~3s at the 60Hz tick getTimer()/setTimer() use */

/* tools/serial_test_server.py's default `--pattern aa`, so it runs with no
 * arguments. */
#define NET_TEST_PATTERN_BYTE 0xAA

void net_measure_throughput(const char *host, unsigned int port)
{
    char devicespec[64];
    unsigned char buf[NET_MAX_READ];
    uint16_t bytes_waiting;
    uint8_t conn_status;
    uint8_t err;
    int16_t got;
    unsigned int start_tick, elapsed_ticks;
    unsigned long total_bytes = 0;
    unsigned long pattern_errors = 0;
    uint16_t i;

    sprintf(devicespec, "N1:TCP://%s:%u/", host, port);

    printf("Connecting to %s\n", devicespec);
    if (network_open(devicespec, 12, 0) != FN_ERR_OK) {
        printf("network_open failed.\n");
        /* Close even on failure: skipping it broke F1/F2 polling after
         * returning to graphics mode on real hardware. */
        network_close(devicespec);
        printf("Press any key to continue...\n");
        waitkey(0);
        return;
    }

    start_tick = getTimer();

    while ((unsigned int)(getTimer() - start_tick) < NET_TEST_TICKS) {
        if (network_status(devicespec, &bytes_waiting, &conn_status, &err) != FN_ERR_OK)
            break;

        if (bytes_waiting == 0)
            continue;

        /* Always request the fixed max: the DriveWire firmware sends
         * everything buffered but only erases what was asked for, so
         * under-asking desyncs the stream. The return value, not
         * bytes_waiting, is the count delivered. */
        got = network_read_nb(devicespec, buf, NET_MAX_READ);
        if (got < 0)
            break;

        for (i = 0; i < (uint16_t)got; ++i) {
            if (buf[i] != NET_TEST_PATTERN_BYTE)
                ++pattern_errors;
        }
        total_bytes += (unsigned long)got;

        /* Exercise the uplink too, matching the test server's own
         * bidirectional design (it verifies whatever it receives). */
        network_write(devicespec, buf, 1);
    }

    network_close(devicespec);

    elapsed_ticks = getTimer() - start_tick;
    printf("Received %lu bytes in %u ticks\n", total_bytes, elapsed_ticks);
    if (elapsed_ticks > 0)
        printf("~%lu bytes/sec\n", (total_bytes * 60UL) / elapsed_ticks);
    printf("Pattern errors: %lu\n", pattern_errors);
    printf("Press any key to continue...\n");
    waitkey(0);
}
