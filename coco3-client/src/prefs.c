#include "prefs.h"
#include <coco.h>
#include <cmoc.h>
#include <fujinet-fuji.h>

/* Same creator/app id every FujiRealm client uses -- identifies the game, not
 * the platform. Keys 1-3 are identity, display target and server host. */
#define FUJINET_CREATOR_ID 0x3022
#define FUJINET_APP_ID 2
#define APPKEY_HWSCROLL 4

/* Stored as { MAGIC, 0 or 1 }: one byte holding 0 or 1 could be mistaken for
 * garbage from a never-written key. */
#define MAGIC 0xA5

unsigned char pref_hwscroll_load(void)
{
    unsigned char buf[MAX_APPKEY_LEN + 2];
    uint16_t count;

    fuji_set_appkey_details(FUJINET_CREATOR_ID, FUJINET_APP_ID, DEFAULT);
    if (fuji_read_appkey(APPKEY_HWSCROLL, &count, buf) && count == 2 &&
        buf[0] == MAGIC && buf[1] <= 1) {
        return buf[1];
    }
    return 1;
}

void pref_items_seen_save(unsigned long token, unsigned char seen)
{
    unsigned char rec[ITEMS_LEN];

    rec[0] = ITEMS_MAGIC;
    memcpy(rec + 1, &token, 4);
    rec[5] = seen;
    /* pref_hwscroll_load() already set the appkey details. */
    fuji_write_appkey(APPKEY_ITEMS_SEEN, ITEMS_LEN, rec);
}

void pref_hwscroll_save(unsigned char on)
{
    unsigned char rec[2];

    rec[0] = MAGIC;
    rec[1] = on ? 1 : 0;
    fuji_set_appkey_details(FUJINET_CREATOR_ID, FUJINET_APP_ID, DEFAULT);
    fuji_write_appkey(APPKEY_HWSCROLL, 2, rec);
}
