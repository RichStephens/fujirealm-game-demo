#ifndef RUNM_H
#define RUNM_H

/* Launches a machine-language program from the current disk exactly as
 * RUNM"name" would, then never returns. The load overwrites this program,
 * which is fine: it runs from BASIC's RUN code, driven by the command
 * line, not from our memory. Same technique as fujinet-battleship's
 * support/coco/loader.c. */
void runm(const char *filename);

#endif
