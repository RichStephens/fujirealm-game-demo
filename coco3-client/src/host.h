#ifndef HOST_H
#define HOST_H

#define HOST_MAX_LEN 32

/* Resolves the connection host into host_buf (must be at least
 * HOST_MAX_LEN+1 bytes): the compiled-in default (server_host_default.h),
 * overridden by appkey key_id 3 if a saved value exists. Text mode only
 * -- call before gime_init_mode(). */
void host_init(char *host_buf);

/* Setup only (hostedit.c): echoes the current host, prompts for a new one
 * (blank input keeps the current value), and saves it to the appkey if
 * changed. Returns 1 if the host actually changed, 0 otherwise. Text mode
 * only. */
unsigned char host_edit_prompt(char *host_buf);

#endif
