#ifndef TESTPAT_H
#define TESTPAT_H

/* Draws the placeholder screen: a 20x12 grid of flat-colored tiles in the
 * playfield plus a few solid color bars in the HUD strip. `phase` shifts
 * the playfield's color cycle (Phase 2: alternated 0/1 between the two
 * double-buffered draws, so flipping is visibly provable, not just
 * detected). Brackets itself with GFX_ENTER()/GFX_LEAVE() (see gime.h) to
 * reach task1's graphics window -- always draws into whatever buffer is
 * currently selected as the draw target. Setup binary only. */
void draw_test_screen(unsigned char phase);

#endif
