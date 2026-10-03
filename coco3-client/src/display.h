#ifndef DISPLAY_H
#define DISPLAY_H

#define DISPLAY_UNSET 0xFF

/* The saved display target (appkey key_id 2): DISPLAY_RGB or
 * DISPLAY_COMPOSITE (see palette.h), or DISPLAY_UNSET if none is saved. A
 * valid saved value also becomes the current target for
 * display_target_toggle(). */
unsigned char display_target_saved(void);

/* Makes t the current target and persists it to the appkey. */
void display_target_set(unsigned char t);

/* Flips the current target, persists it to the appkey, and returns the
 * new value. */
unsigned char display_target_toggle(void);

/* White text on black, black border, for the current target (RGB until one
 * is known). Call after each width(80). */
void display_text_colors(void);

/* Setup only (displayinit.c): resolves the target, prompting in plain text
 * mode and saving the answer on first run. */
unsigned char display_target_init(void);

#endif
