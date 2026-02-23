/*
 * system.c — System-wide state singleton implementation (Phase C)
 */

#include "system.h"
#include <string.h>

System& System::instance()
{
    static System s;
    return s;
}

System::System()
{
    memset(this, 0, sizeof(System));
    ARC_NUMBER = -1;
}
