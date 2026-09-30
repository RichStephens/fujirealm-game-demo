#include "prefs.h"
#include <coco.h>
#include <cmoc.h>
#include <fujinet-fuji.h>

/* Same creator/app id every FujiRealm client uses -- identifies the game, not
 * the platform. */
#define FUJINET_CREATOR_ID 0x3022
#define FUJINET_APP_ID 2

void pref_items_seen_save(unsigned long token, unsigned char seen)
{
    unsigned char rec[ITEMS_LEN];

    rec[0] = ITEMS_MAGIC;
    memcpy(rec + 1, &token, 4);
    rec[5] = seen;
    fuji_set_appkey_details(FUJINET_CREATOR_ID, FUJINET_APP_ID, DEFAULT);
    fuji_write_appkey(APPKEY_ITEMS_SEEN, ITEMS_LEN, rec);
}
