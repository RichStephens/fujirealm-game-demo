#ifndef LIVE_H
#define LIVE_H

#include "rt_state.h"
#include "terrain.h"

/* The live realtime session: connects and authenticates, polls the
 * connection, folds server frames into live_game/live_terrain, and answers
 * the terrain-cache handshakes. */

extern struct rt_state live_game;
extern struct terrain_cache live_terrain;

/* Counts WORLD_STATE frames (10 Hz server ticks): the movement clock. */
extern unsigned live_world_tick;
/* Facing reported in PLAYER_STATE; the caller keeps it current. */
extern unsigned char live_facing;
/* Palette id of the current map (MAP_CHANGE); 0 until one arrives. */
extern unsigned char live_map_palette;
/* A map transition is loading; input and drawing are held. */
extern unsigned char live_map_loading;
/* Every terrain cell may have changed (fill activated): redraw. */
extern unsigned char live_terrain_reset;
/* Cleared while the server has been silent for several seconds. */
extern unsigned char live_connected;
/* A complete terrain fill has been applied at least once. */
extern unsigned char live_have_terrain;
/* Carried in every PLAYER_STATE; the server acts once per change (ENTER/SPACE
 * talk or pick up, P toggles PvP). The caller bumps them. */
extern unsigned char live_pickup_counter;
extern unsigned char live_pvp_counter;
/* RTS_BUTTON_DIALOGUE_DECLINE after a dialogue was declined, until the next
 * one opens; only the dialogue window sets it. */
extern unsigned char live_dlg_decline;

/* Opens the connection to host:HYBRID_SERVER_PORT and sends the realtime
 * preamble and AUTH for token. There is no bootstrap: the server streams the
 * terrain window in-band (WINDOW_ROW) because this session never bootstrapped.
 * Returns 0 if the connection or either write failed. */
unsigned char live_connect(const char *host, unsigned long token);

/* Blocks until the first WORLD_STATE and the first complete terrain fill have
 * arrived; returns 0 on timeout. */
unsigned char live_wait_ready(unsigned timeout_ticks);

/* Closes the connection. */
void live_close(void);

/* Reads and applies whatever the server has sent. Cheap when idle. */
void live_pump(void);

/* Queues a PLAYER_STATE for the local player's position (sent by the next
 * live_service). fire != 0 sets the fire button and bumps the fire counter. */
void live_send_state(unsigned char facing, unsigned char fire);

/* Retries pending cache handshakes and sends everything queued. Call once
 * per loop pass. */
void live_service(void);

#endif
