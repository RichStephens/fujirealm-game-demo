#ifndef LOGIN_H
#define LOGIN_H

#include "identity.h"

/* Resolves this player's identity against host (the already-resolved
 * connection host from host.c) and the login server on LOGIN_SERVER_PORT.
 *
 * Reads appkey key_id 1 ("username,token,host"); if a saved record's host
 * field matches host exactly, tries RESUME_REQUEST first. Otherwise (or
 * if resume is rejected) prompts for a name (text mode, up to 3 tries on
 * "name taken") and sends LOGIN_REQUEST. On success saves the identity
 * back to the appkey and fills username_out (LOGIN_USERNAME_MAX+1 bytes)
 * and token_out (LOGIN_TOKEN_MAX+1 bytes, ASCII decimal digits).
 *
 * Returns 1 on success, 0 if the login server could not be reached, the
 * user canceled name entry, or every retry was rejected. Text mode only
 * -- call before gime_init_mode(). */
unsigned char login_identity(const char *host, char *username_out,
                             char *token_out);

#endif
