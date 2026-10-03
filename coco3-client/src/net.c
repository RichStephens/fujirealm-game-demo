#include "net.h"
#include <coco.h>
#include <cmoc.h>
#include <fujinet-network.h>

void net_check_host(const char *host, unsigned int port)
{
    char devicespec[64];
    unsigned char ok;

    sprintf(devicespec, "N1:TCP://%s:%u/", host, port);
    printf("Checking %s:%u...\n", host, port);
    ok = (unsigned char)(network_open(devicespec, 12, 0) == FN_ERR_OK);
    /* Close even on failure: skipping it broke F1/F2 polling after
     * returning to graphics mode on real hardware. */
    network_close(devicespec);
    if (ok) {
        printf("Connected: the login server is reachable.\n");
    } else {
        printf("Cannot connect to %s.\n", host);
    }
}
