#include "palette.h"

/* Overworld RGB half sourced from fujinet-battleship's real, shipped
 * palette[] array (src/coco/graphics.c) for the same semantic hues.
 * Composite half is ALSO battleship's real composite values, except
 * index 15 (dark green): battleship left it as an untuned "WHITE FOR
 * NOW" placeholder in both halves (their own comment names the intended
 * real values, 2 for RGB and 15 for composite) -- used those directly
 * since this array never inherited the placeholder to begin with.
 *
 * Cave and PvP reassign which of these SAME 16 proven codes occupies
 * which role (cave: warm-dark colors replace the cool teal/blue/foam
 * ones; PvP: red/orange replace them), rather than inventing new,
 * unverified 6-bit codes -- see palette.h. Both halves get the identical
 * reassignment so RGB/Composite stay a matched pair. */
const unsigned char coco_clut[PALETTE_ID_COUNT][2][16] = {
    /* PALETTE_ID_OVERWORLD */
    {
        { 0, 7, 56, 63, 28, 11, 1, 9, 25, 27, 4, 36, 38, 52, 54, 2 },
        { 0, 16, 32, 48, 30, 28, 13, 12, 44, 62, 7, 23, 22, 21, 36, 15 },
    },
    /* PALETTE_ID_CAVE -- darker/earthier: cool hues (teal/blue/foam)
     * replaced with dark red/red orange/dark gray; bright accents
     * (orange/yellow) dampened. Grays, white and black unchanged. */
    {
        { 0, 7, 56, 63, 4, 38, 1, 7, 4, 38, 4, 36, 38, 7, 4, 2 },
        { 0, 16, 32, 48, 7, 22, 13, 16, 7, 22, 7, 23, 22, 16, 7, 15 },
    },
    /* PALETTE_ID_PVP_REALM -- hotter/aggressive: cool hues replaced with
     * red/red orange/orange; yellow kept as a bright accent. */
    {
        { 0, 7, 56, 63, 36, 38, 4, 52, 36, 38, 4, 36, 38, 52, 54, 36 },
        { 0, 16, 32, 48, 23, 22, 7, 21, 23, 22, 7, 23, 22, 21, 36, 23 },
    },
};
