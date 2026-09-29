#include "host.h"
#include "server_host_default.h"
#include <coco.h>
#include <fujinet-fuji.h>

/* Same creator/app id every FujiRealm client uses -- identifies the
 * game, not the platform. */
#define FUJINET_CREATOR_ID 0x3022
#define FUJINET_APP_ID 2
#define APPKEY_SERVER_HOST 3

/* True if every byte in buf[0..count-1] is printable ASCII, as a host name
 * is. Rejects the leftover bytes DriveWire returns, with success, for a key
 * that was never written. */
static unsigned char looks_like_text(const unsigned char *buf, uint16_t count)
{
    uint16_t i;

    for (i = 0; i < count; ++i) {
        if (buf[i] < 0x20 || buf[i] > 0x7E)
            return 0;
    }
    return 1;
}

void host_init(char *host_buf)
{
    unsigned char buf[MAX_APPKEY_LEN + 2];
    uint16_t count;

    fuji_set_appkey_details(FUJINET_CREATOR_ID, FUJINET_APP_ID, DEFAULT);

    if (fuji_read_appkey(APPKEY_SERVER_HOST, &count, buf)
        && count >= 1 && count <= HOST_MAX_LEN
        && looks_like_text(buf, count)) {
        memcpy(host_buf, buf, count);
        host_buf[count] = 0;
        return;
    }

    strcpy(host_buf, SERVER_HOST_DEFAULT);
}
