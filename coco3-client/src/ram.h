#ifndef RAM_H
#define RAM_H

/* Returns 1 if 512K RAM is installed, 0 if only 128K. Must be called
 * after gime_init_mode() (needs task1's low four slots already set) and
 * before gime_select_buffers(). */
unsigned char ram_probe_is_512k(void);

#endif
