#ifndef INPUT_H
#define INPUT_H

/* True if caps lock is on. Typing is lowercase, SHIFT gives uppercase; caps
 * lock reverses both. SHIFT-0 toggles it (via Color BASIC's $011A flag, which
 * is read, never written). */
unsigned char input_caps_locked(void);

/* Prints "abc"/"ABC" at (col,row): the case unshifted letters will have. */
void input_caps_indicator(unsigned char col, unsigned char row);

/* inkey()-based line editor at (col,row), unlike readline()/waitkey()
 * respects real SHIFT state rather than forcing uppercase (see
 * fujinet-config/src/coco/input.c's input()). The case indicator is drawn
 * at (col,row+1). Left arrow is CoCo's own backspace: it moves back one
 * column and truncates the buffer there (matching input_line's own
 * convention), so retyping after backing up replaces the rest of the line
 * rather than inserting into it. Right arrow moves forward through any
 * untouched tail. ENTER accepts, BREAK cancels. buf (max+1 bytes) holds the
 * initial text (may be empty) and receives the result; on BREAK it holds
 * whatever was last displayed, not necessarily the original text. Text mode
 * only. */
unsigned char input_line(unsigned char col, unsigned char row, char *buf,
                         unsigned char max);

/* Prompts for a nonblank name (1..max chars, CLS'd first): "Enter your name: "
 * on row 0 with the entry field right after it, case indicator below, and
 * note (may be NULL) on row 3 while entering. Returns 1 on ENTER with a
 * nonblank name in buf, 0 on BREAK; leaves the cursor at row 2. Text mode
 * only. */
unsigned char input_name_entry(char *buf, unsigned char max, const char *note);

#endif
