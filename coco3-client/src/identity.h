#ifndef IDENTITY_H
#define IDENTITY_H

#define LOGIN_USERNAME_MAX 10
#define LOGIN_TOKEN_MAX 10

/* Reads appkey key_id 1 ("username,token,host"). Returns 1 and fills
 * username_out (LOGIN_USERNAME_MAX+1 bytes) and token_out (LOGIN_TOKEN_MAX+1
 * bytes, ASCII decimal digits) only if the saved record is well formed and
 * its host field equals host exactly; 0 otherwise. */
unsigned char identity_load(const char *host, char *username_out,
                            char *token_out);

/* Saves "username,token,host" to appkey key_id 1. */
void identity_store(const char *host, const char *username,
                    const char *token);

/* Parses an ASCII decimal token into the 32-bit value the AUTH wire packet
 * carries. */
unsigned long identity_token(const char *token_ascii);

#endif
