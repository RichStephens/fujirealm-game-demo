#include "runm.h"
#include <cmoc.h>
#include <coco.h>

void runm(const char *filename)
{
    /* Rebuild the state BASIC has when executing RUNM"NAME": a compressed
     * command line at $2DD ("M" then the quote), CHARAD ($A6) pointing at
     * it, and A = 'M' on entry to the RUN procedure. */
    *((unsigned *)0x2DD) = 0x4D22;
    strcpy((char *)0x2DF, filename);
    *((unsigned *)0xA6) = 0x2DD;

    asm {
        ldd #$4D1C
        jmp $AE75
    }
}
