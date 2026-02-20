#pragma hdrstop
#pragma inline

#include "platform.h"

extern int running_dv;

int get_dv_version(void)
{
    union REGS r;
    int v;

    r.h.ah = 0x2B;
    r.h.al = 0x01;
    r.x.cx = 0x4445;
    r.x.dx = 0x5351;
    int86(0x21, &r, &r);
    if (r.h.al == 0xff) {
        running_dv = 0;
        return -1;
    } 
    else {
        v = r.x.bx;
        running_dv = 1;
        return v;
    }
}




void start_dv_crit(void)
{
        if (!running_dv)
        return;
        /* asm block removed */
        return;
}


void end_dv_crit(void)          // end DESQview non-swappable region
{
    if (!running_dv)
        return;
    /* asm block removed */
}


void dv_pause(void)         // relinquish remainder of timeslice
{
    if (!running_dv)
        return;
    /* asm block removed */
}
