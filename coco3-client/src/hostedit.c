#include "host.h"
#include "input.h"
#include <coco.h>
#include <cmoc.h>
#include <fujinet-fuji.h>

#define FUJINET_CREATOR_ID 0x3022
#define FUJINET_APP_ID 2
#define APPKEY_SERVER_HOST 3

unsigned char host_edit_prompt(char *host_buf)
{
    char before[HOST_MAX_LEN + 1];
    unsigned char len;

    strcpy(before, host_buf);

    /* input_line(), not readline(): readline() always forces uppercase
     * regardless of real SHIFT state (see input.h). */
    printf("Server host (ENTER=keep, BREAK=cancel):\n");
    if (!input_line(0, 1, host_buf, HOST_MAX_LEN)) {
        putchar('\n');
        strcpy(host_buf, before);
        return 0;
    }
    putchar('\n');

    len = (unsigned char)strlen(host_buf);
    if (len == 0) {
        strcpy(host_buf, before);
        return 0;
    }

    fuji_write_appkey(APPKEY_SERVER_HOST, len, (unsigned char *)host_buf);
    return 1;
}
