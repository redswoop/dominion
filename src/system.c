/*
 * system.c — System-wide state global instance + initialization (Phase B3)
 */

#include "system.h"
#include <string.h>

system_t sys;

void system_init(system_t *s)
{
    /* Zero everything first */
    memset(s, 0, sizeof(system_t));

    /* Non-zero defaults */
    s->last_time_c = 0L;
    s->ARC_NUMBER = -1;
    s->MAX_BATCH = 0;
}
