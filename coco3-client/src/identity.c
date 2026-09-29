#include "identity.h"
#include <coco.h>
#include <fujinet-fuji.h>

/* Same creator/app id every FujiRealm client uses -- identifies the
 * game, not the platform. Fixed key 1, distinct from display (2) and
 * server host (3) -- see display.c/host.c. */
#define FUJINET_CREATOR_ID 0x3022
#define FUJINET_APP_ID 2
#define APPKEY_IDENTITY 1
#define IDENTITY_MAX_LEN 64

/* Parses "username,token,host" from a raw appkey read into separate
 * fields, validating each byte range as it goes (no separate "looks like
 * text" pre-check is needed here the way display.c/host.c use one --
 * requiring two commas at valid positions, digit-only token, and an exact
 * host match is itself a strict content-plausibility check against the
 * DriveWire/AdamNet silent-success-on-ENOENT bug). Accepts a trailing
 * CR/LF after the host field, matching a hand-made key file. */
static unsigned char parse_identity(const unsigned char *buf, uint16_t count,
                                    const char *host,
                                    char *username_out, char *token_out)
{
    uint16_t i = 0;
    unsigned char j;
    unsigned char c;

    j = 0;
    while (i < count && j < LOGIN_USERNAME_MAX) {
        c = buf[i];
        if (c == ',') {
            break;
        }
        if (c < 0x20 || c > 0x7E) {
            return 0;
        }
        username_out[j++] = (char)c;
        ++i;
    }
    if (j == 0 || i >= count || buf[i] != ',') {
        return 0;
    }
    username_out[j] = 0;
    ++i;

    j = 0;
    while (i < count && j < LOGIN_TOKEN_MAX) {
        c = buf[i];
        if (c == ',') {
            break;
        }
        if (c < '0' || c > '9') {
            return 0;
        }
        token_out[j++] = (char)c;
        ++i;
    }
    if (j == 0 || i >= count || buf[i] != ',') {
        return 0;
    }
    token_out[j] = 0;
    ++i;

    j = 0;
    while (host[j] != 0) {
        if (i + j >= count || buf[i + j] != (unsigned char)host[j]) {
            return 0;
        }
        ++j;
    }
    i = (uint16_t)(i + j);
    if (i < count && buf[i] != '\r' && buf[i] != '\n') {
        return 0;
    }
    return 1;
}

unsigned char identity_load(const char *host, char *username_out,
                                   char *token_out)
{
    unsigned char buf[IDENTITY_MAX_LEN + 2];
    uint16_t count;

    fuji_set_appkey_details(FUJINET_CREATOR_ID, FUJINET_APP_ID, DEFAULT);

    if (!fuji_read_appkey(APPKEY_IDENTITY, &count, buf) || count == 0 ||
        count > IDENTITY_MAX_LEN) {
        return 0;
    }
    return parse_identity(buf, count, host, username_out, token_out);
}

void identity_store(const char *host, const char *username,
                           const char *token)
{
    unsigned char buf[IDENTITY_MAX_LEN];
    uint16_t len = 0;
    const char *p;

    for (p = username; *p; ++p) {
        buf[len++] = (unsigned char)*p;
    }
    buf[len++] = ',';
    for (p = token; *p; ++p) {
        buf[len++] = (unsigned char)*p;
    }
    buf[len++] = ',';
    for (p = host; *p; ++p) {
        buf[len++] = (unsigned char)*p;
    }

    fuji_set_appkey_details(FUJINET_CREATOR_ID, FUJINET_APP_ID, DEFAULT);
    fuji_write_appkey(APPKEY_IDENTITY, len, buf);
}

unsigned long identity_token(const char *token_ascii)
{
    unsigned long value = 0;
    const char *p;

    for (p = token_ascii; *p >= '0' && *p <= '9'; ++p) {
        value = value * 10 + (unsigned long)(*p - '0');
    }
    return value;
}

