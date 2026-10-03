#include "prefs.h"
#include <cmoc.h>
#include <fujinet-fuji.h>

#define FUJINET_CREATOR_ID 0x3022
#define FUJINET_APP_ID 2

unsigned char pref_items_seen_load(unsigned long token)
{
    unsigned char buf[MAX_APPKEY_LEN + 2];
    uint16_t count;

    fuji_set_appkey_details(FUJINET_CREATOR_ID, FUJINET_APP_ID, DEFAULT);
    if (fuji_read_appkey(APPKEY_ITEMS_SEEN, &count, buf) &&
        count == ITEMS_LEN && buf[0] == ITEMS_MAGIC &&
        memcmp(buf + 1, &token, 4) == 0) {
        return buf[5];
    }
    return 0;
}
