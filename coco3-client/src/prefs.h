#ifndef PREFS_H
#define PREFS_H

/* Items the player has carried at least once: bit n set for item id n
 * (appkey key_id 5, record { ITEMS_MAGIC, token, seen }). The record names
 * its player by login token; another player's record reads as none seen.
 * FRLOGIN loads it (itemseen.c) and hands it over in OVL_BLOCK. */
#define APPKEY_ITEMS_SEEN 5
#define ITEMS_MAGIC 0x5A
#define ITEMS_LEN 6

unsigned char pref_items_seen_load(unsigned long token);

void pref_items_seen_save(unsigned long token, unsigned char seen);

#endif
