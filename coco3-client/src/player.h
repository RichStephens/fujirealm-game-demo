#ifndef PLAYER_H
#define PLAYER_H

/* Local player movement, prediction and camera, following the Atari client:
 * a step is predicted the instant it is legal against the cached terrain, and
 * WORLD_STATE only corrects it (snap if two or more tiles off, otherwise ease
 * in one tile at a time once the server has echoed our last move). */

#define VIEW_COLS 20
#define VIEW_ROWS 12

/* Predicted local position -- what is drawn. live_game.player_x/y is the
 * server's. */
extern unsigned char player_x;
extern unsigned char player_y;

/* World coordinates of the viewport's top-left tile. */
extern unsigned view_x;
extern unsigned view_y;

/* A predicted step is waiting for its PLAYER_STATE. */
extern unsigned char player_send_pending;
/* Counts down while the local player blinks after losing HP; drawn only on
 * even values (Atari enemy hit blink). */
extern unsigned char player_hit_timer;
/* Walk frame (0 or 1), flipped by each step (Atari player_anim). */
extern unsigned char player_anim;

/* Puts the player, prediction state and camera on (x, y): first world state,
 * map change, teleport. */
void player_snap(unsigned char x, unsigned char y);

/* Steps one tile in an RTS_FACE_* direction if the cached terrain and
 * entities allow it (Atari move_if_clear). Returns 1 if the player moved. */
unsigned char player_try_move(unsigned char dir);

/* Folds the position in the WORLD_STATE just applied to live_game into the
 * local one (Atari netstream_apply_world_state). */
void player_on_world_state(void);

/* Advances a queued correction one tile (Atari
 * apply_pending_player_correction). Call once per loop pass. */
void player_apply_correction_step(void);

/* Starts a cosmetic shot from the player in an RTS_FACE_* direction (Atari:
 * one tracer stepping per BULLET tick, stopped by terrain, actors and range).
 * The server alone decides damage. */
void player_fire(unsigned char dir);

/* Advances every live tracer -- ours and remote players' -- one tile. */
void player_tracers_step(void);

/* Records that PLAYER_STATE `seq` went out carrying the current position. */
void player_note_sent(unsigned seq);

/* Scrolls the viewport one tile toward the player if the player is past a
 * scroll margin, then keeps it inside the cached window (Atari
 * net_update_view_position). */
void player_update_view(void);

#endif
