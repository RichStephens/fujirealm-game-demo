#ifndef BOOTSTRAP_H
#define BOOTSTRAP_H

#include "terrain.h"

/* NOT BUILT: superseded by the server's in-band window fill (see live.c).
 * Kept for reference. */

/* Fetches the initial 32x24 terrain window over HYBRID_SERVER_PORT: opens
 * the connection, sends a $BF HELLO carrying the login token, then
 * collects WELCOME plus the 8 WINDOW chunks that tile the window,
 * re-sending HELLO on quiet spells (each retry restarts the transfer
 * server-side, matching lynx-client/src/bootstrap.c's apply_welcome/
 * apply_window). Only the initial window is fetched here; live
 * TERRAIN_EDGE/resync handling is in terrain.c and live.c.
 *
 * On success leaves the connection OPEN (realtime_authenticate() carries
 * on over the same devicespec) and fills *origin_x/*origin_y (the
 * window's absolute world origin), *player_id (from WELCOME), and
 * terrain (must hold BOOTSTRAP_TERRAIN_SIZE bytes). Closes the connection
 * and returns 0 on failure/timeout (~20s). Text mode only. */
unsigned char bootstrap_fetch_window(const char *host, unsigned long token,
                                     unsigned char *terrain,
                                     unsigned char *origin_x,
                                     unsigned char *origin_y,
                                     unsigned char *player_id);

#endif
